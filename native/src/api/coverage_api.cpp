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
#include "../editor/temp_builder.h"
#include "../runtime/coverage_collector.h"
#include "../runtime/coverage_store.h"
#include "../runtime/lcov_writer.h"

namespace godot {

void CoverageApi::_bind_methods() {
    ClassDB::bind_method(D_METHOD("instrument_project", "options"), &CoverageApi::instrument_project);
    ClassDB::bind_method(D_METHOD("run_instrumented_project", "options"), &CoverageApi::run_instrumented_project);
    ClassDB::bind_method(D_METHOD("generate_coverage_report", "options"), &CoverageApi::generate_coverage_report);
    ClassDB::bind_method(D_METHOD("clear_coverage_data", "options"), &CoverageApi::clear_coverage_data);
}

Dictionary CoverageApi::instrument_project(const Dictionary& options) {
    String output_path = TempProjectBuilder::create_temp_project();
    Dictionary result;
    if (output_path.is_empty()) {
        result["error"] = "Failed to create temp project";
        return result;
    }

    result["output_path"] = output_path;
    return result;
}

Dictionary CoverageApi::run_instrumented_project(const Dictionary& options) {
    Dictionary result;
    if (!options.has("output_path")) {
        result["error"] = "Missing 'output_path' in options";
        return result;
    }

    String project_path = options["output_path"];
    String workspace_id = options.get("workspace_id", "default");
    bool blocking = options.get("blocking", false);

    // Generate a unique run ID
    int64_t time = (int64_t)std::time(nullptr);
    int64_t rnd = std::rand();
    String run_id = String::num_int64(time) + "-" + String::num_int64(rnd);

    String output_filename = run_id + ".covdata";

    // Load Settings
    CoverageSettings settings = SettingsGateway::load();
    String data_store_dir = settings.paths_data_store_dir;

    String data_store_base = ProjectSettings::get_singleton()->globalize_path(data_store_dir);
    String store_runs_dir = data_store_base + "/" + workspace_id + "/runs";

    // Ensure runs directory exists (also for logs)
    if (!DirAccess::dir_exists_absolute(store_runs_dir)) {
        DirAccess::make_dir_recursive_absolute(store_runs_dir);
    }

    // Define Log File Path
    String log_file_path = store_runs_dir + "/" + run_id + ".log";

    PackedStringArray args;
    args.push_back("--path");
    args.push_back(project_path);

    // Add Log File Argument (Must be before '++')
    args.push_back("--log-file");
    args.push_back(log_file_path);

    // Inject overrides
    // Use ++ separator to denote user arguments
    args.push_back("++");
    args.push_back("nano_coverage/output_dir=" + store_runs_dir);
    args.push_back("nano_coverage/output_name=" + output_filename);

    bool dry_run = options.get("dry_run", false);
    if (dry_run) {
        result["args"] = args;
        result["run_id"] = run_id;
        result["expected_output_file"] = store_runs_dir + "/" + output_filename;
        result["log_file"] = log_file_path;  // Return log path in dry run too
        return result;
    }

    if (blocking) {
        Array output;
        int32_t ret =
            OS::get_singleton()->execute(OS::get_singleton()->get_executable_path(), args, output, true, true);

        // In blocking mode, we can just pipe stdout directly if captured
        if (output.size() > 0) {
            UtilityFunctions::print(output[0]);
        }

        result["exit_code"] = Variant((int64_t)ret);
        result["run_id"] = run_id;
        result["output_file"] = store_runs_dir + "/" + output_filename;
        result["log_file"] = log_file_path;
        return result;
    }

    int32_t pid = OS::get_singleton()->create_process(OS::get_singleton()->get_executable_path(), args);
    if (pid == -1) {
        result["error"] = "Failed to create process";
        return result;
    }

    result["pid"] = Variant((int64_t)pid);
    result["run_id"] = run_id;
    result["output_file"] = store_runs_dir + "/" + output_filename;
    result["log_file"] = log_file_path;  // Pass log path so Plugin can tail it

    return result;
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