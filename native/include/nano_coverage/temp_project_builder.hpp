#ifndef NANO_COVERAGE_TEMP_PROJECT_BUILDER_HPP
#define NANO_COVERAGE_TEMP_PROJECT_BUILDER_HPP

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class TempProjectBuilder {
public:
    /// Copies the project from 'res://' to a system temporary directory.
    /// Returns the absolute path to the new project folder.
    static String create_temp_project();
};

} // namespace godot

#endif // NANO_COVERAGE_TEMP_PROJECT_BUILDER_HPP