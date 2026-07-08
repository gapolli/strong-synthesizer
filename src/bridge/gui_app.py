#!/usr/bin/env python3

import sys
import os
import argparse
import mido
import tomli_w
import numpy as np
if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QGridLayout, QLabel, QSlider, QComboBox, 
                             QGroupBox, QTextEdit, QPushButton, QFileDialog)
from PySide6.QtCore import Qt, QThread, Signal, Slot, QTimer
from PySide6.QtGui import QPainter, QPen, QColor

from .synth_interface import OrchestratedSynthesizer
from ..output.sonic_pi_gen import SonicPiCodeGenerator

class MidiListenerThread(QThread):
    message_received = Signal(object)
    def __init__(self):
        super().__init__()
        self.running = True
    def run(self):
        input_names = mido.get_input_names()
        if not input_names: return
        try:
            with mido.open_input(input_names) as inport:
                while self.running:
                    for msg in inport.iter_pending(): self.message_received.emit(msg)
                    self.msleep(5)
        except Exception: pass
    def stop(self): self.running = False


class WaveformOscilloscope(QWidget):
    def __init__(self, orchestrator: OrchestratedSynthesizer):
        super().__init__()
        self.orchestrator = orchestrator
        self.setMinimumHeight(140)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), QColor(15, 15, 20))
        pen = QPen(QColor(0, 255, 195), 2)
        painter.setPen(pen)
        data = self.orchestrator.get_scope_buffer(512)
        w, h = self.width(), self.height()
        mid_y = h // 2
        if len(data) < 2: return
        points = []
        for i in range(len(data)):
            x = int((i / len(data)) * w)
            y = int(mid_y - (data[i] * mid_y * 0.9))
            y = max(0, min(h - 1, y))
            points.append((x, y))
        for i in range(len(points) - 1):
            x1, y1 = points[i]
            x2, y2 = points[i+1]
            painter.drawLine(x1, y1, x2, y2)


