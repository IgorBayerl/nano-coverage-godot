import os
import subprocess
import sys
import shutil
import platform

def run_command(command, cwd=None, check=True):
    print(f"Running: {' '.join(command)}")
    try:
        subprocess.run(command, cwd=cwd, check=check, shell=(os.name == 'nt'))
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {e}")
        sys.exit(1)

def check_python_dependencies():
    print("Checking Python dependencies...")
    required = ["scons"]
    installed = []
    
    # Check if scons is in path
    if shutil.which("scons"):
        print("  Found scons in PATH")
    else:
        print("  scons not found in PATH, checking pip...")
        try:
            import SCons
            print("  Found scons via pip")
        except ImportError:
            print("  scons not found. Installing...")
            run_command([sys.executable, "-m", "pip", "install", "scons"])

def check_compiler():
    print("Checking for C++ compiler...")
    if os.name == 'nt':
        # Windows
        # Check for MSVC (cl.exe) or MinGW (g++)
        if shutil.which("cl"):
            print("  Found MSVC (cl.exe)")
        elif shutil.which("g++"):
            print("  Found MinGW (g++)")
        else:
            print("Warning: No C++ compiler found in PATH.")
            print("  Please ensure you have Visual Studio (with C++ workload) or MinGW installed.")
            print("  If you have Visual Studio, run this script from the 'Developer Command Prompt'.")
    else:
        # Linux/Mac
        if shutil.which("g++") or shutil.which("clang++"):
            print("  Found C++ compiler")
        else:
            print("Warning: No C++ compiler found.")
            print("  Please install gcc or clang (e.g., sudo apt install build-essential).")

def init_submodules():
    print("Initializing git submodules...")
    # Sync first to ensure URLs are correct
    run_command(["git", "submodule", "sync"])
    run_command(["git", "submodule", "update", "--init", "--recursive"])

def main():
    print(f"=== Nano Coverage Godot Setup ({platform.system()}) ===")
    
    # 1. Initialize Submodules
    init_submodules()

    # 2. Check Python Dependencies (SCons)
    check_python_dependencies()

    # 3. Check Compiler
    check_compiler()

    print("\n=== Setup Complete ===")
    print("You can now run 'python build.py' to build the project.")

if __name__ == "__main__":
    main()
