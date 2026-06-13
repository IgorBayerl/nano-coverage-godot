#include "script_scanner.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>

#include "../utils/logger.h"

namespace godot {

Array ScriptScanner::build_ignore_globs(const CoverageSettings& settings) {
    Array globs;
    for (int i = 0; i < settings.ignore_paths.size(); ++i) {
        globs.push_back(settings.ignore_paths[i]);
    }

    if (settings.ignore_addons) {
        globs.push_back("res://addons/**");
    } else {
        // Never instrument our own addon code.
        globs.push_back("res://addons/nano_coverage_godot/**");
    }
    return globs;
}

Array ScriptScanner::compile_ignore_patterns(const Array& glob_patterns) {
    Array compiled_regexes;
    for (int i = 0; i < glob_patterns.size(); ++i) {
        String glob = glob_patterns[i];

        String regex_str = glob.replace(".", "\\.");
        regex_str = regex_str.replace("**", "<<GLOBSTAR>>");
        regex_str = regex_str.replace("*", "[^/]*");
        regex_str = regex_str.replace("<<GLOBSTAR>>", ".*");
        regex_str = "^" + regex_str + "$";

        Ref<RegEx> rx = RegEx::create_from_string(regex_str);
        if (rx.is_valid()) {
            compiled_regexes.push_back(rx);
        }
    }
    return compiled_regexes;
}

Array ScriptScanner::find_gd_files(const String& root_path, const Array& compiled_regexes) {
    Array files;
    Ref<DirAccess> dir = DirAccess::open(root_path);
    if (dir.is_null()) {
        Logger::error("ScriptScanner failed to open directory: " + root_path);
        return files;
    }

    dir->list_dir_begin();
    String file_name = dir->get_next();

    while (!file_name.is_empty()) {
        if (file_name == "." || file_name == "..") {
            file_name = dir->get_next();
            continue;
        }

        String full_path = root_path.path_join(file_name);
        bool is_ignored = false;

        for (int i = 0; i < compiled_regexes.size(); ++i) {
            Ref<RegEx> rx = compiled_regexes[i];
            if (rx->search(full_path).is_valid()) {
                is_ignored = true;
                break;
            }
        }

        if (is_ignored) {
            file_name = dir->get_next();
            continue;
        }

        if (dir->current_is_dir()) {
            files.append_array(find_gd_files(full_path, compiled_regexes));
        } else if (file_name.ends_with(".gd")) {
            files.append(full_path);
        }

        file_name = dir->get_next();
    }
    return files;
}

Array ScriptScanner::scan_project(const CoverageSettings& settings) {
    Array compiled = compile_ignore_patterns(build_ignore_globs(settings));
    return find_gd_files("res://", compiled);
}

}  // namespace godot
