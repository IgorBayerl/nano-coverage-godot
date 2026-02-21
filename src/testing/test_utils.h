#ifndef NANO_COVERAGE_TEST_UTILS_H
#define NANO_COVERAGE_TEST_UTILS_H

#include <filesystem>
#include <godot_cpp/classes/dir_access.hpp>  // Added for directory operations
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/godot.hpp>  // Ensure core types are available
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// RAII helper to automatically restore settings
class SettingsOverride {
    String key;
    Variant old_value;
    bool existed;

   public:
    SettingsOverride(const String& p_key, const Variant& p_value) {
        key = p_key;
        ProjectSettings* ps = ProjectSettings::get_singleton();
        if (ps->has_setting(key)) {
            old_value = ps->get_setting(key);
            existed = true;
        } else {
            existed = false;
        }
        ps->set_setting(key, p_value);
    }

    ~SettingsOverride() {
        ProjectSettings* ps = ProjectSettings::get_singleton();
        if (existed) {
            ps->set_setting(key, old_value);
        } else {
            // Reverting to "undefined" is tricky in Godot, setting to null/empty is best approximation
            ps->set_setting(key, Variant());
        }
    }
};

class TestUtils {
   public:
    // Recursively removes a directory using Godot's paths (res://...) converted to global paths
    // Uses std::error_code to avoid exceptions since godot-cpp builds with -fno-exceptions
    static void clean_dir(const String& res_path) {
        String global_path = ProjectSettings::get_singleton()->globalize_path(res_path);
        std::string path_str = global_path.utf8().get_data();

        std::error_code ec;
        if (std::filesystem::exists(path_str, ec)) {
            std::filesystem::remove_all(path_str, ec);
            if (ec) {
                UtilityFunctions::printerr("TestUtils: Failed to cleanup ", res_path,
                                           ". Error: ", String(ec.message().c_str()));
            }
        }
    }
};

}  // namespace godot

#endif  // NANO_COVERAGE_TEST_UTILS_H