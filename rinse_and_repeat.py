#!/usr/bin/env python3
import subprocess
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

CMD_FILE = "command_to_repeat.txt"

class SourceChangeHandler(FileSystemEventHandler):
    def on_modified(self, event):
        if not event.is_directory:
            with open(CMD_FILE, 'r') as f:
                cmd = f.read().strip()
            if cmd:
                print(f"\n🔄 File changed: {event.src_path}")
                print(f"Running: {cmd}\n")
                subprocess.run(cmd, shell=True)

# Ensure command file exists
open(CMD_FILE, 'a').close()

observer = Observer()
observer.schedule(SourceChangeHandler(), path='src', recursive=True)
observer.start()
print("👀 Watching src/ for changes... (Press Ctrl+C to stop)")

try:
    observer.join()
except KeyboardInterrupt:
    print("\n✋ Stopped watching")
    observer.stop()
    observer.join()