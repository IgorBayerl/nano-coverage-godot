#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>  // Add this
#include <godot_cpp/variant/string.hpp>

namespace godot {

class InstrumentedProjectBuilder {
   public:
    /// Copies the project from 'res://' to a system temporary directory.
    /// Accepts options (e.g. "exclude" array) to skip instrumentation for specific paths.
    /// Returns the absolute path to the new project folder.
    static String build_instrumented_project(const Dictionary& options);  // Updated signature
};
}  // namespace godot