"""
Visualizzatore microfono in tempo reale
- Segnale nel tempo (finestra 100 ms)
- Spettro FFT (0 – 10 kHz)
- Connessione UART 921600 baud
- Campioni 12 bit little-endian (2 byte/campione)
"""

import tkinter as tk
from tkinter import ttk
import threading
import queue
import struct
import time
import collections

import serial
import serial.tools.list_ports

import numpy as np
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation

# ── Costanti ────────────────────────────────────────────────────────────────
BAUD_RATE        = 921600
SAMPLE_RATE      = 20_000          # Hz
BYTES_PER_SAMPLE = 2
MASK_12BIT       = 0x0FFF
WINDOW_MS        = 100             # ms di segnale visualizzato
WINDOW_SAMPLES   = int(SAMPLE_RATE * WINDOW_MS / 1000)   # 2000 campioni


class MicVisualizerApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Visualizzatore Microfono – 20 kHz")
        self.root.configure(bg="#1e1e2e")

        self.serial_port: serial.Serial | None = None
        self.running = False
        self.data_queue: queue.Queue[int] = queue.Queue(maxsize=200_000)

        # Buffer circolare per la finestra temporale
        self.buffer = collections.deque([2048] * WINDOW_SAMPLES,
                                        maxlen=WINDOW_SAMPLES)

        # Asse del tempo in ms (0 → WINDOW_MS)
        self.t_axis = np.linspace(0, WINDOW_MS, WINDOW_SAMPLES)

        # Asse delle frequenze per la FFT
        self.freq_axis = np.fft.rfftfreq(WINDOW_SAMPLES, d=1.0 / SAMPLE_RATE)

        self._build_controls()
        self._build_plot()
        self._start_animation()

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ── Costruzione UI ──────────────────────────────────────────────────────

    def _build_controls(self):
        bar = ttk.LabelFrame(self.root, text="Connessione", padding=8)
        bar.pack(side=tk.TOP, fill=tk.X, padx=10, pady=(10, 0))

        ttk.Label(bar, text="Porta COM:").grid(row=0, column=0, padx=(0, 4))

        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(bar, textvariable=self.port_var,
                                       width=14, state="readonly")
        self.port_combo.grid(row=0, column=1, padx=(0, 4))

        ttk.Button(bar, text="Aggiorna",
                   command=self._refresh_ports).grid(row=0, column=2, padx=(0, 4))

        self.connect_btn = ttk.Button(bar, text="Connetti",
                                      command=self._toggle_connection)
        self.connect_btn.grid(row=0, column=3, padx=(0, 14))

        ttk.Label(bar,
                  text=f"{BAUD_RATE} baud  |  8-N-1  |  {SAMPLE_RATE // 1000} kHz",
                  foreground="gray").grid(row=0, column=4)

        self.status_var = tk.StringVar(value="Disconnesso")
        ttk.Label(bar, textvariable=self.status_var,
                  foreground="#a6e3a1").grid(row=0, column=5, padx=(20, 0))

        self._refresh_ports()

    def _build_plot(self):
        self.fig = Figure(figsize=(12, 7), facecolor="#1e1e2e")

        # ── Segnale nel tempo ───────────────────────────────────────────────
        self.ax_time = self.fig.add_subplot(2, 1, 1)
        self.ax_time.set_facecolor("#181825")
        self.ax_time.set_title("Segnale nel tempo", color="#cdd6f4", fontsize=11)
        self.ax_time.set_xlabel("Tempo (ms)", color="#cdd6f4")
        self.ax_time.set_ylabel("ADC (0 – 4095)", color="#cdd6f4")
        self.ax_time.set_xlim(0, WINDOW_MS)
        self.ax_time.set_ylim(0, 4095)
        self.ax_time.tick_params(colors="#cdd6f4")
        for spine in self.ax_time.spines.values():
            spine.set_edgecolor("#313244")

        self.line_time, = self.ax_time.plot(
            self.t_axis, [2048] * WINDOW_SAMPLES,
            color="#89b4fa", linewidth=0.7
        )

        # Linea di riferimento a 2048 (metà scala = tensione di riposo)
        self.ax_time.axhline(2048, color="#585b70", linewidth=0.5, linestyle="--")

        # ── Spettro FFT ─────────────────────────────────────────────────────
        self.ax_fft = self.fig.add_subplot(2, 1, 2)
        self.ax_fft.set_facecolor("#181825")
        self.ax_fft.set_title("Spettro in frequenza (FFT)", color="#cdd6f4", fontsize=11)
        self.ax_fft.set_xlabel("Frequenza (Hz)", color="#cdd6f4")
        self.ax_fft.set_ylabel("Ampiezza", color="#cdd6f4")
        self.ax_fft.set_xlim(0, SAMPLE_RATE // 2)
        self.ax_fft.set_ylim(0, 100)
        self.ax_fft.tick_params(colors="#cdd6f4")
        for spine in self.ax_fft.spines.values():
            spine.set_edgecolor("#313244")

        self.line_fft, = self.ax_fft.plot(
            self.freq_axis, np.zeros(len(self.freq_axis)),
            color="#a6e3a1", linewidth=0.7
        )

        self.fig.tight_layout(pad=2.5)

        canvas = FigureCanvasTkAgg(self.fig, master=self.root)
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=10, pady=8)
        self.canvas = canvas

    def _start_animation(self):
        """Aggiorna il grafico ogni 100 ms."""
        self.ani = animation.FuncAnimation(
            self.fig, self._update_plot, interval=100, blit=False
        )

    # ── Aggiornamento grafico ───────────────────────────────────────────────

    def _update_plot(self, _frame):
        # Svuota la coda e riempie il buffer circolare
        try:
            while True:
                self.buffer.append(self.data_queue.get_nowait())
        except queue.Empty:
            pass

        y = np.array(self.buffer, dtype=np.float32)

        # -- Dominio del tempo ----------------------------------------------
        self.line_time.set_ydata(y)

        # -- FFT: rimuove la componente DC prima di trasformare -------------
        y_ac = y - y.mean()
        fft_vals = np.abs(np.fft.rfft(y_ac)) * (2.0 / WINDOW_SAMPLES)
        self.line_fft.set_ydata(fft_vals)

        # Aggiusta asse Y della FFT dinamicamente
        peak = float(fft_vals.max())
        if peak > 1:
            self.ax_fft.set_ylim(0, peak * 1.2)

        self.canvas.draw_idle()
        return []

    # ── Porta seriale ───────────────────────────────────────────────────────

    def _refresh_ports(self):
        ports = sorted(p.device for p in serial.tools.list_ports.comports())
        self.port_combo["values"] = ports
        if ports:
            self.port_combo.set(ports[0])
        self.status_var.set(f"{len(ports)} porta/e trovata/e")

    def _toggle_connection(self):
        if self.running:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        port = self.port_var.get()
        if not port:
            return
        try:
            self.serial_port = serial.Serial(
                port=port, baudrate=BAUD_RATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
        except serial.SerialException as e:
            self.status_var.set(f"Errore: {e}")
            return

        self.running = True
        self.connect_btn.config(text="Disconnetti")
        self.status_var.set(f"Connesso a {port}")
        threading.Thread(target=self._read_loop, daemon=True).start()

    def _disconnect(self):
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.serial_port = None
        self.connect_btn.config(text="Connetti")
        self.status_var.set("Disconnesso")

    def _read_loop(self):
        """
        Thread in background: legge dalla seriale a burst,
        decodifica parole da 2 byte little-endian, estrae 12 bit,
        mette i campioni nella coda.
        """
        leftover = b""
        while self.running:
            try:
                if not self.serial_port or not self.serial_port.is_open:
                    break
                n = self.serial_port.in_waiting
                if n >= BYTES_PER_SAMPLE:
                    # Legge solo byte multipli di 2 per non spezzare le parole
                    chunk = self.serial_port.read(n - n % BYTES_PER_SAMPLE)
                    data = leftover + chunk
                    leftover = b""
                    idx = 0
                    while idx + BYTES_PER_SAMPLE <= len(data):
                        # Little-endian: byte[0] = LSB, byte[1] = MSB
                        word = struct.unpack_from("<H", data, idx)[0]
                        value = word & MASK_12BIT   # tieni solo i 12 bit ADC
                        try:
                            self.data_queue.put_nowait(value)
                        except queue.Full:
                            pass   # se la coda è piena, scarta il campione
                        idx += BYTES_PER_SAMPLE
                    if idx < len(data):
                        leftover = data[idx:]       # byte rimasto in sospeso
                else:
                    time.sleep(0.001)
            except serial.SerialException:
                break
            except Exception:
                break

        self.root.after(0, self._on_read_stopped)

    def _on_read_stopped(self):
        if self.running:
            self._disconnect()

    def _on_close(self):
        self._disconnect()
        self.root.destroy()


# ── Entry point ─────────────────────────────────────────────────────────────

def main():
    root = tk.Tk()
    root.geometry("1200x780")
    MicVisualizerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
