# 🎹 Strong Synthesizer (v0.2.5)

> **"Modular Virtual Analog, FM, and Granular Sound Synthesis Engine."**

A hybrid multi-engine software synthesizer combining low-level **C++ performance** with an asynchronous **Python workflow**. The core signal paths are rendered sample-by-sample, driven by an automated **miniaudio** backend link, and managed via direct `ctypes` bindings through a real-time **PySide6** desktop instrumentation panel.

---

## ✨ Core Synthesis Architectures

*   **Virtual Analog (VA) Engine**: Features polyphonic voice slots backed by a 3-oscillator sawtooth unison grid per voice, complete with real-time micro-detuning (MIDI CC 14) and pitch-wheel tracking.
*   **Hardware-Accurate FM Engine**: A fully expanded emulation of the classic Sega Genesis **Yamaha YM2612** chip, supporting all 8 native phase-cascade routing algorithms (0–7) and vintage `.VGM` register dump parsing.
*   **Stochastic Granular Engine**: Built for dynamic sample injection. Streams raw `.wav`/`.flac` files directly from Python into C++ memory grids, offering stochastic grain spawning, positional jitter, and linear pitch interpolation.
*   **Multi-Mode DSP Filter & Waveshaper**: Implements a highly stable Chamberlin State-Variable Filter (SVF) supporting dynamically switchable **Low-Pass (LPF)**, **High-Pass (HPF)**, and **Band-Pass (BPF)** modes. Features built-in saturation curves via an asymmetric hyperbolic tangent (\(\tanh\)) soft-clipping waveshaper with auto-gain compensation.
*   **Advanced LFO Modulator**: Features an expandable low-frequency oscillator capable of generating multiple routing waveforms (**Sine**, **Triangle**, **Sawtooth**, and **Square Pulse**) to modulate filter metrics in real-time.

---

## 🏗️ Project Architecture Blueprint

```text
.
├── CMakeLists.txt            # Unified C++/C build script (auto-fetches miniaudio)
├── LICENSE                   # MIT License legal terms
├── pyproject.toml            # Python packaging manifest and virtualenv dependencies
├── config/
│   └── presets/
│       ├── chiptune_square.toml # Pre-configured classic chiptune pulse wave tone
│       ├── genesis_bass.toml    # FM heavy hardware bass timbre
│       └── retro_lead.toml      # Unison-detuned rich analog vintage lead
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
---

## 🎛️ Execution Pipeline Reference

Use the commands below to query the system status, execute automated diagnostics, or launch the interface:

```bash
# Display the active module status matrix mapping
python3 -m src.cli info

# Trigger a standalone diagnostic tone through the console
python3 -m src.cli test-note --mode fm --note 69

# Launch the PySide6 Graphical Master Interface Dashboard
python3 -m src.bridge.gui_app

