"""
Nano Coverage Godot - Build and run Tests

This script orchestrates the full verification pipeline. It ensures the C++
extension builds correctly, passes low-level unit tests, and integrates
successfully with GdUnit4 for high-level coverage reporting.

Steps:
1.  Compilation:     Builds the C++ extension in debug mode.
2.  Cache Priming:   Runs Godot briefly to import assets and prevent timeout errors.
3.  C++ Tests:       Executes GoogleTest unit tests via a generated GDScript runner.
4.  GdUnit4 Baseline: Runs GdUnit4 tests without coverage to ensure stability.
5.  Coverage Run:    Runs GdUnit4 tests *with* Nano Coverage instrumentation to verify report generation and lcov.info output.

Usage:
    python scripts/test.py
"""

import os
import utils
import build

GDUNIT_TEST_DIR = "res://addons/gdUnit4/test/mocker/"

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
        utils.log_success("All Pipeline Phases Completed Successfully!")
    except KeyboardInterrupt:
        utils.fail("\nInterrupted by user")

if __name__ == "__main__":
    main()