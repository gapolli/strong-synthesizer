#!/usr/bin/env python3

import os
import sys
import time
import struct
import argparse
from .synth_interface import NativeYM2612Engine

def show_help():
    manual = """
NAME
    vgm_parser.py - Parse and stream classic chiptune logs directly to emulation cores

SYNOPSIS
    vgm_parser.py [OPTIONS] -i FILE.VGM

DESCRIPTION
    Decodes standard chiptune data logs (.vgm), reads timing and execution commands,
    and loops playback payloads straight into our dynamic library target interfaces.

OPTIONS
    -h, --help
        Display this manual-style help message and exit.

    -i, --input PATH
        Target chiptune command dump data file to open and parse.
"""
    print(manual)


class VgmPlayer:

    def __init__(self, engine: NativeYM2612Engine):
        self.engine = engine

    def play(self, file_path: str):
        if not os.path.exists(file_path):
            print(f"Error: Target chiptune file missing: {file_path}")
            return

        with open(file_path, "rb") as f:
            data = f.read()

        # Check for standard VGM identification tag header block
        if data[0:4] != b"Vgm ":
            print("Error: Input does not match valid uncompressed vgm format constraints.")
            return

        # Read specific payload start pointers from standard header specifications
        vgm_version = struct.unpack("<I", data[0x08:0x0C])[0]
        data_offset = struct.unpack("<I", data[0x34:0x38])[0]
        
        # Determine index location where real command payloads start
        current_idx = 0x40 if data_offset == 0 else (0x34 + data_offset)
        
        print(f"Streaming initialization: Ver: {hex(vgm_version)} Offset: {hex(current_idx)}")
        
        try:
            while current_idx < len(data):
                cmd = data[current_idx]
                
                if cmd == 0x50:  # Texas Instruments SN76489 PSG target register write command
                    psg_data = data[current_idx + 1]
                    self.engine.write_psg_direct(psg_data)
                    current_idx += 2
                elif cmd == 0x52:  # Yamaha YM2612 Port 0 data write
                    reg = data[current_idx + 1]
                    val = data[current_idx + 2]
                    self.engine.lib.synth_set_algorithm(self.engine.core, val & 0x07) # Route to algo bindings for fallback verification
                    current_idx += 3
                elif cmd == 0x61:  # Precise wait time command sample step length payload block
                    samples = struct.unpack("<H", data[current_idx + 1:current_idx + 3])[0]
                    time.sleep(samples / 44100.0)
                    current_idx += 3
                elif cmd == 0x62:  # Standard baseline 60Hz frame timing block step instruction
                    time.sleep(1.0 / 60.0)
                    current_idx += 1
                elif cmd == 0x63:  # Standard baseline 50Hz frame timing block step instruction
                    time.sleep(1.0 / 50.0)
                    current_idx += 1
                elif cmd == 0x66:  # File parsing loop termination signal block command found
                    print("Track layout termination point reached.")
                    break
                elif 0x70 <= cmd <= 0x7F:  # Short inline step waits tracking logic mapping execution loop
                    wait_samples = (cmd & 0x0F) + 1
                    time.sleep(wait_samples / 44100.0)
                    current_idx += 1
                else:
                    current_idx += 1  # Skip unmapped parameters dynamically
        except KeyboardInterrupt:
            print("\nPlayback loop halted.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-h', '--help', action='store_true')
    parser.add_argument('-i', '--input', default=None)
    args, unknown = parser.parse_known_args()

    if args.help or not args.input:
        show_help()
        sys.exit(0)

    base_dir = os.path.dirname(os.path.abspath(__file__))
    lib_path = os.path.join(base_dir, "../../build/libsynth_bridge.so")
    
    engine_instance = NativeYM2612Engine(lib_path)
    player = VgmPlayer(engine_instance)
    player.play(args.input)
