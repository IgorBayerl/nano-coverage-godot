#include "coverage_monitor.h"

#include <filesystem>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>  // <--- FIX: Required for UtilityFunctions::print
#include <mutex>

#include "../data/persistence.h"
#include "lcov_writer.h"
#include "../config/settings_gateway.h"

namespace godot {
namespace fs = std::filesystem;

namespace {
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
    collector.record_hit(file_path, line);
}

void NanoCoverage::reset() {
    collector.clear();
}

void NanoCoverage::save_session() {
    String out_dir_godot = get_output_dir();
    String global_out_dir = ProjectSettings::get_singleton()->globalize_path(out_dir_godot);
    fs::path data_path = fs::path(global_out_dir.utf8().get_data()) / "coverage.data";

    CoverageData snapshot = collector.snapshot();

    UtilityFunctions::print("NanoCoverage: Appending execution data to ", String(data_path.string().c_str()));
    if (!Persistence::append_execution_data(data_path.string(), snapshot)) {
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

    // Load Metadata (Coverable lines)
    CoverageMetadata meta;
    if (!Persistence::load_metadata(meta_path.string(), meta)) {
        UtilityFunctions::printerr("NanoCoverage: Report generation failed. Could not load ",
                                   String(meta_path.string().c_str()));
        return;
    }

    // Load Execution Data (Hits)
    CoverageData hits;
    // It's okay if this fails or is empty (0 coverage), but we try to load it.
    if (fs::exists(data_path)) {
        Persistence::load_and_merge_execution_data(data_path.string(), hits);
    }

    // Reconstruct full coverage data including 0s
    CoverageData final_data;

    for (const auto& meta_kv : meta) {
        const std::string& file_path = meta_kv.first;
        const std::vector<uint32_t>& coverable_lines = meta_kv.second;

        std::unordered_map<uint32_t, uint64_t>& file_lines = final_data[file_path];

        // Check if we have hits for this file
        auto hit_file_it = hits.find(file_path);
        
        for (uint32_t line : coverable_lines) {
            uint64_t count = 0;
            if (hit_file_it != hits.end()) {
                auto hit_line_it = hit_file_it->second.find(line);
                if (hit_line_it != hit_file_it->second.end()) {
                    count = hit_line_it->second;
                }
            }
            file_lines[line] = count;
        }
    }

    // Load Settings & Write LCOV
    CoverageSettings settings = SettingsGateway::load();
    
    LCOVWriter::write_lcov_report(final_data, settings);
}

int64_t NanoCoverage::get_total_hit_count() const {
    return (int64_t)collector.get_total_hits();
}

Dictionary NanoCoverage::get_snapshot() const {
    Dictionary out;
    CoverageData snapshot = collector.snapshot();

    for (const auto& f : snapshot) {
        Dictionary lines;
        for (const auto& l : f.second)
            lines[l.first] = (int64_t)l.second;
        out[String(f.first.c_str())] = lines;
    }
    return out;
}

}  // namespace godot