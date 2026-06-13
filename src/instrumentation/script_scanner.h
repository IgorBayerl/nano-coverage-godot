#pragma once

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../config/settings_gateway.h"

namespace godot {

// Shared .gd file discovery used by both instrumentation modes
// (memory via ProjectBootstrapper, disk via DiskInstrumenter).
class ScriptScanner {
   public:
    // Builds the effective ignore glob list from the coverage settings,
    // including the addon exclusion rules.
    static Array build_ignore_globs(const CoverageSettings& settings);

    // Compiles glob patterns (e.g. "**/*_test.gd") into an Array of Ref<RegEx>.
    static Array compile_ignore_patterns(const Array& glob_patterns);

    // Recursively collects all .gd files under root_path, skipping paths
    // matching any of the compiled regexes.
    static Array find_gd_files(const String& root_path, const Array& compiled_regexes);

    // Convenience: scans res:// using the ignore rules from settings.
    static Array scan_project(const CoverageSettings& settings);
};

}  // namespace godot
