#!/usr/bin/env python
import os

# Environment Setup
env = SConscript("godot-cpp/SConstruct")
project_env = env.Clone()

build_tests = ARGUMENTS.get("build_tests", "yes")

# Include Paths
project_env.Append(CPPPATH=[
    "src",
    "godot-cpp/include",
    "godot-cpp/gen/include",
    "thirdparty/tree-sitter/lib/include",
    "thirdparty/tree-sitter/lib/src",
    "thirdparty/tree-sitter-gdscript/src",
])

# Source Compilation
sources = []

# Google Test
if build_tests == "yes":
    print(f"Enabling Tests (build_tests={build_tests})")
    project_env.Append(CPPDEFINES=["TESTS_ENABLED"])
    
    gtest_root = "thirdparty/googletest/googletest"
    
    # Add GTest Includes (Root include needed for gtest-all.cc internals)
    project_env.Append(CPPPATH=[
        os.path.join(gtest_root, "include"),
        gtest_root 
    ])
    
    # Compile GTest Library
    sources.append(os.path.join(gtest_root, "src", "gtest-all.cc"))
else:
    print(f"Disabling Tests (build_tests={build_tests})")

# Project Source Files
for root, dirs, files in os.walk("src"):
    for file in files:
        if not file.endswith(".cpp"):
            continue

        # Filter tests if disabled
        if build_tests == "no" and ("_test.cpp" in file or file == "test_main.cpp"):
            continue
        
        # Avoid duplicate inclusion if gtest ends up in src
        if "gtest-all.cc" in file:
            continue
            
        sources.append(os.path.join(root, file).replace("\\", "/"))

# Tree-sitter Dependencies
sources.extend([
    "thirdparty/tree-sitter/lib/src/lib.c",
    "thirdparty/tree-sitter-gdscript/src/parser.c"
])

scanner_c = "thirdparty/tree-sitter-gdscript/src/scanner.c"
if os.path.exists(scanner_c):
    sources.append(scanner_c)

# Target Generation
addon_bin_dir = "demo/addons/nano_coverage_godot/bin/"
project_env.Execute(Mkdir(addon_bin_dir))

base_name = "nano_coverage_godot"
try:
    arch = project_env.get('arch', 'x86_64')
    suffix = f".{project_env['platform']}.{project_env['target']}.{arch}"
    target_name = base_name + suffix
except KeyError:
    target_name = base_name + ".dll"

# Ensure proper extension for the platform
shlib_suffix = project_env.subst('$SHLIBSUFFIX')
if not target_name.endswith(shlib_suffix):
    target_name += shlib_suffix

library = project_env.SharedLibrary(
    os.path.join(addon_bin_dir, target_name),
    sources,
)

Default(library)