# Alternative launch via CLI package mapping alias
python3 -m src.cli gui
```

---

## 📁 Lossless Code & Performance Exporters

The synthesizer provides built-in multi-format audio exporters. Using the central master recording controller block, you can track session performance buffers and render output files using the following profiles:
*   **Universal Audio Renderers**: Export lossless session tracks into `.wav`, `.flac`, `.ogg`, or `.mp3` directly via `soundfile`.
*   **Advanced Preset Management**: Deep TOML patch serialization storing comprehensive dashboard states (filter modes, LFO types, and 4-operator ADSR matrices) to disk.
*   **Sonic Pi live-coding Exporter**: Converts active patch snapshots, algorithm choices, and cutoff values into playable, human-readable Ruby templates (`*.rb`).

---

## 🐛 Known Bugs & Structural Backlog

The following technical items are cataloged for containment in upcoming development sprints:

### 1. Filter Over-Modulation Crash (LFO Depth Out-of-Bounds)
*   **Symptom**: Setting the **LFO Depth (%)** slider to high values while using deep modulation depths can mute the audio stream or lock the oscilloscope interface.
*   **Root Cause**: The calculation `baseCutoffHz_ + (lfoValue * lfoDepth_ * 4000.0)` does not check for boundary parameters or structural symmetry. If the LFO swing forces the final frequency below `0.0 Hz` (phase inversion) or pushes it past the critical stability limit of the Chamberlin formula, the loop triggers a feedback explosion.
*   **Mitigation Strategy**: Implement a final dynamic safety mapping directly inside the parameter transmission step of `Engine::renderNextSample()` to prevent out-of-bound ranges.

---

## 🗺️ STRATEGIC DEVELOPMENT ROADMAP (Sprints Core Backlog)

This roadmap outlines the exact development track and code structures to be implemented in future engineering sprints:

### 🚀 Sprint 1: Stereo Delay & Echo Processing Module (C++ Layer)
*   **Goal**: Expand the mono rendering engine to a multi-channel pipeline.
*   **Implementation Steps**:
    1. Update `Engine::renderNextSample` to a dual output channel signature block: `void renderStereoBlock(float* leftOut, float* rightOut)`.
    2. Instantiate two circular buffers (`std::vector<float>`) inside `engine.cpp` to store 2 seconds of history for the Left and Right audio paths.
    3. Implement a Ping-Pong echo cross-feedback routing logic: route the feedback of the Left delay channel into the input of the Right delay channel, and vice versa.
    4. Expose `synth_orchestrator_configure_delay(Engine* engine, double time_ms, double feedback, double mix)` to the C-linkage interface in `bridge_bindings.cpp`.

### 🚀 Sprint 2: Extended Waveforms for Virtual Analog (Polyphonic VA)
*   **Goal**: Break the lock on the pure sawtooth oscillator grid.
*   **Implementation Steps**:
    1. Define an enum class `VAWaveform { SAW, SQUARE, TRIANGLE, SINE }` inside `engine.hpp`.
    2. Expand the `renderAnalogVA()` method to parse the selected waveform type on each sample cycle.
    3. Implement the standard mathematical formulas for the new osc profiles:
        *   *Square*: `(phase < 0.5) ? 1.0 : -1.0`
        *   *Triangle*: `(phase < 0.5) ? (4.0 * phase - 1.0) : (3.0 - 4.0 * phase)`
        *   *Sine*: `std::sin(2.0 * M_PI * phase)`
    4. Connect the routing selection dropdown menu inside `gui_app.py` to transmit the chosen waveform index down to the C++ core.

### 🚀 Sprint 3: Advanced Media IO & Quick-Button Actions (Python Bridge)
*   **Goal**: Build interface quick-buttons to streamline importing and exporting media files.
*   **Implementation Steps**:
    1. **Lossless Media Support**: Extend `synth_interface.py` with file loading functions. Use `soundfile` to read `.wav`, `.flac`, `.ogg`, and `.mp3` files, automatically downmixing multi-channel streams to mono float arrays before copying them to the `GranularEngine` memory table.
    2. **Asynchronous MIDI Sequencer**: Add a background file reader to play `.mid`/`.midi` files using a daemon Python thread, feeding note triggers directly into `self.orchestrator.note_on()`.
    3. **Preset Quick-Buttons**: Add discrete "Load TOML Patch", "Save TOML Patch", and "Export Sonic Pi Ruby" buttons to the main header row of `gui_app.py`.

### 🚀 Sprint 4: Direct Media Playback Engine (GUI Integration)
*   **Goal**: Add an on-board media player block inside the dashboard interface to audition recorded tracks or samples without requiring external software.
*   **Implementation Steps**:
    1. Create a "Recorded Wave Playback" strip inside `gui_app.py` using a dedicated group box container layout.
    2. Instantiate layout controls: "Play", "Pause", "Stop", and a progress position tracking slider node.
    3. Hook the button triggers to an asynchronous worker streaming thread in Python using `sounddevice.play()` to feed selected target files (`.wav`, `.mp3`, `.ogg`, `.flac`) safely back to the output hardware without blocking the main PySide6 window thread.

### 🚀 Sprint 5: Deep Structural Expansion of Presets (Sound Design Matrix)
*   **Goal**: Map out and design five new complex TOML files inside `config/presets/` to expand the instrument's sound bank out of the box.
*   **Implementation Steps**:
    1. Define `ambient_pad.toml`: Uses low filter cutoffs, long envelope attacks, high sustain values, and slow LFO rate modulations.
    2. Define `industrial_lead.toml`: Combines high filter drive saturation levels, deep LFO depth, and fast envelope decay lines.
    3. Define `vgm_sfx_laser.toml`: Uses custom linear algorithms in the YM2612 module with high-frequency pitch sweeping settings.
    4. Define `metallic_bell.toml`: Maps cross-modulation ratios inside the 4-operator FM engine with linear cascades.
    5. Define `distorted_unison.toml`: Couples high unison detuning amounts with maximum distortion gain inputs.

### 🚀 Sprint 6: Linux Audio Probing & Pro Architecture Integration
*   **Goal**: Integrate the project with advanced Linux audio setups like JACK and Hydrogen.
*   **Implementation Steps**:
    1. **Native JACK Connectivity**: Update the initialization flags inside `CMakeLists.txt` to discover and link against `libjack.so`. When starting the engine, pass `ma_backend_jack` to miniaudio's device initialization configuration block to let the synthesizer register directly as a system audio port.
    2. **Hydrogen Transport Synchronization**: Map an ALSA sequencer or virtual MIDI device thread inside `midi_manager.py` to listen for MIDI clock pulses. This synchronizes our stochastic granular triggers and YM2612 arpeggios with external pattern sequencers in real time.
---

## 📜 Historical Development Timeline (Original Phases & Repository Status)

### Phase 1: Technical Foundation & Environment
*   [x] Set up local and remote Git repository tracking configurations (`https://github.com`).
*   [x] Build automated cross-platform Bash environment installers (`setup.sh`, `validate_build.sh`) managing isolated virtual environments and regression test harnesses.
*   [x] Establish automated, platform-agnostic dependency fetching integrations (`miniaudio.h` acquired via native CMake network commands).

