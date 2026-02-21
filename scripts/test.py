"""
Nano Coverage Godot - Test Pipeline

Orchestrates the full test pipeline for Nano Coverage:
1. Compiles the C++ GDExtension.
2. Runs the Godot C++ Unit Tests.
3. Runs the GdUnit4 test suite without coverage (Baseline).
4. Runs the GdUnit4 test suite with Nano Coverage (Comparison).
"""

import os
import subprocess
import sys
import glob
import build

# Configuration
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# UPDATED: Changed from 'godot_project' to 'demo'
GODOT_PROJECT_DIR = os.path.join(PROJECT_ROOT, "demo")
GDUNIT_TEST_DIR = "res://addons/gdUnit4/test/mocker/" # Change to "res://addons/gdUnit4/test/" to run the entire suite

# Formatting
os.system('')
COLOR_GREEN = '\033[92m'
COLOR_CYAN = '\033[96m'
COLOR_RED = '\033[91m'
COLOR_RESET = '\033[0m'

def log_step(msg):
    print(f"\n{COLOR_CYAN}[=] {msg}{COLOR_RESET}")

def log_success(msg):
    print(f"{COLOR_GREEN}[+] {msg}{COLOR_RESET}")

def log_error(msg):
    print(f"{COLOR_RED}[!] {msg}{COLOR_RESET}")

def find_godot_executable():
    patterns = []
    if os.name == 'nt':
        patterns = ["Godot_*.exe"]
    elif sys.platform == 'darwin':
        patterns = ["Godot.app"] 
    else:
        patterns = ["Godot_*.x86_64", "Godot_*.x86_32", "Godot_v*"]

    found = []
    for p in patterns:
        found.extend(glob.glob(os.path.join(PROJECT_ROOT, p)))

    if not found:
        log_error("Godot executable not found. Run 'python setup.py' to download it.")
        sys.exit(1)

    exe = sorted(found)[-1]
    if sys.platform == 'darwin' and exe.endswith(".app"):
        exe = os.path.join(exe, "Contents", "MacOS", "Godot")
        
    return os.path.abspath(exe)

def prime_godot_cache(godot_exe):
    log_step("Priming Godot Cache...")
    cmd = [godot_exe, "--headless", "--path", GODOT_PROJECT_DIR, "--editor", "--quit"]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        log_success("Cache updated.")
    except subprocess.CalledProcessError:
        log_error("Warning: Cache priming exited with an error. Tests might fail.")

def run_cpp_tests(godot_exe):
    log_step("Phase 1: C++ Core Unit Tests")
    runner_script_path = os.path.join(GODOT_PROJECT_DIR, "addons", "run_cpp_tests.gd")
    
    gd_script_content = """extends SceneTree
func _init():
    var runner = NanoCoverageTestRunner.new()
    var result = runner.run_all_tests()
    quit(result)
"""
    try:
        with open(runner_script_path, "w") as f:
            f.write(gd_script_content)

        cmd = [godot_exe, "--headless", "--path", GODOT_PROJECT_DIR, "--script", "addons/run_cpp_tests.gd"]
        ret_code = subprocess.call(cmd)
        
        if ret_code != 0:
            log_error(f"C++ Tests Failed (Exit Code: {ret_code})")
            sys.exit(ret_code)
            
        log_success("C++ Tests Passed")
    finally:
        if os.path.exists(runner_script_path):
            os.remove(runner_script_path)

def run_gdunit_baseline(godot_exe):
    log_step("Phase 2: GdUnit4 Baseline Tests (No Coverage)")
    cmd = [
        godot_exe, "--headless", "--path", GODOT_PROJECT_DIR,
        "-s", "res://addons/gdUnit4/bin/GdUnitCmdTool.gd",
        "-a", GDUNIT_TEST_DIR
    ]
    
    ret_code = subprocess.call(cmd)
    if ret_code != 0:
        log_error(f"GdUnit4 Baseline Failed (Exit Code: {ret_code})")
        sys.exit(ret_code)
        
    log_success("GdUnit4 Baseline Passed")

def run_gdunit_coverage(godot_exe):
    log_step("Phase 3: GdUnit4 Tests WITH Nano Coverage")
    runner_script_path = os.path.join(GODOT_PROJECT_DIR, "addons", "run_cov_tests.gd")
    
    # We purposefully exclude ONLY nano_coverage_godot.
    # This allows GdUnit4 itself to be fully instrumented so we can capture its coverage data.
    # Note: If tests explicitly assert line numbers (has_line()), they might fail because 
    # the instrumenter currently injects full lines, shifting the code downward.
    gd_script_content = f"""extends SceneTree
func _init():
    var api = ClassDB.instantiate("CoverageApi")
    print("[NanoCoverage] Instrumenting project...")
    
    var instr_res = api.instrument_project({{
        "exclude": ["res://addons/nano_coverage_godot"]
    }})
    
    if instr_res.has("error"):
        printerr("[NanoCoverage] Instrumentation failed: ", instr_res.error)
        quit(1)
        return
        
    var temp_path = instr_res.output_path
    var run_res = api.run_instrumented_project({{
        "output_path": temp_path,
        "workspace_id": "cli_tests",
        "dry_run": true
    }})
    
    var base_args = run_res.args
    var final_args = PackedStringArray()
    for arg in base_args:
        if arg == "++":
            final_args.push_back("--headless")
            final_args.push_back("-s")
            final_args.push_back("res://addons/gdUnit4/bin/GdUnitCmdTool.gd")
            final_args.push_back("-a")
            final_args.push_back("{GDUNIT_TEST_DIR}")
        final_args.push_back(arg)
        
    print("[NanoCoverage] Running instrumented tests...")
    var output = []
    var exit_code = OS.execute(OS.get_executable_path(), final_args, output, true, true)
    
    if output.size() > 0:
        print(output[0])
        
    print("[NanoCoverage] Generating LCOV report...")
    var report_res = api.generate_coverage_report({{"workspace_id": "cli_tests"}})
    if report_res.has("error"):
        printerr("[NanoCoverage] Report Generation Failed: ", report_res.error)
        
    quit(exit_code)
"""
    try:
        with open(runner_script_path, "w") as f:
            f.write(gd_script_content)

        cmd = [godot_exe, "--headless", "--path", GODOT_PROJECT_DIR, "--script", "addons/run_cov_tests.gd"]
        ret_code = subprocess.call(cmd)
        
        if ret_code != 0:
            log_error(f"GdUnit4 Coverage Run Failed (Exit Code: {ret_code})")
            sys.exit(ret_code)
            
        log_success("GdUnit4 Coverage Run Passed. lcov.info generated.")
    finally:
        if os.path.exists(runner_script_path):
            os.remove(runner_script_path)

def main():
    print(f"\n{COLOR_GREEN}=== Nano Coverage Test Pipeline ==={COLOR_RESET}")
    
    class BuildArgs:
        platform = "auto"
        target = "template_debug" 
        clean = False
        only_clean = False
        no_tests = False 
    
    try:
        build.run_scons(BuildArgs())
    except SystemExit:
        log_error("Build Failed. Aborting tests.")
        sys.exit(1)
        
    godot_exe = find_godot_executable()
    prime_godot_cache(godot_exe)
    
    try:
        run_cpp_tests(godot_exe)
        run_gdunit_baseline(godot_exe)
        run_gdunit_coverage(godot_exe)
        
        print(f"\n{COLOR_GREEN}=== All Pipeline Phases Completed Successfully! ==={COLOR_RESET}")
    except KeyboardInterrupt:
        log_error("\nInterrupted by user.")
        sys.exit(1)

if __name__ == "__main__":
    main()