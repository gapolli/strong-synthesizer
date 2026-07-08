#!/usr/bin/env python3

import sys
import os
import mido
import argparse
from .synth_interface import OrchestratedSynthesizer

def show_help():
    manual = """
NAME
    midi_manager.py - Capture real-time MIDI input events and route to Engine abstractions

SYNOPSIS
    midi_manager.py [OPTIONS]

DESCRIPTION
    Monitors targeted subsystem hardware MIDI interface streams via mido and dispatches
    asynchronous note on/off payloads directly into selected polymorphic sound core blocks.

OPTIONS
    -h, --help
        Display this manual-style help message and exit.

    -p, --port NAME
        Target string identifier of the hardware MIDI device port to listen onto.
        Omitting lists all available hardware ports instead.
"""
    print(manual)

class MidiManager:
    
    def __init__(self, synth_engine: OrchestratedSynthesizer):
        self.engine = synth_engine
        self.inport = None

    def list_ports(self):
        print("Available MIDI Input Ports:")
        ports = mido.get_input_names()
        if not ports:
            print("  No hardware MIDI devices detected.")
        for port in ports:
            print(f"  - {port}")

    def start_listening(self, port_name: str):
        try:
            self.inport = mido.open_input(port_name)
            print(f"Successfully listening for MIDI inputs on port: [{port_name}]")
            for msg in self.inport:
                self.process_message(msg)
        except KeyboardInterrupt:
            print("\nMIDI Streaming Stopped By User Interaction.")
        except Exception as e:
            print(f"Error opening port: {e}")

    def process_message(self, msg: mido.Message):
        if msg.type == 'note_on':
            if msg.velocity > 0:
                self.engine.note_on(msg.note, msg.velocity)
            else:
                self.engine.note_off(msg.note)
        elif msg.type == 'note_off':
            self.engine.note_off(msg.note)
        elif msg.type == 'control_change':
            self.engine.control_change(msg.control, msg.value)
        elif msg.type == 'pitchwheel':
            # Translate 14-bit pitch range [-8192, 8191] to localized engine CC mapping 128
            normalized_pitch = int(((msg.pitch + 8192) / 16383.0) * 127.0)
            self.engine.control_change(128, normalized_pitch)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-h', '--help', action='store_true')
    parser.add_argument('-p', '--port', default=None)
    args, unknown = parser.parse_known_args()

    if args.help:
        show_help()
        sys.exit(0)

    base_dir = os.path.dirname(os.path.abspath(__file__))
    if sys.platform == "win32":
        lib_name = "synth_bridge.dll"
    elif sys.platform == "darwin":
        lib_name = "libsynth_bridge.dylib"
    else:
        lib_name = "libsynth_bridge.so"
    lib_path = os.path.join(base_dir, "../../build", lib_name)
    
    try:
        engine = OrchestratedSynthesizer(lib_path)
        manager = MidiManager(engine)
    except Exception as e:
        print(f"Initialization failure: {e}")
        sys.exit(1)

    if not args.port:
        manager.list_ports()
        print("\nRe-run script specifying active hardware target: --port \"<name>\"")
    else:
        manager.start_listening(args.port)
