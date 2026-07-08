#!/usr/bin/env python3

import sys
import typer
from rich.console import Console
from rich.table import Table

from .engine import SynthesisOrchestrator

console = Console()
app = typer.Typer(
    name="synth",
    help="Strong Synthesizer - Unified Sound Engine CLI Command Interface",
    no_args_is_help=True
)

@app.command()
def info():
    """Display synthesis workspace property specs."""
    table = Table(title="[bold blue]Strong Synthesizer Specs[/]")
    table.add_column("Module Component Architecture", style="cyan")
    table.add_column("Status Mapping", style="green")
    
    table.add_row("Virtual Analog Engine", "Active (Polyphonic Unison Grid)")
    table.add_row("FM Synthesizer (YM2612)", "Active (Hardware Accurate Routing Mode)")
    table.add_row("Granular Engine Layer", "Active (High-Performance DSP Look-Up)")
    table.add_row("Universal Code Exporters", "Enabled (WAV, MP3, OGG, FLAC, SonicPi)")
    
    console.print(table)

@app.command()
def gui():
    """Launch the real-time PySide6 Master GUI Dashboard."""
    console.print("[bold green]🚀 Initializing Graphical Dashboard Application Loop...[/]")
    from .bridge import gui_app
    gui_app.main()

@app.command()
def test_note(
    mode: str = typer.Option("fm", "--mode", "-m", help="Engine selection profile target"),
    note: int = typer.Option(69, "--note", "-n", help="Target MIDI note integer parameter value")
):
    """Trigger a clean diagnostic tone verification sequence."""
    console.print(f"[bold blue]🎵 Bootstrapping engine component under mode: [{mode}]...[/]")
    try:
        orchestrator = SynthesisOrchestrator()
        orchestrator.select_engine_mode(mode)
        
        console.print(f"Triggering Note On -> ID: {note}")
        orchestrator.trigger_note_on(note, 127)
        import time
        time.sleep(1.5)
        
        console.print("Triggering Note Off")
        orchestrator.trigger_note_off(note)
        time.sleep(0.5)
        console.print("[bold green]✅ Tone test complete.[/]")
    except Exception as e:
        console.print(f"[bold red]❌ Error: {e}[/]")
        raise typer.Exit(code=1)

def main():
    app()

if __name__ == "__main__":
    main()
