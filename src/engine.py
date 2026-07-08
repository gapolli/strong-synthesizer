import os
import sys
import struct
import time
import mido
if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

from .bridge.synth_interface import OrchestratedSynthesizer

class SynthesisOrchestrator:

    def __init__(self):
        base_dir = os.path.dirname(os.path.abspath(__file__))
        lib_path = os.path.join(base_dir, "../build/libsynth_bridge.so")
        
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"C++ core engine shared library missing: {lib_path}")
            
        self.backend = OrchestratedSynthesizer(lib_path)
        self.current_mode = "fm"

    def select_engine_mode(self, mode: str):
        modes = {"analog": 0, "fm": 1, "granular": 2}
        target_mode = mode.lower()
        if target_mode in modes:
            self.current_mode = target_mode
            self.backend.set_mode(modes[target_mode])

    def trigger_note_on(self, note: int, velocity: int):
        self.backend.note_on(note, velocity)

    def trigger_note_off(self, note: int):
        self.backend.note_off(note)

    def route_control_change(self, control_id: int, value: int):
        self.backend.control_change(control_id, value)

    def import_preset_toml(self, filepath: str) -> dict:
        """Imports and applies a full synthesizer state patch configuration."""
        with open(filepath, "rb") as f:
            config = tomllib.load(f)
        
        if "synthesis" in config:
            s = config["synthesis"]
            if "mode" in s: self.select_engine_mode(s["mode"])
            if "cutoff_hz" in s and "resonance_q" in s:
                self.backend.configure_filter(0, float(s["cutoff_hz"]), float(s["resonance_q"]))
        return config

    def import_midi_file(self, filepath: str, progress_callback=None):
        """Parses and plays a standard MIDI sequence file asynchronously."""
        mid = mido.MidiFile(filepath)
        for msg in mid.play():
            if progress_callback and not progress_callback():
                break # Allow UI thread interrupt hooks
            if msg.type == 'note_on' and msg.velocity > 0:
                self.trigger_note_on(msg.note, msg.velocity)
            elif msg.type in ['note_off', 'note_on']:
                self.trigger_note_off(msg.note)
            elif msg.type == 'control_change':
                self.route_control_change(msg.control, msg.value)

    def import_vgm_file(self, filepath: str, progress_callback=None):
        """Parses and streams vintage Sega Genesis hardware register dumps."""
        with open(filepath, "rb") as f:
            data = f.read()

        if data[0:4] != b"Vgm ":
            raise ValueError("Invalid format: Not an uncompressed VGM data log.")

        data_offset = struct.unpack("<I", data[0x34:0x38])[0]
        current_idx = 0x40 if data_offset == 0 else (0x34 + data_offset)

        while current_idx < len(data):
            if progress_callback and not progress_callback():
                break
            cmd = data[current_idx]
            
            if cmd == 0x50:  # SN76489 PSG target register write command
                self.backend.lib.synth_write_psg(self.backend.engine, data[current_idx + 1])
                current_idx += 2
            elif cmd == 0x52:  # YM2612 Port 0 Write
                val = data[current_idx + 2]
                self.backend.set_mode(1) # Force FM mode transition to mirror file constraints
                self.backend.lib.synth_set_algorithm(self.backend.engine, val & 0x07)
                current_idx += 3
            elif cmd == 0x61:  # Precise sample wait time payload
                samples = struct.unpack("<H", data[current_idx + 1:current_idx + 3])[0]
                time.sleep(samples / 44100.0)
                current_idx += 3
            elif cmd == 0x62:  # 60Hz standard wait command step
                time.sleep(1.0 / 60.0)
                current_idx += 1
            elif cmd == 0x66:  # Termination signal
                break
            elif 0x70 <= cmd <= 0x7F:  # Short inline sample ticks waits tracking
                time.sleep(((cmd & 0x0F) + 1) / 44100.0)
                current_idx += 1
            else:
                current_idx += 1
