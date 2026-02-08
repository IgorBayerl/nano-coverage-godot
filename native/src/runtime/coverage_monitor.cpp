#include "coverage_monitor.h"

#include <filesystem>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>  // <--- FIX: Required for UtilityFunctions::print
#include <mutex>

#include "../data/persistence.h"
#include "lcov_writer.h"
#include "../config/settings_gateway.h"
#include "../instrumentation/instrumenter.h"
#include "../instrumentation/source_reader.h"
#include <godot_cpp/classes/dir_access.hpp>

namespace godot {
namespace fs = std::filesystem;

namespace {
String get_override_from_args(const String& key_prefix) {
    PackedStringArray user_args = OS::get_singleton()->get_cmdline_user_args();
    for (const String& arg : user_args) {
        if (arg.begins_with(key_prefix)) {
            return arg.substr(key_prefix.length());
        }
    }
    return "";
}

String get_output_dir() {
    String override = get_override_from_args("nano_coverage/output_dir=");
    if (!override.is_empty()) return override;

    if (ProjectSettings::get_singleton()->has_setting("nano_coverage/output_dir")) {
        String val = ProjectSettings::get_singleton()->get_setting("nano_coverage/output_dir");
        if (!val.is_empty()) return val;
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
    String out_filename = "coverage.data";
    
    String name_override = get_override_from_args("nano_coverage/output_name=");
    if (!name_override.is_empty()) {
        out_filename = name_override;
    } else if (ProjectSettings::get_singleton()->has_setting("nano_coverage/output_name")) {
        String val = ProjectSettings::get_singleton()->get_setting("nano_coverage/output_name");
        if (!val.is_empty()) out_filename = val;
    }
    fs::path data_path = fs::path(global_out_dir.utf8().get_data()) / out_filename.utf8().get_data();

    CoverageData snapshot = collector.snapshot();

    UtilityFunctions::print("NanoCoverage: Appending execution data to ", String(data_path.string().c_str()));
    
    // Ensure parent directory exists
    std::error_code ec;
    fs::create_directories(data_path.parent_path(), ec);
    if (ec) {
        UtilityFunctions::printerr("NanoCoverage: Failed to create session directory: ", String(ec.message().c_str()));
    }

    if (!Persistence::append_execution_data(data_path.string(), snapshot)) {
        UtilityFunctions::printerr("NanoCoverage: Failed to save session data.");
    }
}

void NanoCoverage::generate_report() {
    // Load Settings
    CoverageSettings settings = SettingsGateway::load();
    // UtilityFunctions::print("NanoCoverage: Report Dir Setting: ", settings.paths_report_dir);
    
    // 1. Load Metadata (PREVIOUSLY "Generate Metadata On-The-Fly")
    CoverageMetadata meta;
    
    String data_store_dir = settings.paths_data_store_dir;
    String global_data_dir = ProjectSettings::get_singleton()->globalize_path(data_store_dir);
    fs::path meta_path = fs::path(global_data_dir.utf8().get_data()) / "coverage.meta";
    
    if (fs::exists(meta_path)) {
        UtilityFunctions::print("NanoCoverage: Loading metadata from ", String(meta_path.string().c_str()));
        if (!Persistence::load_metadata(meta_path.string(), meta)) {
            UtilityFunctions::printerr("NanoCoverage: Failed to parse coverage.meta");
            return;
        }
    } else {
        UtilityFunctions::printerr("NanoCoverage: No metadata found at ", String(meta_path.string().c_str()), ". Did you run the instrumented project?");
        return;
    }

    // 2. Load Execution Data (Hits)
    // Execution Data is in output_dir (where the game wrote it)
    String out_dir_godot = get_output_dir();
    String global_out_dir = ProjectSettings::get_singleton()->globalize_path(out_dir_godot);
    fs::path data_root = fs::path(global_out_dir.utf8().get_data());
    fs::path data_path = data_root / "coverage.data";

    CoverageData hits;
    if (fs::exists(data_path)) {
        Persistence::load_and_merge_execution_data(data_path.string(), hits);
    }
    
    // 3. Merge Metadata and Hits
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

    // 4. Write LCOV Report
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