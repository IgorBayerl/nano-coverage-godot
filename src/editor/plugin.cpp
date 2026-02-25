#include "plugin.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include "../utils/logger.h"

#include "../config/settings_gateway.h"

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"),
                         &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_report_pressed"),
                         &NanoCoverageEditorPlugin::_on_generate_report_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_data_pressed"), &NanoCoverageEditorPlugin::_on_clear_data_pressed);
    ClassDB::bind_method(D_METHOD("_on_settings_changed"), &NanoCoverageEditorPlugin::_on_settings_changed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {
    coverage_api.instantiate();
}

NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {
}

void NanoCoverageEditorPlugin::_enter_tree() {
    // Register Project Setting
    SettingsGateway::register_settings();

    // Create "Run Instrumented" Button
    run_instrumented_button = memnew(Button);
    run_instrumented_button->set_text("Run Instrumented");
    run_instrumented_button->set_tooltip_text("Run the project in a temporary environment with coverage enabled");
    run_instrumented_button->connect("pressed", Callable(this, "_on_run_instrumented_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, run_instrumented_button);

    // Create "Generate Report" Button
    generate_report_button = memnew(Button);
    generate_report_button->set_text("Generate Report");
    generate_report_button->set_tooltip_text("Merge coverage data and generate lcov report");
    generate_report_button->connect("pressed", Callable(this, "_on_generate_report_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, generate_report_button);

    // Create "Clear Data" Button
    clear_data_button = memnew(Button);
    clear_data_button->set_text("Clear Data");
    clear_data_button->set_tooltip_text("Clear all collected coverage data");
    clear_data_button->connect("pressed", Callable(this, "_on_clear_data_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, clear_data_button);

    // Connect to settings changed
    ProjectSettings::get_singleton()->connect("settings_changed", Callable(this, "_on_settings_changed"));

    // Initial visibility update
    _update_visibility();
}

void NanoCoverageEditorPlugin::_exit_tree() {
    if (run_instrumented_button) {
        remove_control_from_container(CONTAINER_TOOLBAR, run_instrumented_button);
        run_instrumented_button->queue_free();
        run_instrumented_button = nullptr;
    }
    if (generate_report_button) {
        remove_control_from_container(CONTAINER_TOOLBAR, generate_report_button);
        generate_report_button->queue_free();
        generate_report_button = nullptr;
    }
    if (clear_data_button) {
        remove_control_from_container(CONTAINER_TOOLBAR, clear_data_button);
        clear_data_button->queue_free();
        clear_data_button = nullptr;
    }

    if (ProjectSettings::get_singleton()->is_connected("settings_changed", Callable(this, "_on_settings_changed"))) {
        ProjectSettings::get_singleton()->disconnect("settings_changed", Callable(this, "_on_settings_changed"));
    }

}

void NanoCoverageEditorPlugin::_update_visibility() {
    if (run_instrumented_button) {
        run_instrumented_button->set_visible(true);
    }
    if (generate_report_button) {
        generate_report_button->set_visible(true);
    }
    if (clear_data_button) {
        clear_data_button->set_visible(true);
    }
}

void NanoCoverageEditorPlugin::_on_settings_changed() {
    _update_visibility();
}

void NanoCoverageEditorPlugin::_on_run_instrumented_pressed() {
    Logger::info("Launching game with hot-patch coverage...");

    // Clear previous data
    if (coverage_api.is_valid()) {
        Dictionary opts;
        opts["workspace_id"] = "default";
        coverage_api->clear_coverage_data(opts);
    }

    // Launch Godot subprocess with our custom game_runner.gd
    PackedStringArray args;
    args.push_back("--path");
    
    // FIX: Convert res:// to absolute system path
    String absolute_path = ProjectSettings::get_singleton()->globalize_path("res://");
    args.push_back(absolute_path);
    
    args.push_back("--script");
    args.push_back("res://addons/nano_coverage_godot/game_runner.gd");

    int32_t pid = OS::get_singleton()->create_process(OS::get_singleton()->get_executable_path(), args);

    if (pid == -1) {
        Logger::error("Failed to launch game process.");
    } else {
        Logger::info("Process started with PID " + String::num_int64(pid));
    }
}


void NanoCoverageEditorPlugin::_on_generate_report_pressed() {
    if (coverage_api.is_null()) {
        Logger::error("API not initialized.");
        return;
    }

    Dictionary opts;
    opts["workspace_id"] = "default";

    Logger::info("Generating report...");
    Dictionary result = coverage_api->generate_coverage_report(opts);

    if (result.has("status") && String(result["status"]) == "ok") {
        Logger::info("Report generated at: " + String(result["report_path"]));
    } else {
        Logger::error("Report generation failed.");
    }
}

void NanoCoverageEditorPlugin::_on_clear_data_pressed() {
    if (coverage_api.is_null()) {
        return;
    }

    Dictionary opts;
    opts["workspace_id"] = "default";

    coverage_api->clear_coverage_data(opts);
    Logger::info("Coverage data cleared.");
}

}  // namespace godot