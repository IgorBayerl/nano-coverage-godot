import os
import subprocess
import sys
import glob
import argparse
import build # Import our local build script

# --- CONFIGURATION ---
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GODOT_PROJECT_DIR = os.path.join(PROJECT_ROOT, "godot_project")

# --- COLORS ---
os.system('')
GREEN = '\033[92m'
RED = '\033[91m'
RESET = '\033[0m'

def prime_godot_cache(godot_exe):
    """
    Runs Godot in headless editor mode briefly to force it to 
    scan GDExtensions and update the internal class cache (.godot folder).
    """
    print(f"{GREEN}[+] Priming Godot Cache...{RESET}")
    
    # --headless: No window
    # --editor: Runs editor logic (scans plugins/extensions)
    # --quit: Exits immediately after initialization
    cmd = [
        godot_exe,
        "--headless",
        "--path", GODOT_PROJECT_DIR,
        "--editor",
        "--quit"
    ]
    
    try:
        # We assume success if it exits with 0
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        print(f"    {GREEN}Cache updated successfully.{RESET}")
    except subprocess.CalledProcessError:
        print(f"    {RED}Warning: Cache priming exited with error. Tests might fail.{RESET}")

def find_godot_executable():
    """
    Finds the Godot executable in the project root directory.
    """
    patterns = []
    if os.name == 'nt':
        patterns = ["Godot_*.exe"]
    elif sys.platform == 'darwin':
        patterns = ["Godot.app"] 
    else:
        patterns = ["Godot_*.x86_64", "Godot_*.x86_32", "Godot_v*"]

    found = []
    for p in patterns:
        found.extend(glob.glob(os.path.join(PROJECT_ROOT, p)))

    if not found:
        print(f"{RED}[!] Godot executable not found.{RESET}")
        print("    Please run 'python setup.py' to download it.")
        sys.exit(1)

    # Pick the latest version found
    exe = sorted(found)[-1]
    
    # MacOS specific handling
    if sys.platform == 'darwin' and exe.endswith(".app"):
        exe = os.path.join(exe, "Contents", "MacOS", "Godot")
        
    return os.path.abspath(exe)

def main():
    print(f"\n{GREEN}=== Running Nano Coverage Unit Tests ==={RESET}")
    
    # 0. Ensure Debug Build (with Tests) Exists
    print(f"{GREEN}[+] Verifying Build...{RESET}")
    
    # Create a dummy arguments object to pass to build.run_scons
    class BuildArgs:
        platform = "auto"
        target = "template_debug" # Tests are auto-enabled in debug
        clean = False
        only_clean = False
        no_tests = False 
    
    try:
        # This reuses the logic in build.py. 
        # Because we updated SConstruct to use env.Clone(), 
        # this will be very fast if no files changed.
        build.run_scons(BuildArgs())
    except SystemExit:
        print(f"{RED}[!] Build Failed. Aborting tests.{RESET}")
        sys.exit(1)
    except Exception as e:
        print(f"{RED}[!] Build Exception: {e}{RESET}")
        sys.exit(1)
    
    godot_exe = find_godot_executable()
    prime_godot_cache(godot_exe)
    runner_script_path = os.path.join(GODOT_PROJECT_DIR, "addons", "run_cpp_tests.gd")
    
    # 1. Create the temporary GDScript runner
    # This script instantiates our C++ class and runs the tests
    gd_script_content = """
extends SceneTree

func _init():
    # NanoCoverageTestRunner is registered in register_types.cpp when TESTS_ENABLED is defined
    var runner = NanoCoverageTestRunner.new()
    var result = runner.run_all_tests()
    
    # Exit with status code (0 = success, 1 = failure)
    if result == 0:
        quit(0)
    else:
        quit(1)
"""
    
    try:
        with open(runner_script_path, "w") as f:
            f.write(gd_script_content)

        # 2. Construct the command
        cmd = [
            godot_exe,
            "--headless",
            "--path", GODOT_PROJECT_DIR,
            "--script", "addons/run_cpp_tests.gd"
        ]
        
        print(f"Executing: {' '.join(cmd)}\n")
        print("-" * 50)
        
        # 3. Run Godot and wait for exit
        # We allow stdout to flow directly to the console so we see GTest output in real-time
        ret_code = subprocess.call(cmd)
        print("-" * 50)

        # 4. Check results
        if ret_code != 0:
            print(f"\n{RED}❌ TESTS FAILED (Exit Code: {ret_code}){RESET}")
            sys.exit(ret_code)
        else:
            print(f"\n{GREEN}✅ ALL TESTS PASSED{RESET}")
            sys.exit(0)

    except KeyboardInterrupt:
        print("\nInterrupted by user.")
        sys.exit(1)
    finally:
        # 5. Cleanup
        if os.path.exists(runner_script_path):
            os.remove(runner_script_path)

if __name__ == "__main__":
    main()