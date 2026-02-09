#pragma once

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class InstrumentedProjectBuilder {
   public:
    /// Copies the project from 'res://' to a system temporary directory.
    /// Returns the absolute path to the new project folder.
    static String build_instrumented_project();
};

}  // namespace godot
