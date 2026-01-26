#include "lcov_writer.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void LCOVWriter::write_lcov_report(const String& output_path, const Dictionary& snapshot) {
    Ref<FileAccess> file = FileAccess::open(output_path, FileAccess::WRITE);
    if (file.is_null()) {
        UtilityFunctions::printerr("NanoCoverage: Could not open report file for writing: ", output_path);
        return;
    }

    UtilityFunctions::print("NanoCoverage: Writing LCOV report to ", output_path);

    // Snapshot keys are now Strings (File Paths) due to our recent update
    Array file_paths = snapshot.keys();

    for (int i = 0; i < file_paths.size(); i++) {
        String file_path = file_paths[i];
        Dictionary lines = snapshot[file_path];

        // 1. Write Source File (SF)
        file->store_line("SF:" + file_path);

        // 2. Write Data Lines (DA)
        Array line_nums = lines.keys();
        for (int j = 0; j < line_nums.size(); j++) {
            int line = line_nums[j];
            int64_t hits = lines[line];

            // Format: DA:line_number,hit_count
            file->store_line("DA:" + String::num_int64(line) + "," + String::num_int64(hits));
        }

        // 3. Close Record
        file->store_line("end_of_record");
    }

    file->close();
}

}  // namespace godot