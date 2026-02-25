#pragma once
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

struct CoverageSettings {
    String data_store_dir;
    String report_dir;
    Array ignore_paths;
    String report_lcov_filename;
    bool use_absolute_paths;
};

class SettingsGateway {
public:
    static void register_settings();
    static CoverageSettings load();
};

} // namespace godot
