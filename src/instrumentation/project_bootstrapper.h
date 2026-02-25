#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

class ProjectBootstrapper : public RefCounted {
    GDCLASS(ProjectBootstrapper, RefCounted)

private:
    Array compile_ignore_patterns(const Array& glob_patterns);
    Array get_all_files(const String& current_path, const Array& compiled_regexes);

protected:
    static void _bind_methods();

public:
    void instrument_all_scripts();
};

} // namespace godot
