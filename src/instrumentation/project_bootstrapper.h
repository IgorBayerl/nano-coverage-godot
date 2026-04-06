#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/templates/hash_set.hpp>

namespace godot {

class ProjectBootstrapper : public RefCounted {
    GDCLASS(ProjectBootstrapper, RefCounted)

private:
    Array compile_ignore_patterns(const Array& glob_patterns);
    Array get_all_files(const String& current_path, const Array& compiled_regexes);
    HashSet<String> get_autoload_paths();
    void dump_failed_script(const String& original_path, const String& patched_code);

protected:
    static void _bind_methods();

public:
    void instrument_all_scripts();
};

} // namespace godot
