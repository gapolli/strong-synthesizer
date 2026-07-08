import os

class SonicPiCodeGenerator:

    def __init__(self):
        pass

    def generate_studio_code_block(self, mode_name: str, cutoff_normalized: float) -> str:
        """Compiles raw studio parameters into native Sonic Pi synth instructions."""
        hz_value = int(cutoff_normalized * 12000.0)
        
        if "FM" in mode_name:
            target_synth = "fm"
        elif "Granular" in mode_name:
            target_synth = "gloop"
        else:
            target_synth = "saw"

        code_template = f"""# 🎹 Strong Synthesizer Automated Export Session Patch File
# Generated live tracking state models from workspace dashboards

use_bpm 120

live_loop :strong_synth_auto_stream do
  use_synth :{target_synth}
  
  # Automated Master DSP Filter Modulations
  with_fx :lpf, cutoff: {hz_value} do
    # Stream structural arpeggiations
    play_chord [:e3, :g3, :b3, :d4], attack: 0.05, release: 0.4, amp: 0.7
    sleep 1
    
    play_chord [:c3, :e3, :g3, :b3], attack: 0.1, release: 0.6, amp: 0.6
    sleep 1
  end
end
"""
        return code_template

    def write_script_file(self, target_filepath: str, mode_index: int, cutoff_hz: float, algorithm_index: int) -> bool:
        """Formats and outputs structural snapshot parameters into files securely."""
        try:
            mode_strings = ["Virtual Analog Poly", "Hardware FM Engine (YM2612)", "Granular Engine"]
            selected_mode = mode_strings[mode_index] if mode_index < len(mode_strings) else "Unknown"
            
            cutoff_norm = cutoff_hz / 12000.0
            raw_script_text = self.generate_studio_code_block(selected_mode, cutoff_norm)
            
            raw_script_text += f"\n# Architectural Hardware Meta Tracking Properties\n"
            raw_script_text += f"# Selected Engine Profile: {selected_mode}\n"
            raw_script_text += f"# Core Cutoff Frequency Metric: {int(cutoff_hz)} Hz\n"
            raw_script_text += f"# Target FM Modulator Matrix Code Algorithm: {algorithm_index}\n"

            with open(target_filepath, "w", encoding="utf-8") as target_file:
                target_file.write(raw_script_text)
            return True
        except Exception:
            return False
