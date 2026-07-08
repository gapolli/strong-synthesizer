from ..engine import SynthesisOrchestrator

class SharedADSREnvelope:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def update_operator_parameters(self, op_idx: int, attack: float, decay: float, sustain: float, release: float):
        # Routes properties directly down to the shared C++ envelope structures
        self.orchestrator.backend.lib.synth_set_operator_envelope(
            self.orchestrator.backend.engine, op_idx, attack, decay, sustain, release
        )