class SynthDashboard(QMainWindow):

    def __init__(self, orchestrator: OrchestratedSynthesizer):
        super().__init__()
        self.orchestrator = orchestrator
        self.is_recording = False
        self.sonic_pi_gen = SonicPiCodeGenerator()
        
        self.keyboard_map = {
            Qt.Key_Z: 60, Qt.Key_S: 61, Qt.Key_X: 62, Qt.Key_D: 63, Qt.Key_C: 64,
            Qt.Key_V: 65, Qt.Key_G: 66, Qt.Key_B: 67, Qt.Key_H: 68, Qt.Key_N: 69,
            Qt.Key_J: 70, Qt.Key_M: 71, Qt.Key_Comma: 72, Qt.Key_Q: 72, Qt.Key_2: 73,
            Qt.Key_W: 74, Qt.Key_3: 75, Qt.Key_E: 76, Qt.Key_R: 77, Qt.Key_5: 78,
            Qt.Key_T: 79, Qt.Key_6: 80, Qt.Key_Y: 81, Qt.Key_7: 82, Qt.Key_U: 83, Qt.Key_I: 84
        }
        self.active_keys = set()

        self.setWindowTitle("Strong Synthesizer - Pro Custom Master Dashboard")
        self.resize(1280, 950)
        self.init_ui()
        self.start_midi_thread()
        
        self.ui_timer = QTimer()
        self.ui_timer.timeout.connect(self.scope_view.update)
        self.ui_timer.start(16)

    def init_ui(self):
        main_widget = QWidget()
        main_layout = QVBoxLayout()
        top_strip = QHBoxLayout()

        global_group = QGroupBox("Global Settings")
        global_layout = QVBoxLayout()
        self.mode_select = QComboBox()
        self.mode_select.addItems(["Virtual Analog Poly", "Hardware FM Engine (YM2612)", "Granular Engine"])
        self.mode_select.setCurrentIndex(1)
        self.mode_select.currentIndexChanged.connect(self.on_engine_mode_changed)
        global_layout.addWidget(self.mode_select)
        
        self.algo_select = QComboBox()
        for i in range(8): self.algo_select.addItem(f"Algorithm {i}")
        self.algo_select.setCurrentIndex(5)
        self.algo_select.currentIndexChanged.connect(self.on_algorithm_changed)
        global_layout.addWidget(self.algo_select)
        global_group.setLayout(global_layout)
        top_strip.addWidget(global_group)
        
        modulation_group = QGroupBox("SVF Filter, Drive & LFO Modulation Matrix")
        mod_layout = QGridLayout()
        
        mod_layout.addWidget(QLabel("Filter Topology Mode:"), 0, 0)
        self.filter_mode_select = QComboBox()
        self.filter_mode_select.addItems(["Low-Pass (LPF)", "High-Pass (HPF)", "Band-Pass (BPF)"])
        self.filter_mode_select.setCurrentIndex(0)
        self.filter_mode_select.currentIndexChanged.connect(self.on_filter_parameters_altered)
        mod_layout.addWidget(self.filter_mode_select, 0, 1)
        
        mod_layout.addWidget(QLabel("Cutoff (Hz):"), 0, 2)
        self.cutoff_slider = QSlider(Qt.Horizontal)
        self.cutoff_slider.setRange(20, 12000)
        self.cutoff_slider.setValue(2000)
        self.cutoff_slider.valueChanged.connect(self.on_filter_parameters_altered)
        mod_layout.addWidget(self.cutoff_slider, 0, 3)

        mod_layout.addWidget(QLabel("Waveshaper Drive:"), 1, 0)
        self.drive_slider = QSlider(Qt.Horizontal)
        self.drive_slider.setRange(0, 100)
        self.drive_slider.setValue(0)
        self.drive_slider.valueChanged.connect(self.on_distortion_drive_altered)
        mod_layout.addWidget(self.drive_slider, 1, 1)
        
        mod_layout.addWidget(QLabel("LFO Rate (Hz):"), 1, 2)
        self.lfo_rate = QSlider(Qt.Horizontal)
        self.lfo_rate.setRange(1, 100)
        self.lfo_rate.setValue(20)
        self.lfo_rate.valueChanged.connect(self.on_lfo_parameters_altered)
        mod_layout.addWidget(self.lfo_rate, 1, 3)
        
        mod_layout.addWidget(QLabel("LFO Depth (%):"), 2, 0)
        self.lfo_depth = QSlider(Qt.Horizontal)
        self.lfo_depth.setRange(0, 100)
        self.lfo_depth.setValue(0)
        self.lfo_depth.valueChanged.connect(self.on_lfo_parameters_altered)
        mod_layout.addWidget(self.lfo_depth, 2, 1)
        
        modulation_group.setLayout(mod_layout)
        top_strip.addWidget(modulation_group)

        rec_group = QGroupBox("Master Output & Presets IO")
        rec_layout = QVBoxLayout()
        self.rec_btn = QPushButton("Start Recording Track")
        self.rec_btn.clicked.connect(self.toggle_mastering_recording_state)
        rec_layout.addWidget(self.rec_btn)
        
        preset_layout = QHBoxLayout()
        save_btn = QPushButton("Save TOML")
        save_btn.clicked.connect(self.save_session_preset_toml)
        preset_layout.addWidget(save_btn)
        load_btn = QPushButton("Load TOML")
        load_btn.clicked.connect(self.load_session_preset_toml)
        preset_layout.addWidget(load_btn)
        rec_layout.addLayout(preset_layout)

        sonic_pi_btn = QPushButton("Export Sonic Pi Script")
        sonic_pi_btn.clicked.connect(self.export_active_as_sonic_pi_script)
        rec_layout.addWidget(sonic_pi_btn)
        
        rec_group.setLayout(rec_layout)
        top_strip.addWidget(rec_group)
        main_layout.addLayout(top_strip)

        # --- PANEL: TEXAS INSTRUMENTS SN76489 PSG CONTROL STRIP ---
        psg_box = QGroupBox("SN76489 PSG Coarse Control Node (Tone & Volume Attenuation)")
        psg_layout = QHBoxLayout()
        
        for ch in range(3):
            ch_group = QGroupBox(f"Square Tone {ch + 1}")
            ch_vbox = QVBoxLayout()
            ch_vbox.addWidget(QLabel("Attenuation (0-15):"))
            vol_slider = QSlider(Qt.Horizontal)
            vol_slider.setRange(0, 15)
            vol_slider.setValue(15)
            vol_slider.valueChanged.connect(lambda v, c=ch: self.on_psg_volume_changed(c, v))
            ch_vbox.addWidget(vol_slider)
            ch_group.setLayout(ch_vbox)
            psg_layout.addWidget(ch_group)
            
        noise_group = QGroupBox("Noise Channel 4")
        noise_vbox = QVBoxLayout()
        noise_vbox.addWidget(QLabel("Volume Attenuation:"))
        n_vol = QSlider(Qt.Horizontal)
        n_vol.setRange(0, 15)
        n_vol.setValue(15)
        n_vol.valueChanged.connect(lambda v: self.on_psg_volume_changed(3, v))
        noise_vbox.addWidget(n_vol)
        noise_group.setLayout(noise_vbox)
        psg_layout.addWidget(noise_group)
        
        psg_box.setLayout(psg_layout)
        main_layout.addWidget(psg_box)

        # --- NEW PANEL: HIGH PERFORMANCE STOCHASTIC GRANULAR DESK STRIP ---
        self.granular_box = QGroupBox("Granular Engine Parameter Node & Wave File Loader")
        gran_layout = QGridLayout()

        self.sample_load_btn = QPushButton("Load External Audio Wave Asset File")
        self.sample_load_btn.clicked.connect(self.open_granular_sample_file_dialog)
        gran_layout.addWidget(self.sample_load_btn, 0, 0, 1, 2)

        gran_layout.addWidget(QLabel("Scan Position (%):"), 1, 0)
        self.gran_pos_slider = QSlider(Qt.Horizontal)
        self.gran_pos_slider.setRange(0, 100)
        self.gran_pos_slider.setValue(50)
        self.gran_pos_slider.valueChanged.connect(self.on_granular_parameters_altered)
        gran_layout.addWidget(self.gran_pos_slider, 1, 1)

        gran_layout.addWidget(QLabel("Grain Duration (ms):"), 1, 2)
        self.gran_dur_slider = QSlider(Qt.Horizontal)
        self.gran_dur_slider.setRange(10, 500)
        self.gran_dur_slider.setValue(50)
        self.gran_dur_slider.valueChanged.connect(self.on_granular_parameters_altered)
        gran_layout.addWidget(self.gran_dur_slider, 1, 3)

        gran_layout.addWidget(QLabel("Target Density Value:"), 2, 0)
        self.gran_dens_slider = QSlider(Qt.Horizontal)
        self.gran_dens_slider.setRange(1, 32)
        self.gran_dens_slider.setValue(10)
        self.gran_dens_slider.valueChanged.connect(self.on_granular_parameters_altered)
        gran_layout.addWidget(self.gran_dens_slider, 2, 1)

        self.granular_box.setLayout(gran_layout)
        main_layout.addWidget(self.granular_box)
        self.granular_box.setEnabled(False) # Synchronized context visualization mode locking

        # --- OPERATORS MATRIX GRID CONFIGURATION FOR DETAILED SYNTHESIS ---
        operators_box = QGroupBox("4-Operator ADSR Parameter Matrix Grid")
        operators_grid = QGridLayout()
        
        for op_idx in range(4):
            op_group = QGroupBox(f"Operator {op_idx + 1}")
            op_layout = QVBoxLayout()
            
            op_layout.addWidget(QLabel("Coarse Frequency Tuning:"))
            freq_slider = QSlider(Qt.Horizontal)
            freq_slider.setRange(50, 2000)
            freq_slider.setValue(440)
            freq_slider.valueChanged.connect(lambda v, idx=op_idx: self.on_freq_adjusted(idx, v))
            op_layout.addWidget(freq_slider)
            
            adsr_grid = QGridLayout()
            stages = [('A', 'Attack', 10), ('D', 'Decay', 20), ('S', 'Sustain', 70), ('R', 'Release', 30)]
            for row, (stg, name, def_val) in enumerate(stages):
                adsr_grid.addWidget(QLabel(f"{name}:"), row, 0)
                slider = QSlider(Qt.Horizontal)
                slider.setRange(0, 100)
                slider.setValue(def_val)
                slider.valueChanged.connect(lambda v, idx=op_idx, s=stg: self.on_adsr_changed(idx, s, v))
                adsr_grid.addWidget(slider, row, 1)
                
            op_layout.addLayout(adsr_grid)
            op_group.setLayout(op_layout)
            operators_grid.addWidget(op_group, op_idx // 2, op_idx % 2)
            
        operators_box.setLayout(operators_grid)
        main_layout.addWidget(operators_box)
        
        # --- VISUALIZERS AND DIAGNOSTICS ---
        bottom_row = QHBoxLayout()
        
        scope_group = QGroupBox("Oscilloscope Visualizer (Real-time Frame Stream)")
        scope_layout = QVBoxLayout()
        self.scope_view = WaveformOscilloscope(self.orchestrator)
        scope_layout.addWidget(self.scope_view)
        scope_group.setLayout(scope_layout)
        bottom_row.addWidget(scope_group, 6)
        
        log_group = QGroupBox("Operations Logger Monitor")
        log_layout = QVBoxLayout()
        self.log_viewer = QTextEdit()
        self.log_viewer.setReadOnly(True)
        log_layout.addWidget(self.log_viewer)
        log_group.setLayout(log_layout)
        bottom_row.addWidget(log_group, 4)
        
        main_layout.addLayout(bottom_row)

        main_widget.setLayout(main_layout)
        self.setCentralWidget(main_widget)
        
        self.log_message("System Online. Play notes using your computer keyboard (Z, X, C, V... / Q, W, E, R...)!")

    def keyPressEvent(self, event):
        if event.isAutoRepeat(): return
        key = event.key()
        if key in self.keyboard_map and key not in self.active_keys:
            note = self.keyboard_map[key]
            self.active_keys.add(key)
            self.orchestrator.note_on(note, 127)
            self.log_message(f"PC Keyboard Trigger -> Note On ID: {note}")

    def keyReleaseEvent(self, event):
        if event.isAutoRepeat(): return
        key = event.key()
        if key in self.keyboard_map and key in self.active_keys:
            note = self.keyboard_map[key]
            self.active_keys.remove(key)
            self.orchestrator.note_off(note)
            self.log_message(f"PC Keyboard Trigger -> Note Off ID: {note}")

    def log_message(self, text: str):
        self.log_viewer.append(text)

    def start_midi_thread(self):
        self.midi_thread = MidiListenerThread()
        self.midi_thread.message_received.connect(self.process_async_midi)
        self.midi_thread.start()

    @Slot(object)
    def process_async_midi(self, msg):
        if msg.type == 'note_on' and msg.velocity > 0: self.orchestrator.note_on(msg.note, msg.velocity)
        elif msg.type in ['note_off', 'note_on']: self.orchestrator.note_off(msg.note)

    def on_distortion_drive_altered(self):
        drive_amount = self.drive_slider.value() / 100.0
        self.orchestrator.control_change(13, int(drive_amount * 127))
        self.log_message(f"Custom Waveshaper Drive saturation increased -> Level: {int(drive_amount * 100)}%")

    def on_engine_mode_changed(self, index: int):
        self.orchestrator.set_mode(index)
        # Context visibility mapping: Toggle granular layout controls based on selected index mode
        is_granular_active = (index == 2)
        self.granular_box.setEnabled(is_granular_active)
        self.log_message(f"Synthesis Mode target adjusted dynamically -> Mode: {index}")

    def on_algorithm_changed(self, index: int):
        self.orchestrator.control_change(1, int((index / 7.0) * 127.0))
        self.log_message(f"Global FM Routing Algorithm adjusted to Model [{index}]")

    def on_filter_parameters_altered(self):
        cutoff = float(self.cutoff_slider.value())
        selected_mode_idx = self.filter_mode_select.currentIndex()
        # Explicitly transmit selected topology index [0=LP, 1=HP, 2=BP] to C++
        self.orchestrator.configure_filter(selected_mode_idx, cutoff, 0.707)

    def on_lfo_parameters_altered(self):
        rate = self.lfo_rate.value() / 10.0
        depth = self.lfo_depth.value() / 100.0
        self.orchestrator.configure_lfo(rate, depth)

    def on_freq_adjusted(self, op_index: int, value: int):
        # Cleaned routing fallback to avoid internal structural crashes
        if op_index == 0: 
            self.orchestrator.control_change(1, int((value / 2000.0) * 127.0))
        self.log_message(f"Operator {op_index + 1} Base Tuning updated to {value} Hz")

    def on_adsr_changed(self, op_index: int, stage: str, value: int):
        normalized_value = value / 100.0
        self.orchestrator.update_operator_envelope(op_index, stage, float(normalized_value))

    def on_psg_volume_changed(self, channel_index: int, attenuation: int):
        latch_byte = 0x80 | ((channel_index & 0x03) << 5) | 0x10 | (attenuation & 0x0F)
        # Directly pass latch controls through to raw control change mappings safely
        self.orchestrator.control_change(80 + channel_index, attenuation)
        self.log_message(f"PSG Channel {channel_index + 1} Attenuation -> {attenuation} (Byte: {hex(latch_byte)})")

    def save_session_preset_toml(self):
        file_path, _ = QFileDialog.getSaveFileName(self, "Save Patch TOML", "", "Preset Files (*.toml)")
        if file_path:
            preset_payload = {"synthesis": {"mode_index": self.mode_select.currentIndex(), "cutoff_hz": self.cutoff_slider.value()}}
            with open(file_path, "wb") as f: f.write(tomli_w.dumps(preset_payload).encode("utf-8"))
            self.log_message(f"Configuration saved successfully: {file_path}")

    def load_session_preset_toml(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Load Patch TOML", "config/presets", "Preset Files (*.toml)")
        if file_path:
            try:
                with open(file_path, "rb") as f: 
                    payload = tomllib.load(f)
                
                s = payload.get("synthesis", {})
                lfo = payload.get("lfo", {})
                psg = payload.get("psg_channels", {})
                
                mode_idx = s.get("mode_index", 1)
                cutoff_hz = s.get("cutoff_hz", 2000)
                algo_idx = s.get("algorithm_index", 5)
                
                lfo_rate_val = int(lfo.get("rate_hz", 2.0) * 10)
                lfo_depth_val = int(lfo.get("depth_percent", 0.0) * 100)
                
                self.mode_select.setCurrentIndex(mode_idx)
                self.cutoff_slider.setValue(cutoff_hz)
                self.algo_select.setCurrentIndex(algo_idx)
                self.lfo_rate.setValue(lfo_rate_val)
                self.lfo_depth.setValue(lfo_depth_val)
                
                if psg:
                    psg_val = psg.get("square_1_attenuation", 15)
                    self.orchestrator.control_change(80, psg_val)
                
                info = payload.get("preset_info", {})
                patch_name = info.get("name", "Unknown Patch")
                self.log_message(f"=== Successfully Loaded Patch Profile: [{patch_name}] ===")
            except Exception as parse_error:
                self.log_message(f"Failed to process custom preset snapshot: {str(parse_error)}")

    def export_active_as_sonic_pi_script(self):
        file_path, _ = QFileDialog.getSaveFileName(self, "Export Sonic Pi Script", "", "Sonic Pi Ruby Files (*.rb)")
        if file_path:
            mode_idx = self.mode_select.currentIndex()
            cutoff = float(self.cutoff_slider.value())
            algo_idx = self.algo_select.currentIndex()
            if self.sonic_pi_gen.write_script_file(file_path, mode_idx, cutoff, algo_idx):
                self.log_message(f"Sonic Pi Live-Coding automation file exported to: {file_path}")
            else:
                self.log_message("Failed to generate Sonic Pi script file template.")

    def toggle_mastering_recording_state(self):
        if not self.is_recording:
            if self.orchestrator.start_recording():
                self.is_recording = True
                self.rec_btn.setText("Stop & Export Recording")
                self.log_message("Recording output channel pipeline capture active.")
        else:
            file_path, _ = QFileDialog.getSaveFileName(self, "Mastering Export Track", "", "Universal Audio (*.wav *.mp3 *.ogg *.flac)")
            if file_path:
                self.orchestrator.stop_recording_and_export_universal(file_path)
                self.log_message(f"Track saved successfully: {file_path}")
            self.is_recording = False
            self.rec_btn.setText("Start Recording Track")

    def open_granular_sample_file_dialog(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Select Sample Audio Asset", "", "Audio Files (*.wav *.flac)")
        if file_path:
            try:
                self.log_message(f"Loading external audio file into granular memory grid: {file_path}")
                self.orchestrator.load_wave_sample_file(file_path)
                filename = os.path.basename(file_path)
                self.sample_load_btn.setText(f"Sample Active: {filename}")
                self.log_message("Audio file loaded and normalized successfully into C++ core!")
            except Exception as e:
                self.log_message(f"Error loading wave sample into core: {str(e)}")

    def on_granular_parameters_altered(self):
        position = self.gran_pos_slider.value() / 100.0
        duration_ms = float(self.gran_dur_slider.value())
        density = self.gran_dens_slider.value()
        self.orchestrator.configure_granular(position, duration_ms, density)

    def closeEvent(self, event):
        self.ui_timer.stop()
        if hasattr(self, 'midi_thread'):
            self.midi_thread.stop()
            self.midi_thread.wait()
        super().closeEvent(event)

def main():
    parser = argparse.ArgumentParser(description="Strong Synthesizer Core UI Dashboard Manager")
    parser.add_argument('--lib', default=None, help="Explicit target path to compiled library")
    args, unknown = parser.parse_known_args()

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

    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    
    # Restructured palette definitions targeting the explicit QPalette ColorRole enum values
    from PySide6.QtGui import QPalette
    palette = app.palette()
    palette.setColor(QPalette.ColorRole.Window, QColor(25, 25, 30))
    palette.setColor(QPalette.ColorRole.WindowText, Qt.white)
    palette.setColor(QPalette.ColorRole.Base, QColor(15, 15, 20))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor(25, 25, 30))
    palette.setColor(QPalette.ColorRole.ToolTipBase, Qt.white)
    palette.setColor(QPalette.ColorRole.ToolTipText, Qt.white)
    palette.setColor(QPalette.ColorRole.Text, Qt.white)
    palette.setColor(QPalette.ColorRole.Button, QColor(45, 45, 50))
    palette.setColor(QPalette.ColorRole.ButtonText, Qt.white)
    palette.setColor(QPalette.ColorRole.BrightText, Qt.red)
    palette.setColor(QPalette.ColorRole.Link, QColor(0, 255, 195))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(0, 255, 195))
    palette.setColor(QPalette.ColorRole.HighlightedText, Qt.black)
    app.setPalette(palette)

    try:
        orchestrator_instance = OrchestratedSynthesizer(lib_path)
        window = SynthDashboard(orchestrator_instance)
        window.show()
        sys.exit(app.exec())
    except Exception as initialization_error:
        print(f"Critical error loading synthesis workspace application loop: {str(initialization_error)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
