# import re
# import serial
# import threading
# import tkinter as tk
# from tkinter.scrolledtext import ScrolledText

# PORT = "COM5"      # Change this to your FPGA UART COM port
# BAUD = 9600

# SHOW_TAGS = {"OP0", "OP1", "OP2", "OP3", "OP4"}

# tag_re = re.compile(r"^\[([A-Z0-9]+)\]")


# class SerialViewer:
#     def __init__(self, root):
#         self.root = root
#         self.root.title("Filtered Serial Debug Viewer")
#         self.root.geometry("1100x700")
#         self.root.configure(bg="#1E1E1E")

#         header = tk.Label(
#             root,
#             text=f"Filtered Serial Viewer | {PORT} @ {BAUD}",
#             bg="#121212",
#             fg="#FFFFFF",
#             font=("Consolas", 20, "bold"),
#             pady=12,
#         )
#         header.pack(fill=tk.X)

#         self.text = ScrolledText(
#             root,
#             bg="#1E1E1E",
#             fg="#DDDDDD",
#             insertbackground="#FFFFFF",
#             font=("Consolas", 16),
#             wrap=tk.WORD,
#             padx=16,
#             pady=16,
#         )
#         self.text.pack(fill=tk.BOTH, expand=True)

#         self.text.tag_configure("op", foreground="#40C4FF", font=("Consolas", 17, "bold"))
#         self.text.tag_configure("output", foreground="#00E676", font=("Consolas", 20, "bold"))
#         self.text.tag_configure("done", foreground="#69F0AE", font=("Consolas", 22, "bold"))
#         self.text.tag_configure("error", foreground="#FF5252", font=("Consolas", 17, "bold"))

#         self.running = True
#         self.thread = threading.Thread(target=self.read_serial, daemon=True)
#         self.thread.start()

#     def should_show(self, line):
#         line = line.strip()

#         if "[OUTPUT]" in line:
#             return "output"

#         if "INFERENCE DONE" in line or "CONGRATULATIONS" in line:
#             return "done"

#         match = tag_re.match(line)
#         if match:
#             tag = match.group(1)
#             if tag in SHOW_TAGS:
#                 return "op"

#         return None

#     def insert_line(self, line):
#         line = line.rstrip("\r\n")
#         style = self.should_show(line)

#         if style is None:
#             return

#         if style == "done":
#             self.text.insert(tk.END, "\n" + line + "\n\n", style)
#         else:
#             self.text.insert(tk.END, line + "\n", style)

#         self.text.see(tk.END)

#     def read_serial(self):
#         try:
#             with serial.Serial(PORT, BAUD, timeout=1) as ser:
#                 while self.running:
#                     raw = ser.readline()
#                     if not raw:
#                         continue

#                     line = raw.decode("utf-8", errors="replace")
#                     self.root.after(0, self.insert_line, line)

#         except Exception as e:
#             self.root.after(0, self.insert_line, f"[ERROR] {e}")

#     def stop(self):
#         self.running = False


# if __name__ == "__main__":
#     root = tk.Tk()
#     app = SerialViewer(root)

#     def on_close():
#         app.stop()
#         root.destroy()

#     root.protocol("WM_DELETE_WINDOW", on_close)
#     root.mainloop()



# import serial
# import threading
# import tkinter as tk
# from tkinter.scrolledtext import ScrolledText

# PORT = "COM5"      # Change this
# BAUD = 9600

# class SerialViewer:
#     def __init__(self, root):
#         self.root = root
#         self.root.title("Raw Serial Viewer")
#         self.root.configure(bg="#1E1E1E")

#         self.status = tk.Label(
#             root,
#             text="Starting...",
#             bg="#121212",
#             fg="#FFFFFF",
#             font=("Consolas", 20, "bold"),
#             pady=12,
#         )
#         self.status.pack(fill=tk.X)

#         test_button = tk.Button(
#             root,
#             text="Test GUI print",
#             font=("Consolas", 14, "bold"),
#             command=lambda: self.insert_line("[PY] Button works. GUI is alive."),
#         )
#         test_button.pack(fill=tk.X)

#         self.text = ScrolledText(
#             root,
#             bg="#1E1E1E",
#             fg="#DDDDDD",
#             insertbackground="#FFFFFF",
#             font=("Consolas", 16),
#             wrap=tk.NONE,
#             padx=16,
#             pady=16,
#         )
#         self.text.pack(fill=tk.BOTH, expand=True)

#         self.text.tag_configure("normal", foreground="#DDDDDD", font=("Consolas", 16))
#         self.text.tag_configure("output", foreground="#00E676", font=("Consolas", 20, "bold"))
#         self.text.tag_configure("error", foreground="#FF5252", font=("Consolas", 17, "bold"))

#         self.insert_line("[PY] GUI started.")
#         self.insert_line(f"[PY] Trying to open {PORT} @ {BAUD}...")

#         self.running = True
#         self.thread = threading.Thread(target=self.read_serial, daemon=True)
#         self.thread.start()

