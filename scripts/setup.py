"""
Nano Coverage Godot - Development Environment Setup Script

This script automates the initialization of the development environment for the
nano-coverage-godot project. It handles the downloading and configuration of
dependencies required to build and test the project.

Scope of Automation:
1.  Build Tools:     Verifies SCons and C++ compiler availability.
2.  Git Submodules:  Initializes and updates project submodules.
3.  Godot CPP:       Enforces the specific git tag for compatibility.
4.  Google Test:     Downloads and configures Google Test (v1.14.0).
5.  GdUnit4:         Downloads and installs the GdUnit4 addon (v4.2.0).
6.  GdUnit4 Tests:   Extracts the internal test suite from GdUnit4 for self-testing.
7.  Godot Engine:    Downloads the specific Godot Editor version for testing.

Usage:
    python scripts/setup.py
"""

import os
import sys
import shutil
import platform
import subprocess
import urllib.request
import zipfile
import io

# --- Configuration & Constants ---

class Config:
    # Project Paths
    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
    PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
    
    # Updated to match the new flattened structure
    # 'native' folder is gone, src is at root, but we kept a variable just in case
    THIRDPARTY_DIR = os.path.join(PROJECT_ROOT, "thirdparty")
    
    # Renamed 'godot_project' to 'demo'
    DEMO_DIR = os.path.join(PROJECT_ROOT, "demo") 
    ADDONS_DIR = os.path.join(DEMO_DIR, "addons")

    # Version Control
    GODOT_VERSION = "4.5-stable"
    GODOT_CPP_TAG = "godot-4.3-stable"
    GTEST_VERSION = "1.14.0"
    
    # URLs
    URL_GTEST = f"https://github.com/google/googletest/archive/refs/tags/v{GTEST_VERSION}.zip"

class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

# --- Logger Utilities ---

def log_header(message):
    print(f"\n{Colors.HEADER}{Colors.BOLD}=== {message} ==={Colors.RESET}")

def log_info(message):
    print(f"{Colors.CYAN}[INFO] {message}{Colors.RESET}")

def log_success(message):
    print(f"{Colors.GREEN}[OK]   {message}{Colors.RESET}")

def log_warning(message):
    print(f"{Colors.YELLOW}[WARN] {message}{Colors.RESET}")

def log_error(message):
    print(f"{Colors.RED}[ERR]  {message}{Colors.RESET}")

def log_substep(message):
    print(f"       -> {message}")

# --- Helper Functions ---

def run_command(command, cwd=None, shell=False):
    """
    Executes a shell command and returns success status.
    """
    try:
        use_shell = shell or (os.name == 'nt')
        result = subprocess.run(
            command, 
            cwd=cwd, 
            check=True, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE,
            text=True,
            shell=use_shell
        )
        return True, result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return False, e.stderr.strip()

def download_file(url, dest_path):
    """
    Downloads a file from a URL with a progress percentage.
    """
    try:
        def progress(block_num, block_size, total_size):
            downloaded = block_num * block_size
            if total_size > 0:
                percent = downloaded * 100 / total_size
                sys.stdout.write(f"\r       Downloading: {percent:.1f}%")
                sys.stdout.flush()
        
        urllib.request.urlretrieve(url, dest_path, progress)
        print("") # Newline after progress
        return True
    except Exception as e:
        log_error(f"Download failed: {e}")
        return False

