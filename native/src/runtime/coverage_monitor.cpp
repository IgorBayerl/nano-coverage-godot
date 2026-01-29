#include "coverage_monitor.h"

#include <filesystem>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>  // <--- FIX: Required for UtilityFunctions::print
#include <mutex>

#include "../data/persistence.h"
#include "lcov_writer.h"

namespace godot {
namespace fs = std::filesystem;

namespace {
struct Collector {
    mutable std::mutex mtx;
    // Map: FilePath -> (Line -> Hits)
    CoverageData data;
    uint64_t total_hits = 0;

    void clear_locked() {
        data.clear();
        total_hits = 0;
    }
};

Collector& collector() {
    static Collector c;
    return c;
}

String get_output_dir() {
    if (ProjectSettings::get_singleton()->has_setting("nano_coverage/output_dir")) {
        return ProjectSettings::get_singleton()->get_setting("nano_coverage/output_dir");
    }
    return "res://";
}
}  // namespace

void NanoCoverage::_bind_methods() {
    ClassDB::bind_method(D_METHOD("hit", "file_path", "line"), &NanoCoverage::hit);
    ClassDB::bind_method(D_METHOD("save_session"), &NanoCoverage::save_session);
    ClassDB::bind_method(D_METHOD("generate_report"), &NanoCoverage::generate_report);
    ClassDB::bind_method(D_METHOD("reset"), &NanoCoverage::reset);
    ClassDB::bind_method(D_METHOD("get_total_hit_count"), &NanoCoverage::get_total_hit_count);
    ClassDB::bind_method(D_METHOD("get_snapshot"), &NanoCoverage::get_snapshot);
}

void NanoCoverage::hit(String file_path, int32_t line) {
    if (file_path.is_empty() || line <= 0)
        return;

    std::string path_std = file_path.utf8().get_data();

    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);

    c.data[path_std][line]++;
    c.total_hits++;
}

void NanoCoverage::reset() {
    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);
    c.clear_locked();
}

void NanoCoverage::save_session() {
    String out_dir_godot = get_output_dir();
    String global_out_dir = ProjectSettings::get_singleton()->globalize_path(out_dir_godot);
    fs::path data_path = fs::path(global_out_dir.utf8().get_data()) / "coverage.data";

    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);

    UtilityFunctions::print("NanoCoverage: Appending execution data to ", String(data_path.string().c_str()));
    if (!Persistence::append_execution_data(data_path.string(), c.data)) {
        UtilityFunctions::printerr("NanoCoverage: Failed to save session data.");
    }
}

void NanoCoverage::generate_report() {
    String out_dir_godot = get_output_dir();
    String global_out_dir = ProjectSettings::get_singleton()->globalize_path(out_dir_godot);
    fs::path root = fs::path(global_out_dir.utf8().get_data());

    fs::path meta_path = root / "coverage.meta";
    fs::path data_path = root / "coverage.data";
    fs::path lcov_path = root / "coverage.lcov";

    // 1. Load Metadata (Coverable lines)
    CoverageMetadata meta;
    if (!Persistence::load_metadata(meta_path.string(), meta)) {
        UtilityFunctions::printerr("NanoCoverage: Report generation failed. Could not load ",
                                   String(meta_path.string().c_str()));
        return;
    }

    // 2. Load Execution Data (Hits)
    CoverageData hits;
    // It's okay if this fails or is empty (0 coverage), but we try to load it.
    if (fs::exists(data_path)) {
        Persistence::load_and_merge_execution_data(data_path.string(), hits);
    }

    // 3. Merge: Create the dictionary structure expected by LCOVWriter
    // Structure: { "res://file.gd": { line_int: hits_int } }
    Dictionary final_snap;

    for (const auto& meta_kv : meta) {
        const std::string& file_path = meta_kv.first;
        const std::vector<uint32_t>& coverable_lines = meta_kv.second;

        Dictionary file_lines;

        // Check if we have hits for this file
        auto hit_file_it = hits.find(file_path);

        for (uint32_t line : coverable_lines) {
            uint64_t count = 0;

            // If we have hits for this file, look up the line
            if (hit_file_it != hits.end()) {
                auto hit_line_it = hit_file_it->second.find(line);
                if (hit_line_it != hit_file_it->second.end()) {
                    count = hit_line_it->second;
                }
            }
            file_lines[line] = static_cast<int64_t>(count);
        }

        final_snap[String(file_path.c_str())] = file_lines;
    }

    // 4. Write LCOV
    String final_lcov_str = String(lcov_path.string().c_str());
    LCOVWriter::write_lcov_report(final_lcov_str, final_snap);
    UtilityFunctions::print("NanoCoverage: Report generated at ", final_lcov_str);
}

int64_t NanoCoverage::get_total_hit_count() const {
    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);
    return (int64_t)c.total_hits;
}

Dictionary NanoCoverage::get_snapshot() const {
    Dictionary out;
    Collector& c = collector();
    std::lock_guard<std::mutex> lock(c.mtx);
    for (const auto& f : c.data) {
        Dictionary lines;
        for (const auto& l : f.second)
            lines[l.first] = (int64_t)l.second;
        out[String(f.first.c_str())] = lines;
    }
    return out;
}

}  // namespace godot