### Phase 2: Core Audio Processing Engine (C++ Layer)
*   [x] `operator.cpp`: Stable mathematical signal generation within normalized `[0.0, 1.0]` phase-accumulation spaces.
*   [x] `envelope.cpp`: Rate-scaled finite state machine updating dynamic ADSR voltage multiplier envelopes sample-by-sample.
*   [x] `ym2612_core.cpp`: Complete 4-Operator FM interlocking matrices processing all 8 vintage routing cascade algorithms natively.
*   [x] `psg_chip.cpp`: Emulation of the SN76489 chip containing 3 square-tone pulse voices and 1 multi-frequency LFSR noise channel.
*   [x] `poly_manager.cpp`: High-utility polyphonic allocation routers executing safe LRU (Least Recently Used) channel stealing routines.
*   [x] `filter.cpp`: Stable Chamberlin state-variable filter module with absolute register clamps and asymmetric saturation loops.
*   [x] `granular_engine.cpp`: Real-time stochastic grain scheduler utilizing fraction playback pointers, linear interpolation, and micro-position window deviations.
*   [x] `audio_output.cpp` / `engine.cpp`: Lock-guarded, multi-threaded synthesis gateway routing render loops directly to underlying sound card hardware.

### Phase 3: Communication Bridge & Abstraction Layers (Python Linkage)
*   [x] `bridge_bindings.cpp`: Clean, un-mangled `extern "C"` export wrapper exposing engine pointers securely via ctypes boundaries.
*   [x] `synth_interface.py`: Mid-level abstraction gateway exposing core arguments, soundfile streaming pipelines, and lossless array conversions.
*   [x] `midi_manager.py` / `gui_app.py`: Real-time asynchronous background worker loops intercepting 14-bit hardware controllers seamlessly.
*   [x] `cli.py`: Unified console query utility providing modular status checks (`typer` + `rich`).

### Phase 4: Graphical User Interface & UX Desktop Modules
*   [x] `gui_app.py`: Desktop instrument application running a unified Fusion dark theme via strict Qt6 configurations (`QPalette.ColorRole`).
*   [x] Computer qwerty keyboard layout matrix mapping and physical hardware-style slider nodes connecting parameters in real-time.
*   [x] Deep sound patch serialization engines handling structured preset save/load passes via TOML (`tomli` / `tomli_w`).

### Phase 5: Exporters & Subsystem Expansions
*   [x] `sonic_pi_gen.py`: Algorithmic text builder parsing patch variables into runnable live-coding Ruby templates.
*   [x] `wav_export.py` / `audio_device.py`: Session recording exporters mapping audio memory blocks straight into multi-format lossless files.
*   [ ] Implement dedicated native DAC channel handling (8-bit linear PCM array streaming on channel 6 within the YM2612 module).

---

## 📚 References, Inspirations, and Recommended Repositories

*   [miniaudio](https://github.com/mackron/miniaudio)
*   [Monique Monosynth](https://github.com/surge-synthesizer/monique-monosynth)
*   [Overtone](https://github.com/overtone/overtone)
*   [VGM-JS](https://github.com/apollolux/vgm-js)
*   [YM2612-JS](https://github.com/apollolux/ym2612-js)
*   [Echo](https://github.com/sikthehedgehog/Echo)
*   [Sonic-Pi](https://github.com/sonic-pi-net/sonic-pi)

---
License: MIT