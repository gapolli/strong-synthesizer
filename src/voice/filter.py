from ..engine import SynthesisOrchestrator

class StateVariableFilterWrapper:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def apply_filter_settings(self, mode: int, cutoff_hz: float, resonance_q: float):
        self.orchestrator.backend.configure_filter(mode, cutoff_hz, resonance_q)
