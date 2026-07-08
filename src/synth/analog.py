from ..engine import SynthesisOrchestrator

class VirtualAnalogEngine:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def activate(self):
        self.orchestrator.select_engine_mode("analog")
