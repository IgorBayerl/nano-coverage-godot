"""
Nano Coverage Godot - Development Environment Setup Script

This script automates the initialization of the development environment for the
nano-coverage-godot project. It handles dependency resolution, tool verification,
and ensures the project structure is correctly linked.

Steps:
1.  Build Tools:     Verifies SCons installation and C++ compiler availability.
2.  Git Sync:        Performs a "smart sync" of submodules based on .gitmodules, fixing detached heads, URL mismatches, and ghost folders.
3.  Godot CPP:       Enforces the specific git tag for GDExtension compatibility.
4.  Google Test:     Downloads and configures Google Test (v1.14.0).
5.  GdUnit4:         Links the GdUnit4 addon submodule into the demo project.
6.  Godot Engine:    Downloads the specific Godot Editor version for testing.

Usage:
    python scripts/setup.py
"""

import os
import sys
import platform
import shutil
import configparser
import utils

# --- Configuration ---
GODOT_VERSION = "4.5-stable"
GODOT_CPP_TAG = "godot-4.3-stable"
GTEST_VERSION = "1.14.0"
URL_GTEST = f"https://github.com/google/googletest/archive/refs/tags/v{GTEST_VERSION}.zip"

# --- Helper Functions for Submodules ---

def _clean_config_value(value):
    if not value: return None
    return value.split(';')[0].split('#')[0].strip()

def _get_registered_submodules():
    success, output = utils.run_command(["git", "submodule", "status"], capture=True, check=False)
    if not success: return []
    
    paths = []
    for line in output.splitlines():
        parts = line.split()
        if len(parts) >= 2:
            paths.append(parts[1])
    return paths

# --- Main Setup Steps ---

def check_system_dependencies():
    utils.log_header("Checking Build Tools")
    
    # Check SCons
    try:
        import SCons
        utils.log_success("SCons library found")
    except ImportError:
        utils.log_warning("SCons library missing. Installing via pip...")
        ok, err = utils.run_command([sys.executable, "-m", "pip", "install", "scons"], capture=True)
        if not ok:
            utils.fail(f"Failed to install SCons: {err}")
        utils.log_success("SCons installed")

    # Check C++ Compiler
    if os.name == 'nt' and (shutil.which("g++") or shutil.which("cl")):
        utils.log_success("Compiler found")
        return
    if os.name != 'nt' and (shutil.which("g++") or shutil.which("clang++")):
        utils.log_success("Compiler found")
        return

    utils.fail("No C++ compiler found (Install Visual Studio or MinGW)")

def sync_git_submodules():
    """
    Advanced submodule sync. Reads .gitmodules as source of truth,
    adds missing modules, syncs URLs, and warns about ghosts.
    """
    utils.log_header("Syncing Git Submodules")
    
    gitmodules_path = os.path.join(utils.PROJECT_ROOT, ".gitmodules")
    if not os.path.exists(gitmodules_path):
        utils.fail(".gitmodules file not found")

    config = configparser.ConfigParser()
    config.read(gitmodules_path)

    modules_in_file = []

    for section in config.sections():
        if not section.startswith('submodule "'):
            continue
            
        path = _clean_config_value(config[section]['path'])
        url = _clean_config_value(config[section]['url'])
        branch = _clean_config_value(config[section].get('branch', None))
        
        modules_in_file.append(path)
        full_path = os.path.join(utils.PROJECT_ROOT, path)

        utils.log_substep(f"Checking: {path}")

        # Check if currently tracked by git index
        status_ok, _ = utils.run_command(["git", "ls-files", "--error-unmatch", path], capture=True, check=False)
        is_in_index = status_ok
        
        # Handle "Ghost" folders (exists on disk but not in git)
        if os.path.exists(full_path) and not is_in_index:
            utils.log_warning("   Folder exists but not in Git Index. Cleaning up...")
            try:
                if os.path.isdir(full_path):
                    shutil.rmtree(full_path)
                else:
                    os.remove(full_path)
            except Exception as e:
                utils.log_error(f"   Failed to clean {path}: {e}")

        # Add or Sync
        if not is_in_index:
            utils.log_step(f"   Adding missing submodule: {path}")
            
            parent_dir = os.path.dirname(full_path)
            if not os.path.exists(parent_dir):
                os.makedirs(parent_dir)

            cmd = ["git", "submodule", "add", "--force"]
            if branch:
                cmd.extend(["-b", branch])
            cmd.extend([url, path])

            success, err = utils.run_command(cmd, cwd=utils.PROJECT_ROOT, capture=True, check=False)
            if success:
                utils.log_success(f"   Added {path}")
            else:
                utils.log_error(f"   Failed to add {path}: {err}")
        else:
            # Force URL sync
            utils.run_command(["git", "submodule", "sync", path], cwd=utils.PROJECT_ROOT, check=False, silent=True)
            # Force branch config update
            if branch:
                utils.run_command(["git", "config", f"submodule.{path}.branch", branch], cwd=utils.PROJECT_ROOT, check=False, silent=True)

    # Warn about submodules in Index but NOT in .gitmodules
    registered_paths = _get_registered_submodules()
    for reg_path in registered_paths:
        if reg_path not in modules_in_file:
            utils.log_warning(f"Found submodule in Git Index NOT in .gitmodules: '{reg_path}'")
            utils.log_substep(f"To remove it, run: git rm {reg_path}")

    # Final Recursive Update
    utils.log_step("Running recursive update...")
    ok, err = utils.run_command(["git", "submodule", "update", "--init", "--recursive"], cwd=utils.PROJECT_ROOT, capture=True)
    if not ok:
        utils.fail(f"Failed to update submodules: {err}")
        
    utils.log_success("Git submodules synced")

