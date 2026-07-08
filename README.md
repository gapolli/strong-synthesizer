# 🎹 Strong Synthesizer

> **"Modular Virtual Analog, FM, and Granular Sound Synthesis Engine."**

A hybrid multi-engine software synthesizer combining low-level **C++ performance** with an asynchronous **Python workflow**. The core signal paths are rendered sample-by-sample, driven by an automated **miniaudio** backend link, and managed via direct `ctypes` bindings through a real-time **PySide6** desktop instrumentation panel.

---

## ✨ Core Synthesis Architectures

*   **Virtual Analog (VA) Engine**: Features polyphonic voice slots backed by a 3-oscillator sawtooth unison grid per voice, complete with real-time micro-detuning (MIDI CC 14) and pitch-wheel tracking.
*   **Hardware-Accurate FM Engine**: A fully expanded emulation of the classic Sega Genesis **Yamaha YM2612** chip, supporting all 8 native phase-cascade routing algorithms (0–7) and vintage `.VGM` register dump parsing.
*   **Stochastic Granular Engine**: Built for dynamic sample injection. Streams raw `.wav`/`.flac` files directly from Python into C++ memory grids, offering stochastic grain spawning, positional jitter, and linear pitch interpolation.
*   **DSP Modulation & Waveshaping**: Implements a Chamberlin state-variable filter (SVF) with non-linear asymmetric saturation (\(\tanh\) soft-clipping), auto-gain compensation, and an independent low-frequency oscillator (LFO).

---

## 🏗️ Project Architecture Blueprint

```text
.
├── CMakeLists.txt            # Unified C++/C build script (auto-fetches miniaudio)
├── pyproject.toml            # Python packaging manifest and virtualenv dependencies
├── scripts/
│   ├── setup.sh              # Environment and folder workspace initialization utility
│   └── validate_build.sh     # Compiler validation script with automated ctypes smoke tests
└── src/
    ├── bridge/
    │   ├── bridge_bindings.cpp   # C-Linkage exports (`extern "C"`) bridge layer
    │   ├── gui_app.py            # Master desktop dashboard panel (PySide6 UI + MIDI Threads)
    │   └── synth_interface.py    # Python abstraction wrapper handling dynamic buffer injection
    ├── core/
    │   ├── include/          # Subsystem C++ headers (engine, granular, filter, ym2612)
    │   └── source/           # Subsystem C++ implementations (render loops, dsp)
    └── output/
        └── sonic_pi_gen.py   # Live-coding Ruby script template exporter engine
```

---

## 🚀 Quick Start & Environment Verification

Execute this exact sequence of commands from the project root directory to prepare your workspace, build all native components, and launch the graphical dashboard.

### 1. Initialize the Workspace and Python Environment
This creates the project layout and installs all dependencies (including `PySide6`, `mido`, `soundfile`, and `python-rtmidi`) into an isolated virtual environment:
```bash
chmod +x scripts/setup.sh scripts/validate_build.sh
./scripts/setup.sh --force
source .venv/bin/activate
```

### 2. Run Automated C++ Compilation and Binding Tests
The CMake engine automatically downloads the official single-header `miniaudio.h` repository from source, compiles the static core engine alongside the shared object bridge (`libsynth_bridge.so`), and executes an integration test:
```bash
./scripts/validate_build.sh -c --run-python
```

### 3. Launch the Master Graphical UI Dashboard
Launches the fully integrated dark-themed orchestration desk interface. You can play audio notes using your computer keyboard (`Z, X, C, V...` and `Q, W, E, R...`), view output waves inside the real-time oscilloscope, load audio files into granular memory, and capture performances:
```bash
python3 -m src.bridge.gui_app
```

### 4. Alternative: Run the Unified CLI Panel Queries
```bash
# Display the active module status matrix mapping
python3 -m src.cli info

# Trigger an standalone diagnostic tone through the console
python3 -m src.cli test-note --mode fm --note 69
```

---

## 📁 Lossless Code & Performance Exporters

The synthesizer provides built-in multi-format audio exporters. Using the central master recording controller block, you can track session performance buffers and render output files using the following profiles:
*   **Universal Audio Renderers**: Export lossless session tracks into `.wav`, `.flac`, `.ogg`, or `.mp3` directly via `soundfile`.
*   **Sonic Pi live-coding Exporter**: Converts active patch snapshots, algorithm choices, and cutoff values into playable, human-readable Ruby templates (`*.rb`).

---
License: MIT
