"""
Nano Coverage Godot - Build and run Tests

This script orchestrates the full verification pipeline. It ensures the C++
extension builds correctly, passes low-level unit tests, and integrates
successfully with GdUnit4 for high-level coverage reporting.

Steps:
1.  Compilation:      Builds the C++ extension in debug mode.
2.  Cache Priming:    Runs Godot briefly to import assets.
3.  C++ Tests:        Executes GoogleTest unit tests via a generated GDScript runner.
4.  GdUnit4 Coverage: Executes the 'runtest' script provided by GdUnit4 to verify self-coverage.

Usage:
    python scripts/test.py
"""

import os
import sys
import utils
import build

GDUNIT_TEST_TARGET = "res://addons/gdUnit4/test/mocker"

def prime_godot_cache(godot_exe):
    utils.log_step("Priming Godot Cache")
    cmd = [godot_exe, "--headless", "--path", utils.GODOT_PROJECT_DIR, "--editor", "--quit"]
    utils.run_command(cmd, check=False, silent=True)
    utils.log_success("Cache updated")

def run_cpp_tests(godot_exe):
    utils.log_step("Phase 1: C++ Core Unit Tests")
    runner_script = os.path.join(utils.GODOT_PROJECT_DIR, "addons", "run_cpp_tests.gd")
    
    content = """extends SceneTree
func _init():
    var runner = NanoCoverageTestRunner.new()
    var result = runner.run_all_tests()
    quit(result)
"""
    with open(runner_script, "w") as f:
        f.write(content)

    cmd = [godot_exe, "--headless", "--path", utils.GODOT_PROJECT_DIR, "--script", "addons/run_cpp_tests.gd"]
    
    try:
        utils.run_command(cmd)
        utils.log_success("C++ Tests Passed")
    finally:
        if os.path.exists(runner_script):
            os.remove(runner_script)

def run_gdunit_tests(godot_exe):
    utils.log_step("Phase 2: GdUnit4 Self-Coverage Tests")
    
    # Detect the correct script for the OS
    script_name = "runtest.cmd" if os.name == 'nt' else "runtest.sh"
    script_path = os.path.join(utils.GODOT_PROJECT_DIR, "addons", "gdUnit4", script_name)
    
    # Ensure the script is executable on Linux/Mac
    if os.name != 'nt':
        os.chmod(script_path, 0o755)

    # Construct the command:
    # ./runtest.cmd --godot_binary <path> -a <target> -c
    cmd = [
        os.path.abspath(script_path),
        "--godot_binary", godot_exe,
        "-a", GDUNIT_TEST_TARGET,
        "-c"
    ]
    
    utils.log_substep(f"Executing: {script_name}")
    utils.log_substep(f"Target:    {GDUNIT_TEST_TARGET}")
    
    # Execute inside the 'demo' directory so the script can find project files at '.'
    try:
        utils.run_command(cmd, cwd=utils.GODOT_PROJECT_DIR)
        utils.log_success("GdUnit4 Integration Tests Passed")
    except SystemExit:
        utils.fail("GdUnit4 Integration Tests Failed")

class BuildArgs:
    platform = "auto"
    target = "template_debug" 
    clean = False
    only_clean = False
    no_tests = False 

def main():
    utils.log_header("Nano Coverage Test Pipeline")
    
    try:
        build.run_build(BuildArgs())
    except SystemExit:
        utils.fail("Build Failed")
        
    godot_exe = utils.find_godot_executable()
    if not godot_exe:
        utils.fail("Godot executable not found")

    prime_godot_cache(godot_exe)
    
    try:
        run_cpp_tests(godot_exe)
        run_gdunit_tests(godot_exe)
        utils.log_success("All Pipeline Phases Completed Successfully!")
    except KeyboardInterrupt:
        utils.fail("\nInterrupted by user")

if __name__ == "__main__":
    main()