#!/usr/bin/env python
import os
import sys

# 1. Load Godot-CPP
# We clone the environment so we don't mess up Godot-CPP's internal logic
env = SConscript("godot-cpp/SConstruct")
project_env = env.Clone()

# 2. Build Arguments
build_tests = ARGUMENTS.get("build_tests", "yes")

# 3. Base Configuration
# Paths are relative to Project Root
project_env.Append(CPPPATH=[
    "src",
    "godot-cpp/include",
    "godot-cpp/gen/include",
    "thirdparty/tree-sitter/lib/include",
    "thirdparty/tree-sitter/lib/src",
    "thirdparty/tree-sitter-gdscript/src",
])

# 4. Source Collection
sources = []

# --- Google Test Integration ---
if build_tests == "yes":
    print(f"Enabling Tests (build_tests={build_tests})")
    project_env.Append(CPPDEFINES=["TESTS_ENABLED"])
    
    # Path to the GoogleTest submodule/download
    # Structure: thirdparty/googletest/googletest/include/gtest/gtest.h
    gtest_root = "thirdparty/googletest/googletest"
    
    # Add Headers
    project_env.Append(CPPPATH=[
        os.path.join(gtest_root, "include"),
        gtest_root # <--- CRITICAL FIX: Needed for gtest-all.cc to find internal src files
    ])
    
    # Add GTest Source (We must compile the library itself)
    sources.append(os.path.join(gtest_root, "src", "gtest-all.cc"))
    
else:
    print(f"Disabling Tests (build_tests={build_tests})")

# --- Project Source Collection ---
# Walk 'src' directory
for root, dirs, files in os.walk("src"):
    for file in files:
        if file.endswith(".cpp"):
            path = os.path.join(root, file).replace("\\", "/")
            
            # Logic: If build_tests is 'no', exclude files ending in _test.cpp
            if build_tests == "no" and ("_test.cpp" in file or file == "test_main.cpp"):
                continue
            
            # Additional check: Don't double-add gtest source if it ended up in src for some reason
            if "gtest-all.cc" in file:
                continue
                
            sources.append(path)

# --- Tree Sitter Sources ---
sources.append("thirdparty/tree-sitter/lib/src/lib.c")
sources.append("thirdparty/tree-sitter-gdscript/src/parser.c")
scanner_c = "thirdparty/tree-sitter-gdscript/src/scanner.c"
if os.path.exists(scanner_c):
    sources.append(scanner_c)

# 5. Compilation output
# TARGET: Directly into the demo project's addons folder
addon_bin_dir = "demo/addons/nano_coverage_godot/bin/"
project_env.Execute(Mkdir(addon_bin_dir))

base_name = "nano_coverage_godot"
try:
    arch = project_env.get('arch', 'x86_64')
    suffix = f".{project_env['platform']}.{project_env['target']}.{arch}"
    target_name = base_name + suffix
except KeyError:
    target_name = base_name + ".dll"

shlib_suffix = project_env.subst('$SHLIBSUFFIX')
if not target_name.endswith(shlib_suffix):
    target_name += shlib_suffix

library = project_env.SharedLibrary(
    os.path.join(addon_bin_dir, target_name),
    sources,
)

Default(library)