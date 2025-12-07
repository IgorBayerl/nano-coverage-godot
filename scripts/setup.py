import os
import subprocess
import sys
import shutil
import platform
import urllib.request
import zipfile

# --- CONFIGURATION ---
TESTING_GODOT_VERSION = "4.5-stable" 
TARGET_GODOT_CPP_TAG = "godot-4.3-stable" 
# Official Google Test Release
GTEST_URL = "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip"

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# --- COLORS ---
os.system('') 
GREEN = '\033[92m'
CYAN = '\033[96m'
YELLOW = '\033[93m'
RED = '\033[91m'
RESET = '\033[0m'
CHECK = f"{GREEN}✓{RESET}"
CROSS = f"{RED}✗{RESET}"
ARROW = f"{CYAN}→{RESET}"

def get_godot_url():
    base = f"https://github.com/godotengine/godot/releases/download/{TESTING_GODOT_VERSION}/"
    if os.name == 'nt':
        return base + f"Godot_v{TESTING_GODOT_VERSION}_win64.exe.zip", f"Godot_v{TESTING_GODOT_VERSION}_win64.exe"
    elif sys.platform == 'linux':
        return base + f"Godot_v{TESTING_GODOT_VERSION}_linux.x86_64.zip", f"Godot_v{TESTING_GODOT_VERSION}_linux.x86_64"
    elif sys.platform == 'darwin':
        return base + f"Godot_v{TESTING_GODOT_VERSION}_macos.universal.zip", "Godot.app"
    else:
        print(f"{CROSS} Unsupported platform: {sys.platform}")
        sys.exit(1)

GODOT_URL, GODOT_EXE_NAME = get_godot_url()

def print_step(message):
    print(f"{CYAN}[+]{RESET} {message}")

def print_substep(message, status=""):
    print(f"    {status} {message}")

def run_silent(command, cwd=None):
    try:
        result = subprocess.run(
            command, cwd=cwd, check=True, 
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, shell=(os.name == 'nt')
        )
        return True, result.stdout
    except subprocess.CalledProcessError as e:
        return False, e.stderr + "\n" + e.stdout

def check_tools():
    print_step("Checking Development Tools...")
    try:
        import SCons
        print_substep("SCons library found", CHECK)
    except ImportError:
        print_substep("SCons library missing, installing via pip...", ARROW)
        ok, err = run_silent([sys.executable, "-m", "pip", "install", "scons"])
        if ok: print_substep("SCons installed successfully", CHECK)
        else: print_substep(f"Failed to install SCons: {err}", CROSS); sys.exit(1)

    compiler_found = False
    if os.name == 'nt':
        if shutil.which("g++"): print_substep("MinGW (g++) compiler found", CHECK); compiler_found = True
        elif shutil.which("cl"): print_substep("MSVC (cl.exe) compiler found", CHECK); compiler_found = True
    else:
        if shutil.which("g++") or shutil.which("clang++"): print_substep("Unix C++ compiler found", CHECK); compiler_found = True

    if not compiler_found:
        print(f"\n{RED}!! CRITICAL: No C++ compiler found !!{RESET}")
        sys.exit(1)

def init_submodules():
    print_step("Initializing git submodules...")
    ok, _ = run_silent(["git", "submodule", "sync"], cwd=PROJECT_ROOT)
    if not ok: sys.exit(1)
    ok, _ = run_silent(["git", "submodule", "update", "--init", "--recursive"], cwd=PROJECT_ROOT)
    if ok: print_substep("Submodules updated", CHECK)

def enforce_godot_cpp_version():
    print_step(f"Configuring Compatibility ({TARGET_GODOT_CPP_TAG})...")
    cpp_path = os.path.join(PROJECT_ROOT, "native", "godot-cpp")
    run_silent(["git", "fetch", "--tags"], cwd=cpp_path)
    ok, err = run_silent(["git", "checkout", TARGET_GODOT_CPP_TAG], cwd=cpp_path)
    if ok: print_substep(f"Locked godot-cpp to {TARGET_GODOT_CPP_TAG}", CHECK)
    else: print_substep(f"Failed to checkout tag: {err}", CROSS); sys.exit(1)

