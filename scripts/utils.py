"""
Nano Coverage Godot - Developer Utilities

This module provides a shared library of utility functions used across the
build, setup, and test scripts. It ensures consistent logging, error handling,
and cross-platform file system operations.

- Console Output:  Standardized logging with ANSI colors and formatting.
- Process Mgmt:    Robust shell command execution with output capturing.
- File System:     Cross-platform symlinks, zip extraction, and path resolution.
- Network:         File downloading with visual progress indicators.
- Godot Discovery: Automatically locates the Godot Editor executable.
"""

import os
import sys
import glob
import subprocess
import urllib.request
import zipfile

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GODOT_PROJECT_DIR = os.path.join(PROJECT_ROOT, "demo")
BUILD_DIR = PROJECT_ROOT
THIRDPARTY_DIR = os.path.join(PROJECT_ROOT, "thirdparty")

_CYAN = '\033[96m'
_GREEN = '\033[92m'
_YELLOW = '\033[93m'
_RED = '\033[91m'
_RESET = '\033[0m'
_BOLD = '\033[1m'

os.system('')

def log_header(msg):
    print(f"\n{_BOLD}{_CYAN}=== {msg} ==={_RESET}")

def log_step(msg):
    print(f"{_CYAN}[+]{_RESET} {msg}")

def log_substep(msg):
    print(f"    {msg}")

def log_success(msg):
    print(f"{_GREEN}[OK] {msg}{_RESET}")

def log_warning(msg):
    print(f"{_YELLOW}[WARN] {msg}{_RESET}")

def log_error(msg):
    print(f"{_RED}[ERR] {msg}{_RESET}")

def fail(msg, code=1):
    log_error(msg)
    sys.exit(code)

def run_command(cmd, cwd=None, check=True, env=None, capture=False, silent=False):
    """
    Executes a shell command.
    capture=True returns (success, output_string).
    capture=False returns (success, "").
    """
    try:
        if capture:
            result = subprocess.run(
                cmd, cwd=cwd, check=check, env=env,
                shell=(os.name == 'nt'),
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            return True, result.stdout.strip()
        
        out_dest = subprocess.DEVNULL if silent else None
        subprocess.run(
            cmd, cwd=cwd, check=check, env=env,
            shell=(os.name == 'nt'),
            stdout=out_dest, stderr=out_dest
        )
        return True, ""
    except subprocess.CalledProcessError as e:
        if capture:
            return False, e.stderr.strip()
        if check:
            fail(f"Command failed: {' '.join(cmd)}")
        return False, ""

def download_file(url, dest_path):
    log_substep(f"Downloading {url}")
    
    def progress(count, block_size, total_size):
        if total_size <= 0: return
        percent = min(100, int(count * block_size * 100 / total_size))
        sys.stdout.write(f"\r         Progress: {percent}%")
        sys.stdout.flush()
        
    try:
        urllib.request.urlretrieve(url, dest_path, reporthook=progress)
        print()
        return True
    except Exception as e:
        log_error(f"Download failed: {e}")
        return False

def extract_zip(zip_path, extract_to):
    log_substep(f"Extracting {os.path.basename(zip_path)}")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(extract_to)
        return True
    except Exception as e:
        log_error(f"Extraction failed: {e}")
        return False

def find_godot_executable():
    patterns = ["Godot_*.exe"] if os.name == 'nt' else ["Godot_*.x86_64", "Godot_*.x86_32", "Godot_v*"]
    if sys.platform == 'darwin':
        patterns = ["Godot.app"]
    
    found = []
    for p in patterns:
        found.extend(glob.glob(os.path.join(PROJECT_ROOT, p)))

    if not found: return None
        
    exe = sorted(found)[-1]
    if sys.platform == 'darwin' and exe.endswith(".app"):
        exe = os.path.join(exe, "Contents", "MacOS", "Godot")
        
    return os.path.abspath(exe)

def create_symlink(source, target):
    source = os.path.abspath(source)
    target = os.path.abspath(target)

    if os.path.exists(target):
        if os.path.islink(target) or (os.name == 'nt' and os.path.exists(target)):
            return True
        os.rename(target, target + ".bak")

    os.makedirs(os.path.dirname(target), exist_ok=True)

    try:
        if os.name == 'nt':
            cmd = f'mklink /J "{target}" "{source}"'
            subprocess.run(cmd, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            os.symlink(source, target)
        return True
    except Exception as e:
        log_error(f"Failed to link {source} to {target}: {e}")
        return False