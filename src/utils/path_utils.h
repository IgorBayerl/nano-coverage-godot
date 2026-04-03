#pragma once

#include <godot_cpp/variant/string.hpp>
#include <string>

namespace godot {

class PathUtils {
public:
    // Convert LCOV relative path (e.g. "src/player.gd") to res:// path
    static String lcov_to_res(const String& lcov_path) {
        return "res://" + lcov_path;
    }

    // Convert res:// path to LCOV relative path (strips "res://")
    static String res_to_lcov(const String& res_path) {
        if (res_path.begins_with("res://")) {
            return res_path.substr(6);
        }
        return res_path;
    }

    // std::string variant for use with LcovReport
    static std::string res_to_lcov_std(const String& res_path) {
        return res_to_lcov(res_path).utf8().get_data();
    }
};

} // namespace godot
