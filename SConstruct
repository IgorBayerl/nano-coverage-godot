#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# Change the name of the library here
libname = "nano_coverage"

env.Append(CPPPATH=["src"])
sources = Glob("src/*.cpp")

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
