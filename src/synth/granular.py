from ..engine import SynthesisOrchestrator

class GranularSynthesisEngine:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def activate(self):
        self.orchestrator.select_engine_mode("granular")

    def configure_grain_parameters(self, position: float, duration_ms: float, density: int):
        self.orchestrator.backend.configure_granular(position, duration_ms, density)
