"""
Nano Coverage Godot - Build System Wrapper

This script acts as a high-level wrapper around SCons to compile the C++
GDExtension. It handles platform detection, compiler flag configuration,
and build target management.

Steps:
1.  Platform Check:  Auto-detects OS (Windows/Linux/macOS) and CPU architecture.
2.  Compiler Setup:  Prioritizes MinGW on Windows if MSVC is missing.
3.  Concurrency:     Automatically utilizes all available CPU cores (-jN).
4.  Test Toggle:     Controls the compilation of C++ GoogleTest sources.
5.  Cleaning:        Provides robust build artifact cleaning before compilation.

Usage:
    python scripts/build.py [--platform auto] [--target template_debug] [--clean]
"""

import sys
import shutil
import argparse
import multiprocessing
import utils

def _detect_platform(platform_arg):
    if platform_arg != "auto":
        return platform_arg
    return None

def _detect_compiler_flags():
    if sys.platform != 'win32':
        return []
    
    has_gpp = shutil.which("g++") is not None
    has_msvc = shutil.which("cl") is not None
    
    if has_gpp and not has_msvc:
        utils.log_substep("Auto-detected MinGW")
        return ["use_mingw=yes"]
        
    return []

def _construct_scons_command(args):
    cmd = [sys.executable, "-m", "SCons"]
    
    cpu_count = multiprocessing.cpu_count()
    cmd.append(f"-j{cpu_count}")
    
    platform = _detect_platform(args.platform)
    if platform:
        cmd.append(f"platform={platform}")
        
    cmd.append(f"target={args.target}")
    
    if args.no_tests:
        utils.log_substep("Building without tests")
        cmd.append("build_tests=no")
    else:
        cmd.append("build_tests=yes")
        
    if args.target == "template_debug":
        cmd.append("debug_symbols=yes")
        
    cmd.extend(_detect_compiler_flags())
    return cmd

def run_build(args):
    if args.clean:
        utils.log_step("Cleaning build targets")
        utils.run_command([sys.executable, "-m", "SCons", "-c"], cwd=utils.BUILD_DIR)
        utils.log_success("Clean complete")
        if args.only_clean:
            return

    utils.log_step("Compiling Extension")
    cmd = _construct_scons_command(args)
    
    utils.log_substep(f"Command: {' '.join(cmd)}")
    utils.run_command(cmd, cwd=utils.BUILD_DIR)
    utils.log_success("Build Successful")

def main():
    parser = argparse.ArgumentParser(description="Build Manager")
    parser.add_argument("--platform", choices=["windows", "linux", "macos", "android", "ios", "auto"], default="auto")
    parser.add_argument("--target", choices=["template_debug", "template_release"], default="template_debug")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--only-clean", action="store_true")
    parser.add_argument("--no-tests", action="store_true")
    
    args = parser.parse_args()
    
    utils.log_header("Nano Coverage Builder")
    run_build(args)

if __name__ == "__main__":
    main()