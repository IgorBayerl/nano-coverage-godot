import os
import subprocess
import sys
import argparse
import shutil
import multiprocessing

# --- CONFIGURATION ---
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
# Assumes 'native' is where your SConstruct file lives
NATIVE_DIR = os.path.join(PROJECT_ROOT, "native") 

# --- COLORS ---
os.system('') # Enable ANSI
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RED = '\033[91m'
RESET = '\033[0m'

def print_step(msg):      print(f"{CYAN}[+]{RESET} {msg}")
def print_substep(msg):   print(f"    {msg}")
def print_error(msg):     print(f"{RED}[!] {msg}{RESET}")
def print_success(msg):   print(f"{GREEN}[✓] {msg}{RESET}")

def run_scons(args):
    print_step("Preparing Build...")

    # 1. Construct Base Command (Always use python module for reliability)
    cmd = [sys.executable, "-m", "SCons"]
    
    # 2. Parallel Builds (Use all cores)
    cpu_count = multiprocessing.cpu_count()
    cmd.append(f"-j{cpu_count}")
    print_substep(f"Using {cpu_count} CPU threads")

    # 3. Handle Clean
    if args.clean:
        print_step("Cleaning build targets...")
        subprocess.run(cmd + ["-c"], cwd=NATIVE_DIR, shell=(os.name == 'nt'))
        print_success("Clean complete.")
        if args.only_clean:
            return

    # 4. Platform & Target
    if args.platform != "auto":
        cmd.append(f"platform={args.platform}")
    
    cmd.append(f"target={args.target}")

    # 5. Smart MinGW Detection (Windows Only)
    # If we are on Windows, have G++, but NO MSVC (cl.exe), force MinGW.
    if os.name == 'nt':
        has_gpp = shutil.which("g++") is not None
        has_msvc = shutil.which("cl") is not None
        
        if has_gpp and not has_msvc:
            print_substep(f"{YELLOW}Auto-detected MinGW (no MSVC found). Adding use_mingw=yes{RESET}")
            cmd.append("use_mingw=yes")

    # 6. Debug Symbols
    if args.target == "template_debug":
        cmd.append("debug_symbols=yes")

    # 7. Execute
    print_step(f"Compiling in {os.path.basename(NATIVE_DIR)}...")
    # Print the clean command for the user to see
    display_cmd = " ".join(cmd)
    print_substep(f"{YELLOW}> {display_cmd}{RESET}")

    try:
        # We don't capture output here so the user sees the real-time compilation log
        subprocess.run(cmd, cwd=NATIVE_DIR, check=True, shell=(os.name == 'nt'))
        print_success("Build Successful!")
    except subprocess.CalledProcessError:
        print_error("Build Failed.")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Build Manager")
    parser.add_argument("--platform", choices=["windows", "linux", "macos", "android", "ios", "auto"], default="auto", help="Target platform")
    parser.add_argument("--target", choices=["template_debug", "template_release"], default="template_debug", help="Build target")
    parser.add_argument("--clean", action="store_true", help="Clean before building")
    parser.add_argument("--only-clean", action="store_true", help="Clean and exit")
    
    args = parser.parse_args()
    
    # Header
    print(f"\n{YELLOW}=== Nano Coverage Builder ==={RESET}")
    print_substep(f"Target:   {args.target}")
    print_substep(f"Platform: {args.platform}\n")
    
    run_scons(args)

if __name__ == "__main__":
    main()