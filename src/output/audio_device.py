from ..engine import SynthesisOrchestrator

class AudioDeviceAbstraction:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator
        # The real-time playback thread is bound to our miniaudio C++ driver context

    def get_sample_rate(self) -> int:
        return 48000
