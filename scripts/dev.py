"""
Nano Coverage Godot - Editor & Game Runner

This script facilitates rapid development by launching the Godot Editor or the
Game instance for the demo project. It automatically locates the Godot executable
downloaded by the setup script.

Steps:
1.  executable Search: Locates the correct Godot binary in the project root.
2.  Editor Mode:       Launches the Godot Editor directly into the 'demo' project.
3.  Game Mode:         Runs the project as a standalone game instance.
4.  Diagnostics:       Enables verbose logging and window management flags.

Usage:
    python scripts/dev.py [--editor | --game] [--verbose]
"""

import argparse
import utils

def run_godot(args):
    godot_exe = utils.find_godot_executable()
    
    if not godot_exe:
        utils.fail("Godot executable not found. Please run 'python scripts/setup.py'")

    utils.log_step(f"Found Engine: {godot_exe}")
    
    cmd = [godot_exe, "--path", utils.GODOT_PROJECT_DIR]
    
    mode_str = "GAME"
    if args.editor:
        cmd.append("-e")
        mode_str = "EDITOR"
    
    if args.verbose:
        cmd.append("--verbose")
        
    if args.always_on_top:
        cmd.append("--always-on-top")

    utils.log_step(f"Launching {mode_str}")
    utils.run_command(cmd, check=False)

def main():
    parser = argparse.ArgumentParser(description="Run Godot Environment")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--editor", action="store_true", default=True)
    group.add_argument("--game", action="store_false", dest="editor")
    
    parser.add_argument("--verbose", action="store_true", default=True)
    parser.add_argument("--top", dest="always_on_top", action="store_true")
    
    args = parser.parse_args()
    
    utils.log_header("Nano Coverage Runner")
    run_godot(args)

if __name__ == "__main__":
    main()