def setup_gtest():
    print_step("Setting up Google Test...")
    
    # Path: native/thirdparty/googletest
    thirdparty_dir = os.path.join(PROJECT_ROOT, "native", "thirdparty")
    gtest_root = os.path.join(thirdparty_dir, "googletest")
    
    # Check for the key source file to ensure it installed correctly
    if os.path.exists(os.path.join(gtest_root, "googletest", "src", "gtest-all.cc")):
        print_substep("Google Test found", CHECK)
        return

    # Clean old if partial
    if os.path.exists(gtest_root): shutil.rmtree(gtest_root)
    os.makedirs(thirdparty_dir, exist_ok=True)

    print_substep("Downloading Google Test v1.14.0...", ARROW)
    zip_path = os.path.join(PROJECT_ROOT, "gtest.zip")
    try:
        urllib.request.urlretrieve(GTEST_URL, zip_path)
        
        print_substep("Extracting...", ARROW)
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(thirdparty_dir)
            
        # Rename the extracted folder "googletest-1.14.0" to "googletest"
        extracted_name = os.path.join(thirdparty_dir, "googletest-1.14.0")
        if os.path.exists(extracted_name):
            os.rename(extracted_name, gtest_root)
            
        print_substep("Google Test installed", CHECK)
    except Exception as e:
        print_substep(f"Installation failed: {e}", CROSS)
        sys.exit(1)
    finally:
        if os.path.exists(zip_path): os.remove(zip_path)

def setup_godot_editor():
    print_step("Checking Godot Editor...")
    godot_exe_path = os.path.join(PROJECT_ROOT, GODOT_EXE_NAME)
    
    # Check for Godot in PROJECT_ROOT
    if os.path.exists(godot_exe_path) or (sys.platform == 'darwin' and os.path.exists(os.path.join(PROJECT_ROOT, "Godot.app"))):
        print_substep(f"Found existing {GODOT_EXE_NAME}", CHECK)
        return

    print_substep(f"Downloading Godot {TESTING_GODOT_VERSION}...", ARROW)
    zip_path = os.path.join(PROJECT_ROOT, "godot.zip")
    def show_progress(block_num, block_size, total_size):
        downloaded = block_num * block_size
        if total_size > 0:
            percent = downloaded * 100 / total_size
            sys.stdout.write(f"\r    {ARROW} Downloading: {percent:.1f}%")
            sys.stdout.flush()

    try:
        urllib.request.urlretrieve(GODOT_URL, zip_path, show_progress)
        print("") 
        with zipfile.ZipFile(zip_path, 'r') as zip_ref: zip_ref.extractall(PROJECT_ROOT)
        print_substep("Extraction complete", CHECK)
    except Exception as e:
        print_substep(f"Download failed: {e}", CROSS); sys.exit(1)
    finally:
        if os.path.exists(zip_path): os.remove(zip_path)

def main():
    print(f"\n{YELLOW}=== Nano Coverage Setup ({platform.system()}) ==={RESET}\n")
    init_submodules()
    enforce_godot_cpp_version()
    check_tools()
    setup_gtest()
    setup_godot_editor()
    
    print(f"\n{GREEN}=== Setup Complete ==={RESET}")
    print(f" {CHECK} Lib Target:   Godot 4.3")
    print(f" {CHECK} Test Suite:   Google Test v1.14.0")
    print(f" {CHECK} Test Editor:  Godot {TESTING_GODOT_VERSION}")
    
    # Construct command help
    base_cmd = "python -m SCons"
    flags = " platform=windows" if os.name == 'nt' else ""
    if os.name == 'nt' and shutil.which("g++") and not shutil.which("cl"): flags += " use_mingw=yes"
    
    print("\n---------------------------------------------------------")
    print(f"{YELLOW}HOW TO BUILD:{RESET}")
    print(f"    {CYAN}{base_cmd}{flags}{RESET}")
    print("---------------------------------------------------------")

if __name__ == "__main__":
    main()