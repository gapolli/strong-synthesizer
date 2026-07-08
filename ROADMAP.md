# ROADMAP.md

Strategic development logs and architectural spec maps for the Strong Synthesizer framework. This document functions as an internal project state tracking matrix.

---

## 📌 Repository Core Infrastructure Status (v0.2.5) - ARCHITECTURE REINFORCED

The codebase is compiled-stable, robust against extreme numerical ranges, and verified against floating-point saturation explosions.

### Completed Infrastructure Fixes & Enhancements
- **Ultimate SVF Feedback Safeguards**: Resolved Chamberlin topology instability loops at high cutoff/resonance spikes. Added hard register clipping bounds (`-4.0` to `4.0`) inside `filter.cpp` alongside real-time `NaN`/`inf` state purgers to prevent audio muting.
- **Oscilloscope Overflow Sanitization**: Upgraded the PySide6 painter pipeline with vector data filters (`np.nan_to_num` + `np.clip`), eliminating numerical `OverflowError` and graph pipeline segmentation crashes.
- **UI Race Condition Mitigation**: Deferred real-time voice grid indicator callbacks using explicit guard checks (`hasattr`) to guarantee proper layout rendering order before memory allocations.

---

## 🚀 Engine Core Subsystem Progress Matrix

### 1. Virtual Analog Engine Layer
- [x] Monophonic basic waveform tables (Sine, Square, Sawtooth, Triangle).
- [x] Polyphonic allocation slot maps managing parallel rendering streams.
- [x] **Unison Detune Architecture**: Implemented a 3-oscillator sawtooth detuning grid per voice socket with customizable width parameters via MIDI CC ID 14.
- [x] **Dynamic Pitch Bend Engine**: Integrated real-time pitch-wheel tracking calculations into voice increments supporting standard +/- 2 semi-tone adjustments.

### 2. FM Synthesizer (YM2612 Hardware Accurate Emulation)
- [x] Linear phase accumulation loops operating within normalized `[0.0, 1.0]` phase domains.
- [x] LRU (Least Recently Used) polyphonic channel tracking supporting parallel rendering passes.
- [x] Complete command packet parser for vintage Sega Genesis `.VGM` register dumps.
- [x] **Full 8-Algorithm Expansion Matrix**: Fully expanded the phase cascade routing systems to process all native Yamaha YM2612 algorithms (0 through 7) down to single sample rendering tiers.

### 3. Granular Sound Engine Layer
- [x] Pre-allocated static fallback memory buffer grids.
- [x] Probabilistic grain allocation loop based on random density distributions.
- [x] **Dynamic Sample Injection API**: Added direct memory load bindings for loading external files via Python into the C++ runtime.
- [x] **Stochastic Pitch Interlocking**: Added linear interpolation modules, micro-position deviation jitter, and playback speed variations.
- [x] **Alternate Contour Windows**: Integrated smooth **Gaussian envelope smoothing models** running alongside standard Hann curves to expand tonal possibilities.

### 4. Multi-Mode DSP Modulators & Filters
- [x] Chamberlin state-variable audio filter topology running Low-Pass mode.
- [x] **High-Pass (HPF) and Band-Pass (BPF) Modes**: Successfully implemented the remaining discrete filter topologies with live selection triggers via the dashboard.
- [x] Non-linear asymmetric saturation stages with automatic gain compensation.
- [x] **Multi-Waveform LFO Engine**: Expanded the low-frequency modulator to support real-time generation of **Sine**, **Triangle**, **Sawtooth**, and **Square Pulse** modulation curves.

### 5. Advanced Dashboards & Exporters Bridges
- [x] **Vector ADSR Contour Plotter**: Built a custom dynamic drawing engine tracking and rendering envelope geometries on the screen.
- [x] **Polyphonic Active Voice Trackers**: Real-time status light grid showing active voice allocation loops instantaneously.
- [x] **Sonic Pi Code Generator**: Completed Python file output class formatting workspace models into runnable live-coding Ruby templates.
- [x] **Universal Lossless Export Tracker**: Multi-format session file saving layer supporting explicit WAV, MP3, FLAC, and OGG stream dumps via `soundfile`.
- [x] **Deep Patch Preset Manager**: Full serialization pipeline saving and loading comprehensive sound profiles via TOML files.

---

## 🛠️ Verification Command Pipeline Reference

Run this exact sequence of terminal operations from the project root to verify project operations:

```bash
# Phase 1: Environment Workspace Re-Initialization
./scripts/setup.sh --force
source .venv/bin/activate

# Phase 2: Automated C++ Target Compilation & Ctypes Verification
./scripts/validate_build.sh -c --run-python

# Phase 3: Launch Graphical Master Interface Panel
python3 -m src.bridge.gui_app
```
