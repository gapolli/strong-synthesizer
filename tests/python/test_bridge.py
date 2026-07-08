#!/usr/bin/env python3

import sys
import os
import ctypes
import time
import argparse

def show_help():
    manual = """
NAME
    test_bridge.py - Verify C++ orchestration core integration via native Python ctypes

SYNOPSIS
    test_bridge.py [OPTIONS]

DESCRIPTION
    Loads the compiled shared object/dynamic library binary and runs an automated
    audio test patch executing engine mode configuration and orchestrator triggers.

OPTIONS
    -h, --help
        Display this manual-style help message and exit.

    -l, --lib PATH
        Explicitly path target the shared dynamic library file.
        Default looks in '../../build/libsynth_bridge.so'
"""
    print(manual)

def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-h', '--help', action='store_true')
    parser.add_argument('-l', '--lib', default=None)
    args, unknown = parser.parse_known_args()

    if args.help:
        show_help()
        return

    lib_path = args.lib
    if not lib_path:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        if sys.platform == "win32":
            lib_name = "synth_bridge.dll"
        elif sys.platform == "darwin":
            lib_name = "libsynth_bridge.dylib"
        else:
            lib_name = "libsynth_bridge.so"
        lib_path = os.path.join(base_dir, "../../build", lib_name)

    if not os.path.exists(lib_path):
        print(f"Error: Shared library binary not found at target: {lib_path}")
        sys.exit(1)

    synth = ctypes.CDLL(lib_path)

    # Configuração das assinaturas do Orquestrador Central
    synth.synth_create_orchestrator.restype = ctypes.c_void_p
    synth.synth_create_orchestrator.argtypes = [ctypes.c_double]
    
    synth.synth_destroy_orchestrator.argtypes = [ctypes.c_void_p]
    
    synth.synth_orchestrator_set_mode.argtypes = [ctypes.c_void_p, ctypes.c_int]
    synth.synth_orchestrator_note_on.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
    synth.synth_orchestrator_note_off.argtypes = [ctypes.c_void_p, ctypes.c_int]
    synth.synth_orchestrator_control_change.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
    
    synth.synth_orchestrator_start_audio.restype = ctypes.c_void_p
    synth.synth_orchestrator_start_audio.argtypes = [ctypes.c_void_p, ctypes.c_double]
    synth.synth_orchestrator_stop_audio.argtypes = [ctypes.c_void_p]

    print("=== Constructing Engine Orchestrator via ctypes ===")
    engine = synth.synth_create_orchestrator(ctypes.c_double(48000.0))

    # Define o modo inicial para FM (Modo 1 no Enum)
    synth.synth_orchestrator_set_mode(engine, 1)

    print("=== Initializing Real-time Orchestrated Stream Driver ===")
    device = synth.synth_orchestrator_start_audio(engine, ctypes.c_double(48000.0))
    if not device:
        print("Error: Audio driver failed initialization runtime.")
        synth.synth_destroy_orchestrator(engine)
        sys.exit(1)

    print("=== Audio Active: Triggering Note Gate On ===")
    synth.synth_orchestrator_note_on(engine, 69, 127) # Nota Lá 440Hz
    time.sleep(1.5)

    print("=== Triggering Note Gate Off ===")
    synth.synth_orchestrator_note_off(engine, 69)
    time.sleep(0.5)

    print("=== Terminating Engine Allocation ===")
    synth.synth_orchestrator_stop_audio(device)
    synth.synth_destroy_orchestrator(engine)
    print("=== Python Linkage Bridge Passed Successfully ===")

if __name__ == "__main__":
    main()