def configure_godot_cpp():
    utils.log_header(f"Configuring godot-cpp ({GODOT_CPP_TAG})")
    
    path = os.path.join(utils.PROJECT_ROOT, "godot-cpp")
    if not os.path.exists(path):
        utils.fail(f"godot-cpp directory not found at {path}")

    # Fetch tags to ensure we have the target
    utils.run_command(["git", "fetch", "--tags"], cwd=path, silent=True)
    
    # Checkout specific tag
    ok, err = utils.run_command(["git", "checkout", GODOT_CPP_TAG], cwd=path, capture=True)
    if not ok:
        utils.fail(f"Failed to checkout tag: {err}")
        
    utils.log_success(f"godot-cpp locked to tag: {GODOT_CPP_TAG}")

def setup_googletest():
    utils.log_header("Setting up Google Test")
    
    target_dir = os.path.join(utils.THIRDPARTY_DIR, "googletest")
    
    # Optimization: Check if main header exists
    if os.path.exists(os.path.join(target_dir, "googletest", "src", "gtest-all.cc")):
        utils.log_success("Google Test is already installed")
        return

    # Cleanup old installs
    if os.path.exists(target_dir):
        shutil.rmtree(target_dir)
        
    os.makedirs(utils.THIRDPARTY_DIR, exist_ok=True)
    zip_path = os.path.join(utils.PROJECT_ROOT, "gtest.zip")
    
    if not utils.download_file(URL_GTEST, zip_path):
        utils.fail("Failed to download Google Test")
        
    try:
        utils.extract_zip(zip_path, utils.THIRDPARTY_DIR)
        
        # Rename extracted folder (usually googletest-1.14.0) to standard name
        extracted_name = os.path.join(utils.THIRDPARTY_DIR, f"googletest-{GTEST_VERSION}")
        if os.path.exists(extracted_name):
            os.rename(extracted_name, target_dir)
            
        utils.log_success("Google Test installed")
    finally:
        if os.path.exists(zip_path):
            os.remove(zip_path)

def setup_gdunit4_symlink():
    utils.log_header("Linking GdUnit4 Addon")

    # Source: thirdparty/gdUnit4/addons/gdUnit4
    source_path = os.path.join(utils.THIRDPARTY_DIR, "gdUnit4", "addons", "gdUnit4")
    # Dest: demo/addons/gdUnit4
    dest_path = os.path.join(utils.GODOT_PROJECT_DIR, "addons", "gdUnit4")

    if not os.path.exists(source_path):
        utils.fail(f"GdUnit4 submodule source not found at: {source_path}")

    if not utils.create_symlink(source_path, dest_path):
        utils.fail("Failed to create GdUnit4 symlink")
        
    utils.log_success("GdUnit4 linked")

def setup_godot_editor():
    utils.log_header("Setting up Godot Editor")
    
    base_url = f"https://github.com/godotengine/godot/releases/download/{GODOT_VERSION}/"
    
    if os.name == 'nt':
        download_url = base_url + f"Godot_v{GODOT_VERSION}_win64.exe.zip"
        exe_name = f"Godot_v{GODOT_VERSION}_win64.exe"
    elif sys.platform == 'linux':
        download_url = base_url + f"Godot_v{GODOT_VERSION}_linux.x86_64.zip"
        exe_name = f"Godot_v{GODOT_VERSION}_linux.x86_64"
    elif sys.platform == 'darwin':
        download_url = base_url + f"Godot_v{GODOT_VERSION}_macos.universal.zip"
        exe_name = "Godot.app"
    else:
        utils.fail(f"Unsupported platform: {sys.platform}")

    final_exe_path = os.path.join(utils.PROJECT_ROOT, exe_name)
    if os.path.exists(final_exe_path):
        utils.log_success(f"Found existing Godot executable: {exe_name}")
        return

    zip_path = os.path.join(utils.PROJECT_ROOT, "godot.zip")
    if not utils.download_file(download_url, zip_path):
        utils.fail("Failed to download Godot")
        
    try:
        utils.extract_zip(zip_path, utils.PROJECT_ROOT)
        utils.log_success("Godot Editor installed")
    finally:
        if os.path.exists(zip_path):
            os.remove(zip_path)

def main():
    utils.log_header(f"Nano Coverage Setup (Host: {platform.system()})")
    
    check_system_dependencies()
    sync_git_submodules() 
    configure_godot_cpp()
    setup_googletest()
    setup_gdunit4_symlink()
    setup_godot_editor()
    
    utils.log_success("Environment is ready!")

if __name__ == "__main__":
    main()