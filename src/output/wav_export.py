from ..engine import SynthesisOrchestrator

class UniversalAudioFileWriter:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def start_track_capture(self):
        self.orchestrator.backend.start_recording()

    def finalize_and_export(self, target_filepath: str):
        self.orchestrator.backend.stop_recording_and_export_universal(target_filepath)
