#include "coverage_api.h"

#include <cstdlib> // For rand
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp> // Explicitly include Variant

#include "../config/settings_gateway.h"
#include "../editor/temp_builder.h"
#include "../runtime/coverage_store.h"
#include "../runtime/lcov_writer.h"
#include "../runtime/coverage_collector.h"

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
    } else {
        result["output_path"] = output_path;
    }
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
    
    // Assume data store is in user://nano_coverage_data for now
    String data_store_base = ProjectSettings::get_singleton()->globalize_path("user://nano_coverage_data");
    String store_runs_dir = data_store_base + "/" + workspace_id + "/runs";
    
    
    PackedStringArray args;
    args.push_back("--path");
    args.push_back(project_path);
    
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
        return result;
    }

    if (blocking) {
         Array output;
         int32_t ret = OS::get_singleton()->execute(OS::get_singleton()->get_executable_path(), args, output, true, true);
         result["exit_code"] = Variant((int64_t)ret);
         result["run_id"] = run_id;
         result["output_file"] = store_runs_dir + "/" + output_filename;
    } else {
        // create_process returns int32_t definition of PID, or -1 on failure
        int32_t pid = OS::get_singleton()->create_process(OS::get_singleton()->get_executable_path(), args);
        
        if (pid == -1) {
            result["error"] = "Failed to create process";
            return result;
        }
        
        result["pid"] = Variant((int64_t)pid);
        result["run_id"] = run_id;
        result["output_file"] = store_runs_dir + "/" + output_filename;
    }

    return result;
}

Dictionary CoverageApi::generate_coverage_report(const Dictionary& options) {
    Dictionary result;
    
    String workspace_id = options.get("workspace_id", "default");
    
    String data_store_base = ProjectSettings::get_singleton()->globalize_path("user://nano_coverage_data");
    std::string base_str = data_store_base.utf8().get_data();
    std::string ws_id_str = workspace_id.utf8().get_data();

    CoverageStore store(base_str, ws_id_str);
    
    CoverageData data = store.load_and_merge();
    
    // We also need Settings.
    CoverageSettings settings = SettingsGateway::load();
    // Override report path if passed? For now stick to settings or default.
    
    // Write report
    LCOVWriter::write_lcov_report(data, settings);
    
    result["status"] = "ok";
    result["report_path"] = settings.paths_report_dir; 
    
    return result;
}

Dictionary CoverageApi::clear_coverage_data(const Dictionary& options) {
    Dictionary result;
    String workspace_id = options.get("workspace_id", "default");
    String data_store_base = ProjectSettings::get_singleton()->globalize_path("user://nano_coverage_data");
    
    CoverageStore store(data_store_base.utf8().get_data(), workspace_id.utf8().get_data());
    store.clear();
    
    result["status"] = "ok";
    return result;
}

}  // namespace godot
