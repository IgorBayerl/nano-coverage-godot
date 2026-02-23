#include "coverage_api.h"

#include <cstdlib>  // For rand
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include "../instrumentation/instrumenter.h"
#include "../runtime/coverage_collector.h"
#include "../runtime/coverage_store.h"
#include "../runtime/lcov_writer.h"

namespace godot {

void CoverageApi::_bind_methods() {
    ClassDB::bind_method(D_METHOD("instrument_script", "source_code", "file_path"), &CoverageApi::instrument_script);
    ClassDB::bind_method(D_METHOD("save_static_metadata"), &CoverageApi::save_static_metadata);
    ClassDB::bind_method(D_METHOD("generate_coverage_report", "options"), &CoverageApi::generate_coverage_report);
    ClassDB::bind_method(D_METHOD("clear_coverage_data", "options"), &CoverageApi::clear_coverage_data);
}

Dictionary CoverageApi::instrument_script(const String& source_code, const String& file_path) {
    Dictionary result;

    std::string res_path_std = file_path.utf8().get_data();
    std::string source_std = source_code.utf8().get_data();

    godot::InstrumentResult inst_res = Instrumenter::instrument_text(source_std, res_path_std);

    if (!inst_res.ok) {
        result["success"] = false;
        result["error"] = String(inst_res.error_message.c_str());
        return result;
    }

    // Accumulate metadata
    session_metadata[res_path_std] = inst_res.covered_lines;

    Array covered_lines_arr;
    for (uint32_t line : inst_res.covered_lines) {
        covered_lines_arr.push_back((int)line);
    }

    result["success"] = true;
    result["code"] = String(inst_res.instrumented_code.c_str());
    result["lines"] = covered_lines_arr;

    return result;
}

void CoverageApi::save_static_metadata() {
    CoverageSettings settings = SettingsGateway::load();
    String data_store_dir = settings.paths_data_store_dir;
    String global_data_dir = ProjectSettings::get_singleton()->globalize_path(data_store_dir);

    if (!DirAccess::dir_exists_absolute(global_data_dir)) {
        DirAccess::make_dir_recursive_absolute(global_data_dir);
    }

    String meta_path_godot = global_data_dir.path_join("coverage.meta");
    Persistence::save_metadata(meta_path_godot.utf8().get_data(), session_metadata);
}


Dictionary CoverageApi::generate_coverage_report(const Dictionary& options) {
    Dictionary result;

    String workspace_id = options.get("workspace_id", "default");

    CoverageSettings settings = SettingsGateway::load();
    String data_store_dir = settings.paths_data_store_dir;

    String data_store_base = ProjectSettings::get_singleton()->globalize_path(data_store_dir);

    // Convert to std::string for CoverageStore
    std::string base_str = data_store_base.utf8().get_data();
    std::string ws_id_str = workspace_id.utf8().get_data();

    // Load Execution Hits
    CoverageStore store(base_str, ws_id_str);
    CoverageData raw_hits = store.load_and_merge();

    // Load Static Metadata (Coverable Lines)
    CoverageMetadata meta;

    // Construct path using Godot String to handle separators gracefully on all OS
    String meta_path_godot = data_store_base.path_join("coverage.meta");

    // Use Persistence directly (std::ifstream) to check/load, avoiding FileAccess inconsistency with absolute paths
    if (!Persistence::load_metadata(meta_path_godot.utf8().get_data(), meta)) {
        UtilityFunctions::printerr("NanoCoverage: Failed to load coverage.meta from ", meta_path_godot);
        result["error"] = "Metadata file missing or unreadable. Did you run instrumentation?";
        return result;
    }

    // Merge Hits into Metadata
    CoverageData final_data;

    for (const auto& meta_kv : meta) {
        const std::string& file_path = meta_kv.first;
        const std::vector<uint32_t>& coverable_lines = meta_kv.second;

        std::unordered_map<uint32_t, uint64_t>& file_lines = final_data[file_path];

        auto hit_file_it = raw_hits.find(file_path);
        bool has_hits = (hit_file_it != raw_hits.end());

        for (uint32_t line : coverable_lines) {
            uint64_t count = 0;

            if (has_hits) {
                auto hit_line_it = hit_file_it->second.find(line);
                if (hit_line_it != hit_file_it->second.end()) {
                    count = hit_line_it->second;
                }
            }
            // Assign count (0 if not found, >0 if found)
            file_lines[line] = count;
        }
    }

    // Write Report
    LCOVWriter::write_lcov_report(final_data, settings);

    result["status"] = "ok";
    result["report_path"] = settings.paths_report_dir;

    return result;
}

Dictionary CoverageApi::clear_coverage_data(const Dictionary& options) {
    Dictionary result;
    String workspace_id = options.get("workspace_id", "default");
    CoverageSettings settings = SettingsGateway::load();
    String data_store_dir = settings.paths_data_store_dir;

    String data_store_base = ProjectSettings::get_singleton()->globalize_path(data_store_dir);

    CoverageStore store(data_store_base.utf8().get_data(), workspace_id.utf8().get_data());
    store.clear();

    result["status"] = "ok";
    return result;
}

}  // namespace godot