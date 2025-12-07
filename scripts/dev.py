import os
import subprocess
import sys
import glob
import argparse

# --- CONFIGURATION ---
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GODOT_PROJECT_DIR = os.path.join(PROJECT_ROOT, "godot_project") 

# --- COLORS ---
os.system('')
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RED = '\033[91m'
RESET = '\033[0m'

def print_step(msg):    print(f"{CYAN}[+]{RESET} {msg}")
def print_error(msg):   print(f"{RED}[!] {msg}{RESET}")

def find_godot_executable():
    """
    Smart search for the Godot binary in the current root folder.
    """
    # 1. Define patterns based on OS
    patterns = []
    if os.name == 'nt':
        patterns = ["Godot_*.exe"]
    elif sys.platform == 'darwin':
        patterns = ["Godot.app"] # On Mac, this is a folder
    else:
        # Linux usually has .x86_64 or no extension
        patterns = ["Godot_*.x86_64", "Godot_*.x86_32", "Godot_v*"]

    # 2. Search
    found = []
    for p in patterns:
        found.extend(glob.glob(os.path.join(PROJECT_ROOT, p)))

    # 3. Validate
    if not found:
        print_error("Godot executable not found in root directory.")
        print(f"    Expected pattern: {patterns}")
        print("    Please run 'python setup.py' to download it.")
        sys.exit(1)

    # 4. Return the newest one (if multiple exist)
    # On Mac, we must point to the inner binary, not the .app folder
    exe = sorted(found)[-1]
    if sys.platform == 'darwin' and exe.endswith(".app"):
        exe = os.path.join(exe, "Contents", "MacOS", "Godot")
        
    return os.path.abspath(exe)

def run_godot(args):
    godot_exe = find_godot_executable()
    
    if not os.path.exists(GODOT_PROJECT_DIR):
        print_error(f"Project directory not found: {GODOT_PROJECT_DIR}")
        sys.exit(1)

    print_step(f"Found Engine: {os.path.basename(godot_exe)}")
    
    # Construct Command
    cmd = [godot_exe, "--path", GODOT_PROJECT_DIR]
    
    mode_str = "GAME"
    if args.editor:
        cmd.append("-e") # -e is shorthand for --editor
        mode_str = "EDITOR"
    
    if args.verbose:
        cmd.append("--verbose")
        
    if args.always_on_top:
        cmd.append("--always-on-top")

    print_step(f"Launching {mode_str}...")
    print(f"    {YELLOW}{' '.join(cmd)}{RESET}")
    print("--------------------------------------------------")
    
    try:
        # On Windows, Godot spawns a console. We wait for it to finish.
        subprocess.run(cmd, check=True)
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Interrupted by user.{RESET}")
    except subprocess.CalledProcessError as e:
        print_error(f"Godot crashed or exited with error code {e.returncode}")

def main():
    parser = argparse.ArgumentParser(description="Run Godot Environment")
    
    # We use mutually exclusive group so you can't be both editor and game
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--editor", action="store_true", default=True, help="Open Editor (Default)")
    group.add_argument("--game", action="store_false", dest="editor", help="Run Game directly")
    
    parser.add_argument("--verbose", action="store_true", default=True, help="Enable verbose logging")
    parser.add_argument("--top", dest="always_on_top", action="store_true", help="Keep window on top")
    
    args = parser.parse_args()
    
    print(f"\n{YELLOW}=== Nano Coverage Runner ==={RESET}")
    run_godot(args)

if __name__ == "__main__":
    main()