def create_symlink(source, target):
    """
    Creates a symlink (Unix) or Directory Junction (Windows) 
    from source to target.
    """
    source = os.path.abspath(source)
    target = os.path.abspath(target)

    if os.path.exists(target):
        # Check if it's already a link pointing to the right place
        if os.path.islink(target) or (os.name == 'nt' and os.path.exists(target)):
             # Basic check to avoid overwriting valid setups
             # On Windows os.path.islink is only true for modern symlinks, not junctions usually
             log_info(f"Target already exists: {os.path.basename(target)}")
             return True
        else:
             log_warning(f"Target exists but is not a link. Backing up...")
             os.rename(target, target + ".bak")

    # Ensure parent dir exists
    os.makedirs(os.path.dirname(target), exist_ok=True)

    try:
        if os.name == 'nt':
            # Windows: Use mklink /J for Directory Junctions (requires no admin privileges usually)
            # Python's os.symlink requires Developer Mode on Windows, subprocess mklink is safer.
            cmd = f'mklink /J "{target}" "{source}"'
            subprocess.run(cmd, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            # Unix/Linux/Mac
            os.symlink(source, target)
        
        log_success(f"Linked: {os.path.basename(source)} <==> {os.path.basename(target)}")
        return True
    except Exception as e:
        log_error(f"Failed to link {source} to {target}: {e}")
        return False

# --- Setup Tasks ---

def check_system_dependencies():
    """
    Checks if SCons and a C++ compiler are installed.
    """
    log_header("Checking Build Tools")
    
    # 1. Check SCons
    try:
        import SCons
        log_success("SCons library found.")
    except ImportError:
        log_warning("SCons library missing. Attempting installation via pip...")
        ok, err = run_command([sys.executable, "-m", "pip", "install", "scons"])
        if ok:
            log_success("SCons installed successfully.")
        else:
            log_error(f"Failed to install SCons: {err}")
            return False

    # 2. Check Compiler
    compiler_found = False
    if os.name == 'nt':
        if shutil.which("g++") or shutil.which("cl"):
            log_success("Compiler found (MinGW or MSVC).")
            compiler_found = True
    else:
        if shutil.which("g++") or shutil.which("clang++"):
            log_success("Compiler found.")
            compiler_found = True

    if not compiler_found:
        log_error("No C++ compiler found (Install Visual Studio or MinGW).")
        return False

    return True

def setup_git_submodules():
    """
    Initializes and updates git submodules.
    """
    log_header("Initializing Git Submodules")
    
    ok, _ = run_command(["git", "submodule", "sync"], cwd=Config.PROJECT_ROOT)
    if not ok:
        log_error("Failed to sync submodules.")
        return False
        
    ok, err = run_command(["git", "submodule", "update", "--init", "--recursive"], cwd=Config.PROJECT_ROOT)
    if ok:
        log_success("Submodules updated.")
        return True
    else:
        log_error(f"Failed to update submodules: {err}")
        return False

def configure_godot_cpp():
    """
    Ensures godot-cpp is checked out to the correct tag.
    """
    # Look for godot-cpp in root first (new structure), then native (old structure fallback)
    path = os.path.join(Config.PROJECT_ROOT, "godot-cpp")
    if not os.path.exists(path):
        path = os.path.join(Config.PROJECT_ROOT, "native", "godot-cpp")

    log_header(f"Configuring godot-cpp ({Config.GODOT_CPP_TAG})")
    
    if not os.path.exists(path):
        log_error(f"godot-cpp directory not found at {path}")
        return False

    run_command(["git", "fetch", "--tags"], cwd=path)
    ok, err = run_command(["git", "checkout", Config.GODOT_CPP_TAG], cwd=path)
    if ok:
        log_success(f"godot-cpp locked to tag: {Config.GODOT_CPP_TAG}")
        return True
    else:
        log_error(f"Failed to checkout tag: {err}")
        return False

def setup_googletest():
    """
    Downloads and extracts Google Test to thirdparty/googletest.
    """
    log_header("Setting up Google Test")
    
    target_dir = os.path.join(Config.THIRDPARTY_DIR, "googletest")
    
    # Check for a key file to avoid re-downloading
    if os.path.exists(os.path.join(target_dir, "googletest", "src", "gtest-all.cc")):
        log_success("Google Test is already installed.")
        return True

    # Cleanup previous partial installs
    if os.path.exists(target_dir):
        shutil.rmtree(target_dir)
    os.makedirs(Config.THIRDPARTY_DIR, exist_ok=True)

    log_info(f"Downloading Google Test v{Config.GTEST_VERSION}...")
    zip_path = os.path.join(Config.PROJECT_ROOT, "gtest.zip")
    
    if download_file(Config.URL_GTEST, zip_path):
        try:
            log_substep("Extracting archive...")
            with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                zip_ref.extractall(Config.THIRDPARTY_DIR)
            
            # Rename folder from googletest-1.14.0 to googletest
            extracted_name = os.path.join(Config.THIRDPARTY_DIR, f"googletest-{Config.GTEST_VERSION}")
            if os.path.exists(extracted_name):
                os.rename(extracted_name, target_dir)
            
            log_success("Google Test installed successfully.")
            return True
        except Exception as e:
            log_error(f"Extraction failed: {e}")
            return False
        finally:
            if os.path.exists(zip_path):
                os.remove(zip_path)
    return False

def setup_gdunit4_symlink():
    """
    Links the gdUnit4 addon from the submodule to the demo project.
    """
    log_header("Linking GdUnit4 Addon")

    # Source: thirdparty/gdUnit4/addons/gdUnit4
    source_path = os.path.join(Config.THIRDPARTY_DIR, "gdUnit4", "addons", "gdUnit4")
    
    # Dest: demo/addons/gdUnit4
    dest_path = os.path.join(Config.ADDONS_DIR, "gdUnit4")

    if not os.path.exists(source_path):
        log_error(f"GdUnit4 submodule source not found at: {source_path}")
        log_substep("Did you add the submodule correctly?")
        return False

    return create_symlink(source_path, dest_path)

def setup_godot_editor():
    """
    Downloads the Godot Editor executable.
    """
    log_header("Setting up Godot Editor")
    
    # Determine platform URL and filename
    base_url = f"https://github.com/godotengine/godot/releases/download/{Config.GODOT_VERSION}/"
    exe_name = ""
    download_url = ""
    
    if os.name == 'nt':
        download_url = base_url + f"Godot_v{Config.GODOT_VERSION}_win64.exe.zip"
        exe_name = f"Godot_v{Config.GODOT_VERSION}_win64.exe"
    elif sys.platform == 'linux':
        download_url = base_url + f"Godot_v{Config.GODOT_VERSION}_linux.x86_64.zip"
        exe_name = f"Godot_v{Config.GODOT_VERSION}_linux.x86_64"
    elif sys.platform == 'darwin':
        download_url = base_url + f"Godot_v{Config.GODOT_VERSION}_macos.universal.zip"
        exe_name = "Godot.app"
    else:
        log_error(f"Unsupported platform: {sys.platform}")
        return False

    final_exe_path = os.path.join(Config.PROJECT_ROOT, exe_name)
    
    if os.path.exists(final_exe_path):
        log_success(f"Found existing Godot executable: {exe_name}")
        return True

    log_info(f"Downloading Godot {Config.GODOT_VERSION}...")
    zip_path = os.path.join(Config.PROJECT_ROOT, "godot.zip")
    
    if download_file(download_url, zip_path):
        try:
            log_substep("Extracting...")
            with zipfile.ZipFile(zip_path, 'r') as zip_ref:
                zip_ref.extractall(Config.PROJECT_ROOT)
            log_success("Godot Editor installed.")
            return True
        except Exception as e:
            log_error(f"Failed to extract Godot: {e}")
            return False
        finally:
            if os.path.exists(zip_path):
                os.remove(zip_path)
    return False

# --- Main Execution ---

def main():
    print(f"\n{Colors.YELLOW}{Colors.BOLD}Nano Coverage Setup (Host: {platform.system()}){Colors.RESET}")
    print("---------------------------------------------------------")

    steps = [
        ("System Dependencies", check_system_dependencies),
        ("Git Submodules", setup_git_submodules),
        ("Godot-CPP Version", configure_godot_cpp),
        ("Google Test", setup_googletest),
        ("GdUnit4 Link", setup_gdunit4_symlink),
        ("Godot Editor", setup_godot_editor)
    ]

    results = []
    
    # Execute steps
    for name, func in steps:
        try:
            success = func()
            results.append((name, success))
            if not success and name == "System Dependencies":
                # Critical failure
                log_error("Critical dependency missing. Stopping setup.")
                sys.exit(1)
        except Exception as e:
            log_error(f"Unexpected error in {name}: {e}")
            results.append((name, False))

    # Print Summary
    print("\n---------------------------------------------------------")
    print(f"{Colors.BOLD}SETUP SUMMARY{Colors.RESET}")
    print("---------------------------------------------------------")
    
    all_passed = True
    for name, success in results:
        status = f"{Colors.GREEN}PASS{Colors.RESET}" if success else f"{Colors.RED}FAIL{Colors.RESET}"
        print(f" {status} : {name}")
        if not success: all_passed = False

    if all_passed:
        scons_cmd = "python -m SCons"
        print("\n---------------------------------------------------------")
        print(f"{Colors.YELLOW}Environment is ready! You can now build the project.{Colors.RESET}")
        print(f"Build Command: {Colors.CYAN}{scons_cmd}{Colors.RESET}")
        print("---------------------------------------------------------")
    else:
        print(f"\n{Colors.RED}Setup completed with errors. Please check the logs above.{Colors.RESET}")
        sys.exit(1)

if __name__ == "__main__":
    main()