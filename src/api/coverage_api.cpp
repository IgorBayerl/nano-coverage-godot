#include "coverage_api.h"

#include <cstdlib>  // For rand
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include "../utils/logger.h"
#include <godot_cpp/variant/variant.hpp>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include "../instrumentation/instrumenter.h"
#include "../runtime/coverage_collector.h"
#include "../data/coverage_store.h"
#include "../reporting/report_generator.h"

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

    // --- Ignore files with 0 coverable lines ---
    // TODO: Fix, we should have a struct to hold this info
    if (inst_res.covered_lines.empty()) {
        result["success"] = true;
        result["code"] = source_code; // Return the original, unmodified code
        result["lines"] = Array();
        result["ignored"] = true;     // Flag to tell GDScript not to reload it
        return result;
    }

    // Accumulate metadata (only for files with > 0 coverable lines)
    session_metadata[res_path_std] = inst_res.covered_lines;

    Array covered_lines_arr;
    for (uint32_t line : inst_res.covered_lines) {
        covered_lines_arr.push_back((int)line);
    }

    result["success"] = true;
    result["code"] = String(inst_res.instrumented_code.c_str());
    result["lines"] = covered_lines_arr;
    result["ignored"] = false;

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
    return ReportGenerator::generate(options);
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