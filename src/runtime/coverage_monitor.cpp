#include "coverage_monitor.h"

#include <filesystem>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "../utils/logger.h"
#include <mutex>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include "../instrumentation/instrumenter.h"
#include "../data/coverage_store.h"
namespace godot {
namespace fs = std::filesystem;



void NanoCoverage::_bind_methods() {
    ClassDB::bind_method(D_METHOD("hit", "file_path", "line"), &NanoCoverage::hit);
    ClassDB::bind_method(D_METHOD("save_session", "workspace_id"), &NanoCoverage::save_session, DEFVAL("default"));
    ClassDB::bind_method(D_METHOD("reset"), &NanoCoverage::reset);
    ClassDB::bind_method(D_METHOD("get_total_hit_count"), &NanoCoverage::get_total_hit_count);
}

void NanoCoverage::hit(String file_path, int32_t line) {
    collector.record_hit(file_path, line);
}

void NanoCoverage::reset() {
    collector.clear();
}

void NanoCoverage::save_session(const String& workspace_id) {
    CoverageSettings settings = SettingsGateway::load();
    String global_out_dir = ProjectSettings::get_singleton()->globalize_path(settings.data_store_dir);

    std::string ws_id_str = workspace_id.utf8().get_data();
    CoverageStore store(global_out_dir.utf8().get_data(), ws_id_str);
    CoverageData snapshot = collector.snapshot();

    Logger::info("Saving Session memory snapshot to workspace: " + workspace_id);
    store.append_run_snapshot("session_data", snapshot);
}

int64_t NanoCoverage::get_total_hit_count() const {
    return (int64_t)collector.get_total_hits();
}

}  // namespace godot
