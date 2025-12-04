import os
import subprocess
import sys
import glob
import argparse

def find_godot_executable():
    # Look for Godot executable in the current directory
    # Pattern matching Godot_*.exe
    exes = glob.glob("Godot_*.exe")
    if not exes:
        # Try looking for console version specifically if available, though standard exe works
        exes = glob.glob("Godot_*.console.exe")
    
    if not exes:
        print("Error: Could not find Godot executable in the root directory.")
        print("Please ensure Godot_*.exe is present.")
        sys.exit(1)
    
    return os.path.abspath(exes[0])

def run_godot(args):
    godot_exe = find_godot_executable()
    project_path = os.path.abspath("godot_project")
    
    cmd = [godot_exe, "--path", project_path]
    
    if args.editor:
        cmd.append("--editor")
    
    if args.verbose:
        cmd.append("--verbose")
        
    if args.always_on_top:
        cmd.append("--always-on-top")

    print(f"Starting Godot: {' '.join(cmd)}")
    print("--------------------------------------------------")
    
    try:
        # On Windows, using subprocess.call or run with shell=False keeps it attached to current console
        # if the exe is a console app. Standard Godot exe might detach if it's a GUI app, 
        # but usually prints to stdout if run from console.
        # To ensure we see output, we can try to capture or just let it inherit handles.
        subprocess.run(cmd, check=True)
    except KeyboardInterrupt:
        print("\nGodot process interrupted.")
    except subprocess.CalledProcessError as e:
        print(f"Godot process exited with error: {e}")

def main():
    parser = argparse.ArgumentParser(description="Run Godot Editor or Game")
    parser.add_argument("--editor", action="store_true", default=True, help="Open the Editor (default)")
    parser.add_argument("--game", action="store_false", dest="editor", help="Run the Game directly")
    parser.add_argument("--verbose", action="store_true", default=True, help="Enable verbose logging")
    parser.add_argument("--always-on-top", action="store_true", help="Keep window on top")
    
    args = parser.parse_args()
    
    run_godot(args)

if __name__ == "__main__":
    main()
