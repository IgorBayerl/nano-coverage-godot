#include "lcov_writer.h"

#include <algorithm>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void LCOVWriter::write_lcov_report(const CoverageData& data, const CoverageSettings& settings) {
    // Construct output path
    String output_dir = settings.paths_report_dir;
    String filename = settings.report_lcov_filename;
    String output_path;

    if (output_dir.ends_with("/")) {
        output_path = output_dir + filename;
    } else {
        output_path = output_dir + "/" + filename;
    }

    // Ensure directory exists
    if (!DirAccess::dir_exists_absolute(output_dir)) {
        Error err = DirAccess::make_dir_recursive_absolute(output_dir);
        if (err != OK) {
            UtilityFunctions::printerr("NanoCoverage: Failed to create report directory: ", output_dir);
        }
    }

    Ref<FileAccess> file = FileAccess::open(output_path, FileAccess::WRITE);
    if (file.is_null()) {
        UtilityFunctions::printerr("NanoCoverage: Could not open report file for writing: ", output_path);
        return;
    }

    UtilityFunctions::print("NanoCoverage: Writing LCOV report to ", output_path);

    // Fetch the original project root if available (injected by TempBuilder)
    String source_root = "";
    if (settings.report_use_absolute_source_paths) {
        if (ProjectSettings::get_singleton()->has_setting("nano_coverage/source_root")) {
            source_root = ProjectSettings::get_singleton()->get_setting("nano_coverage/source_root");
            // Ensure source_root itself is absolute if it's a resource path
            if (source_root.begins_with("res://") || source_root.begins_with("user://")) {
                source_root = ProjectSettings::get_singleton()->globalize_path(source_root);
            }
        }
    }

    for (const auto& file_entry : data) {
        String file_path = String(file_entry.first.c_str());
        const auto& lines = file_entry.second;

        String display_path = file_path;

        if (settings.report_use_absolute_source_paths) {
            if (!source_root.is_empty() && file_path.begins_with("res://")) {
                // Strip "res://" (6 chars) and join with source root
                String rel_path = file_path.substr(6);

                // Handle slash consistency
                if (source_root.ends_with("/")) {
                    display_path = source_root + rel_path;
                } else {
                    display_path = source_root + "/" + rel_path;
                }
            } else if (file_path.begins_with("res://")) {
                // Fallback: globalize path if source_root is not set but we want absolute paths
                display_path = ProjectSettings::get_singleton()->globalize_path(file_path);
            }
        } else {
            // STANDARD RELATIVE PATH LOGIC: Strip "res://"
            // Turns "res://scripts/player.gd" into "scripts/player.gd"
            if (display_path.begins_with("res://")) {
                display_path = display_path.substr(6);
            }
        }

        // Write Test Name (TN)
        file->store_line("TN:");

        // Write Source File (SF)
        file->store_line("SF:" + display_path);

        uint32_t lf = 0;
        uint32_t lh = 0;

        // Sort lines to guarantee LCOV spec compliance!
        std::vector<std::pair<uint32_t, uint64_t>> sorted_lines(lines.begin(), lines.end());
        std::sort(sorted_lines.begin(), sorted_lines.end());

        // Write Data Lines (DA)
        for (const auto& line_entry : sorted_lines) {
            uint32_t line = line_entry.first;
            uint64_t hits = line_entry.second;

            lf++;
            if (hits > 0) {
                lh++;
            }

            file->store_line("DA:" + String::num_int64(line) + "," + String::num_int64(hits));
        }

        // Write Line Totals (LF = Lines Found, LH = Lines Hit)
        file->store_line("LF:" + String::num_int64(lf));
        file->store_line("LH:" + String::num_int64(lh));

        // Close Record
        file->store_line("end_of_record");
    }

    file->close();
}

}  // namespace godot