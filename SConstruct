#!/usr/bin/env python
import os

env = SConscript("godot-cpp/SConstruct")

# --- Tree-sitter Configuration ---
# Include paths for tree-sitter headers
env.Append(CPPPATH=[
    "src",
    "thirdparty/tree-sitter/lib/include",
    "thirdparty/tree-sitter/lib/src",
    "thirdparty/tree-sitter-gdscript/src",
])

# Define sources
sources = Glob("src/*.cpp")

# Add Tree-sitter Core (C source)
# We handle .c files explicitly
sources.append("thirdparty/tree-sitter/lib/src/lib.c")

# Add GDScript Grammar (Parser + Scanner)
sources.append("thirdparty/tree-sitter-gdscript/src/parser.c")
sources.append("thirdparty/tree-sitter-gdscript/src/scanner.c")

# Flags to allow C99 for tree-sitter
if env["platform"] == "windows" and env.get("is_msvc", False):
    env.Append(CFLAGS=["/std:c11"])
    # Disable some warnings for thirdparty code if needed
    env.Append(CCFLAGS=["/wd4244", "/wd4267"]) 
else:
    env.Append(CFLAGS=["-std=c11"])

# --- Build Shared Library ---
libname = "nano_coverage"

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/{}.{}.{}.framework/{}".format(
            libname, env["platform"], env["target"], libname
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "demo/bin/{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
