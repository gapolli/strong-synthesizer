import abc
import ctypes
import os
import soundfile as sf
import numpy as np

class OrchestratedSynthesizer:
    
    def __init__(self, lib_path: str):
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"Shared library not located at: {lib_path}")
            
        self.lib = ctypes.CDLL(lib_path)
        self._setup_bindings()
        
        # Initialize internal track data tracking structure for multi-operator states
        self.op_adsr = {
            i: {"A": 0.01, "D": 0.1, "S": 0.7, "R": 0.3} for i in range(4)
        }
        
        self.engine = self.lib.synth_create_orchestrator(ctypes.c_double(48000.0))
        self.device = self.lib.synth_orchestrator_start_audio(self.engine, ctypes.c_double(48000.0))

    def _setup_bindings(self):
        self.lib.synth_create_orchestrator.restype = ctypes.c_void_p
        self.lib.synth_create_orchestrator.argtypes = [ctypes.c_double]
        self.lib.synth_destroy_orchestrator.argtypes = [ctypes.c_void_p]
        self.lib.synth_orchestrator_set_mode.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.synth_orchestrator_note_on.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.lib.synth_orchestrator_note_off.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.synth_orchestrator_control_change.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.lib.synth_orchestrator_configure_filter.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double, ctypes.c_double]
        self.lib.synth_orchestrator_configure_granular.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double, ctypes.c_int]
        self.lib.synth_orchestrator_configure_lfo.argtypes = [ctypes.c_void_p, ctypes.c_double, ctypes.c_double]
        self.lib.synth_orchestrator_get_scope_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t]
        self.lib.synth_orchestrator_update_fm_envelope.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double, ctypes.c_double]
        
        # Global binding allocation to stream sound files into granular tables
        self.lib.synth_orchestrator_load_granular_sample.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_size_t]
        
        self.lib.synth_start_audio = self.lib.synth_orchestrator_start_audio 
        self.lib.synth_orchestrator_start_recording.restype = ctypes.c_int
        self.lib.synth_orchestrator_start_recording.argtypes = [ctypes.c_void_p]
        self.lib.synth_orchestrator_stop_recording.argtypes = [ctypes.c_void_p]
        self.lib.synth_orchestrator_get_recording_count.restype = ctypes.c_size_t
        self.lib.synth_orchestrator_get_recording_count.argtypes = [ctypes.c_void_p]
        self.lib.synth_orchestrator_get_recording_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
        
        self.lib.synth_orchestrator_start_audio.restype = ctypes.c_void_p
        self.lib.synth_orchestrator_start_audio.argtypes = [ctypes.c_void_p, ctypes.c_double]
        self.lib.synth_orchestrator_stop_audio.argtypes = [ctypes.c_void_p]

    def set_filter_drive_saturation(self, drive_normalized: float):
        raw_cc_value = int(np.clip(drive_normalized, 0.0, 1.0) * 127)
        self.control_change(13, raw_cc_value)

    def set_mode(self, mode_idx: int):
        self.lib.synth_orchestrator_set_mode(self.engine, mode_idx)

    def note_on(self, note: int, velocity: int):
        self.lib.synth_orchestrator_note_on(self.engine, note, velocity)

    def note_off(self, note: int):
        self.lib.synth_orchestrator_note_off(self.engine, note)

    def control_change(self, control_id: int, value: int):
        self.lib.synth_orchestrator_control_change(self.engine, control_id, value)

    def configure_filter(self, mode: int, cutoff: float, resonance: float):
        self.lib.synth_orchestrator_configure_filter(self.engine, mode, ctypes.c_double(cutoff), ctypes.c_double(resonance))

    def configure_granular(self, position: float, duration_ms: float, density: int):
        self.lib.synth_orchestrator_configure_granular(self.engine, ctypes.c_double(position), ctypes.c_double(duration_ms), ctypes.c_int(density))

    def configure_lfo(self, frequency_hz: float, depth_percent: float):
        self.lib.synth_orchestrator_configure_lfo(self.engine, ctypes.c_double(frequency_hz), ctypes.c_double(depth_percent))

    def get_scope_buffer(self, count: int = 512) -> np.ndarray:
        buffer_type = ctypes.c_float * count
        c_buffer = buffer_type()
        self.lib.synth_orchestrator_get_scope_data(self.engine, c_buffer, ctypes.c_size_t(count))
        return np.frombuffer(c_buffer, dtype=np.float32, count=count)

    def start_recording(self) -> bool:
        return self.lib.synth_orchestrator_start_recording(self.engine) == 1

    def stop_recording_and_export_universal(self, filename: str):
        self.lib.synth_orchestrator_stop_recording(self.engine)
        count = self.lib.synth_orchestrator_get_recording_count(self.engine)
        if count == 0: return
        buffer_type = ctypes.c_float * count
        c_buffer = buffer_type()
        self.lib.synth_orchestrator_get_recording_data(self.engine, c_buffer)
        audio_data = np.frombuffer(c_buffer, dtype=np.float32, count=count)
        ext = os.path.splitext(filename).lower()
        if ext == '.ogg': sf.write(filename, audio_data, 48000, format='OGG', subtype='VORBIS')
        elif ext == '.mp3': sf.write(filename, audio_data, 48000, format='MP3')
        elif ext == '.flac': sf.write(filename, audio_data, 48000, format='FLAC')
        else: sf.write(filename, audio_data, 48000, format='WAV', subtype='PCM_16')

    def update_operator_envelope(self, op_idx: int, stage: str, value: float):
        if op_idx in self.op_adsr and stage in self.op_adsr[op_idx]:
            self.op_adsr[op_idx][stage] = value
            p = self.op_adsr[op_idx]
            self.lib.synth_orchestrator_update_fm_envelope(
                self.engine, op_idx, 
                ctypes.c_double(p["A"]), ctypes.c_double(p["D"]), 
                ctypes.c_double(p["S"]), ctypes.c_double(p["R"])
            )

    # Audio data extractor mechanism mapping incoming audio sample vectors down to the synthesis layer
    def load_wave_sample_file(self, file_path: str):
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Target sample audio asset missing: {file_path}")
            
        data, sample_rate = sf.read(file_path, dtype='float32')
        
        # Convert stereo files to mono if needed to align with engine structures
        if len(data.shape) > 1:
            data = np.mean(data, axis=1, dtype=np.float32)
        else:
            data = data.astype(np.float32)
            
        contiguous_array = np.ascontiguousarray(data, dtype=np.float32)
        element_count = contiguous_array.size
        
        data_pointer = contiguous_array.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        self.lib.synth_orchestrator_load_granular_sample(self.engine, data_pointer, ctypes.c_size_t(element_count))

    def __del__(self):
        if hasattr(self, 'lib'):
            if hasattr(self, 'device') and self.device: self.lib.synth_orchestrator_stop_audio(self.device)
            if hasattr(self, 'engine') and self.engine: self.lib.synth_destroy_orchestrator(self.engine)