#     def insert_line(self, line):
#         line = str(line).rstrip("\r\n")

#         if "[ERROR]" in line:
#             tag = "error"
#         elif "[OUTPUT]" in line or "INFERENCE DONE" in line or "CONGRATULATIONS" in line:
#             tag = "output"
#         else:
#             tag = "normal"

#         self.text.insert(tk.END, line + "\n", tag)
#         self.text.see(tk.END)

#     def set_status(self, text, color="#FFFFFF"):
#         self.status.config(text=text, fg=color)

#     def read_serial(self):
#         try:
#             ser = serial.Serial(PORT, BAUD, timeout=0.2)
#             self.root.after(0, self.set_status, f"Opened {PORT} @ {BAUD}", "#00E676")
#             self.root.after(0, self.insert_line, f"[PY] Opened {PORT} @ {BAUD}")

#             while self.running:
#                 raw = ser.readline()
#                 if raw:
#                     line = raw.decode("utf-8", errors="replace")
#                     self.root.after(0, self.insert_line, line)

#             ser.close()

#         except Exception as e:
#             self.root.after(0, self.set_status, "Serial error", "#FF5252")
#             self.root.after(0, self.insert_line, f"[ERROR] {e}")

#     def stop(self):
#         self.running = False


# if __name__ == "__main__":
#     root = tk.Tk()

#     root.title("Raw Serial Viewer")
#     root.geometry("1200x750+100+100")  # width x height + x-position + y-position
#     root.lift()
#     root.attributes("-topmost", True)
#     root.after(1000, lambda: root.attributes("-topmost", False))

#     app = SerialViewer(root)

#     def on_close():
#         app.stop()
#         root.destroy()

#     root.protocol("WM_DELETE_WINDOW", on_close)
#     root.mainloop()


# import serial
# import threading
# import tkinter as tk
# from tkinter.scrolledtext import ScrolledText

# PORT = "COM5"
# BAUD = 9600


# class SerialViewer:
#     def __init__(self, root):
#         self.root = root
#         self.root.title("Raw Serial Debug Viewer")

#         # Force normal visible size and position
#         self.root.geometry("1100x700+100+100")
#         self.root.configure(bg="#1E1E1E")

#         header = tk.Label(
#             root,
#             text=f"RAW Serial Viewer | {PORT} @ {BAUD}",
#             bg="#121212",
#             fg="#FFFFFF",
#             font=("Consolas", 20, "bold"),
#             pady=12,
#         )
#         header.pack(fill=tk.X)

#         self.text = ScrolledText(
#             root,
#             bg="#1E1E1E",
#             fg="#DDDDDD",
#             insertbackground="#FFFFFF",
#             font=("Consolas", 16),
#             wrap=tk.WORD,
#             padx=16,
#             pady=16,
#         )
#         self.text.pack(fill=tk.BOTH, expand=True)

#         self.text.tag_configure(
#             "normal", foreground="#DDDDDD", font=("Consolas", 16)
#         )
#         self.text.tag_configure(
#             "error", foreground="#FF5252", font=("Consolas", 17, "bold")
#         )
#         self.text.tag_configure(
#             "status", foreground="#00E676", font=("Consolas", 17, "bold")
#         )

#         self.running = True

#         self.insert_line("[GUI] Started\n", "status")
#         self.insert_line(f"[GUI] Trying to open {PORT} at {BAUD} baud...\n", "status")

#         # Start serial thread after GUI is already drawn
#         self.root.after(300, self.start_serial_thread)

#     def force_show_window(self):
#         self.root.deiconify()
#         self.root.lift()
#         self.root.focus_force()
#         self.root.attributes("-topmost", True)
#         self.root.after(500, lambda: self.root.attributes("-topmost", False))

#     def start_serial_thread(self):
#         self.force_show_window()
#         self.thread = threading.Thread(target=self.read_serial, daemon=True)
#         self.thread.start()

#     def insert_line(self, line, style="normal"):
#         self.text.insert(tk.END, line, style)
#         self.text.see(tk.END)

#     def read_serial(self):
#         try:
#             with serial.Serial(PORT, BAUD, timeout=0.1) as ser:
#                 self.root.after(
#                     0,
#                     self.insert_line,
#                     "[GUI] Serial port opened successfully.\n\n",
#                     "status",
#                 )

#                 while self.running:
#                     raw = ser.read(256)

#                     if not raw:
#                         continue

#                     text = raw.decode("utf-8", errors="replace")
#                     self.root.after(0, self.insert_line, text, "normal")

#         except Exception as e:
#             self.root.after(0, self.insert_line, f"\n[ERROR] {e}\n", "error")

#     def stop(self):
#         self.running = False


# if __name__ == "__main__":
#     root = tk.Tk()
#     app = SerialViewer(root)

#     def on_close():
#         app.stop()
#         root.destroy()

#     root.protocol("WM_DELETE_WINDOW", on_close)

#     # Force it to appear on screen after mainloop starts
#     root.after(100, app.force_show_window)

#     root.mainloop()