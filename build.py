import os
import subprocess
import sys
import argparse
import shutil

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
NATIVE_DIR = os.path.join(PROJECT_ROOT, "native")

def run_scons(args):
    """
    Runs scons in the native directory.
    """
    # Try to find scons executable, otherwise run as python module
    if shutil.which("scons"):
        cmd = ["scons"]
    else:
        cmd = [sys.executable, "-m", "SCons"]
    
    # Platform (auto-detect if not specified, but scons usually handles this)
    if args.platform != "auto":
        cmd.append(f"platform={args.platform}")
    
    # Target
    cmd.append(f"target={args.target}")

    # Clean
    if args.clean:
        print("Cleaning build...")
        clean_cmd = cmd + ["-c"]
        subprocess.run(clean_cmd, cwd=NATIVE_DIR, shell=(os.name == 'nt'))
        return # Stop here if cleaning
        
    # Parallel build
    import multiprocessing
    cmd.append(f"-j{multiprocessing.cpu_count()}")

    print(f"Building in {NATIVE_DIR}...")
    print(f"Command: {' '.join(cmd)}")
    
    try:
        subprocess.run(cmd, cwd=NATIVE_DIR, check=True, shell=(os.name == 'nt'))
        print("\nBuild successful!")
    except subprocess.CalledProcessError:
        print("\nBuild failed!")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Build Nano Coverage Godot GDExtension")
    parser.add_argument("--platform", choices=["windows", "linux", "macos", "auto"], default="auto", help="Target platform")
    parser.add_argument("--target", choices=["template_debug", "template_release"], default="template_debug", help="Build target")
    parser.add_argument("--clean", action="store_true", help="Clean before building")
    
    args = parser.parse_args()
    
    run_scons(args)

if __name__ == "__main__":
    main()
