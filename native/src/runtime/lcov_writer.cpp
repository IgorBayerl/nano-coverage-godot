#include "lcov_writer.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// CONFIGURATION: Hardcoded toggle
// true  = "C:/Projects/MyGame/script.gd"
// false = "res://script.gd"
const bool kUseAbsolutePath = true;

void LCOVWriter::write_lcov_report(const String& output_path, const Dictionary& snapshot) {
    Ref<FileAccess> file = FileAccess::open(output_path, FileAccess::WRITE);
    if (file.is_null()) {
        UtilityFunctions::printerr("NanoCoverage: Could not open report file for writing: ", output_path);
        return;
    }

    UtilityFunctions::print("NanoCoverage: Writing LCOV report to ", output_path);

    // Fetch the original project root if available (injected by TempBuilder)
    String source_root = "";
    if (kUseAbsolutePath) {
        if (ProjectSettings::get_singleton()->has_setting("nano_coverage/source_root")) {
            source_root = ProjectSettings::get_singleton()->get_setting("nano_coverage/source_root");
        }
    }

    Array file_paths = snapshot.keys();

    for (int i = 0; i < file_paths.size(); i++) {
        String file_path = file_paths[i];
        Dictionary lines = snapshot[file_path];

        // --- Path Resolution Logic ---
        String display_path = file_path;

        if (kUseAbsolutePath && !source_root.is_empty() && file_path.begins_with("res://")) {
            // Strip "res://" (6 chars) and join with source root
            String rel_path = file_path.substr(6);

            // Handle slash consistency
            if (source_root.ends_with("/")) {
                display_path = source_root + rel_path;
            } else {
                display_path = source_root + "/" + rel_path;
            }
        }
        // -----------------------------

        // 1. Write Source File (SF)
        file->store_line("SF:" + display_path);

        // 2. Write Data Lines (DA)
        Array line_nums = lines.keys();
        for (int j = 0; j < line_nums.size(); j++) {
            int line = line_nums[j];
            int64_t hits = lines[line];
            file->store_line("DA:" + String::num_int64(line) + "," + String::num_int64(hits));
        }

        // 3. Close Record
        file->store_line("end_of_record");
    }

    file->close();
}

}  // namespace godot