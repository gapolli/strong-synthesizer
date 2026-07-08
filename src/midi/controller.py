import mido
from ..engine import SynthesisOrchestrator

class MidiControllerInterface:

    def __init__(self, orchestrator: SynthesisOrchestrator):
        self.orchestrator = orchestrator

    def process_incoming_midi_message(self, msg: mido.Message):
        if msg.type == 'note_on' and msg.velocity > 0:
            self.orchestrator.trigger_note_on(msg.note, msg.velocity)
        elif msg.type in ['note_off', 'note_on']:
            self.orchestrator.trigger_note_off(msg.note)
        elif msg.type == 'control_change':
            self.orchestrator.route_control_change(msg.control, msg.value)
