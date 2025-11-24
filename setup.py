import subprocess
import sys
import os

def run_command(command, cwd=None):
    print(f"Running: {command}")
    try:
        subprocess.check_call(command, shell=True, cwd=cwd)
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {command}")
        sys.exit(1)

def main():
    print("=== Nano Coverage Godot Setup ===")

    # 1. Initialize Git Submodules
    print("\n[1/3] Initializing git submodules...")
    if os.path.exists(".git"):
        run_command("git submodule update --init --recursive")
    else:
        print("Not a git repository, skipping submodule update.")

    # 2. Install Python Dependencies
    print("\n[2/3] Installing Python dependencies...")
    run_command(f"{sys.executable} -m pip install scons")

    # 3. Build the Extension
    print("\n[3/3] Building the extension...")
    # Detect platform
    platform = "windows"
    if sys.platform == "linux":
        platform = "linux"
    elif sys.platform == "darwin":
        platform = "macos"
    
    run_command(f"{sys.executable} -m SCons platform={platform} target=template_debug")

    print("\n=== Setup Complete! ===")
    print("You can now run the demo project in Godot.")

if __name__ == "__main__":
    main()
