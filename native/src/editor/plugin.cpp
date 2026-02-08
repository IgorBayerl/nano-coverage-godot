#include "plugin.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../config/settings_gateway.h"

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"),
                         &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_report_pressed"),
                         &NanoCoverageEditorPlugin::_on_generate_report_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_data_pressed"), &NanoCoverageEditorPlugin::_on_clear_data_pressed);
    ClassDB::bind_method(D_METHOD("_on_settings_changed"), &NanoCoverageEditorPlugin::_on_settings_changed);
    ClassDB::bind_method(D_METHOD("_on_log_poll_timeout"), &NanoCoverageEditorPlugin::_on_log_poll_timeout);
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

    // Log Timer
    log_poll_timer = memnew(Timer);
    log_poll_timer->set_wait_time(0.5);  // Check every 500ms
    log_poll_timer->set_one_shot(false);
    log_poll_timer->connect("timeout", Callable(this, "_on_log_poll_timeout"));
    add_child(log_poll_timer);  // Add to tree so it processes

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

    if (log_poll_timer) {
        log_poll_timer->stop();
        log_poll_timer->queue_free();
        log_poll_timer = nullptr;
    }
}

void NanoCoverageEditorPlugin::_update_visibility() {
    CoverageSettings settings = SettingsGateway::load();

    bool show_all = settings.ui_show_all_buttons;

    if (run_instrumented_button) {
        run_instrumented_button->set_visible(show_all || settings.ui_show_run_instrumented_button);
    }
    if (generate_report_button) {
        generate_report_button->set_visible(show_all || settings.ui_show_generate_report_button);
    }
    if (clear_data_button) {
        clear_data_button->set_visible(show_all || settings.ui_show_clear_data_button);
    }
}

void NanoCoverageEditorPlugin::_on_settings_changed() {
    _update_visibility();
}

void NanoCoverageEditorPlugin::_on_run_instrumented_pressed() {
    UtilityFunctions::print("NanoCoverage: Preparing temporary project...");

    if (coverage_api.is_null()) {
        UtilityFunctions::printerr("NanoCoverage: API not initialized.");
        return;
    }

    // Stop previous logging if active
    if (log_poll_timer->is_stopped() == false) {
        log_poll_timer->stop();
    }

    Dictionary instr_opts;
    Dictionary instr_result = coverage_api->instrument_project(instr_opts);

    if (instr_result.has("error")) {
        UtilityFunctions::printerr("NanoCoverage: Instrumentation failed: ", instr_result["error"]);
        return;
    }

    String output_path = instr_result["output_path"];

    Dictionary run_opts;
    run_opts["output_path"] = output_path;
    run_opts["workspace_id"] = "default";
    run_opts["blocking"] = false;  // Non-blocking so we can tail logs

    UtilityFunctions::print("NanoCoverage: Launching instrumented project...");
    Dictionary run_result = coverage_api->run_instrumented_project(run_opts);

    if (run_result.has("error")) {
        UtilityFunctions::printerr("NanoCoverage: Run failed: ", run_result["error"]);
        return;
    }

    UtilityFunctions::print("NanoCoverage: Project running. Run ID: ", run_result["run_id"]);

    // Setup Log Tailing
    if (run_result.has("log_file") && run_result.has("pid")) {
        current_log_path = run_result["log_file"];
        current_pid = run_result["pid"];
        log_read_pos = 0;

        UtilityFunctions::print("NanoCoverage: Tailing log file: ", current_log_path);
        log_poll_timer->start();
    }
}

void NanoCoverageEditorPlugin::_on_log_poll_timeout() {
    if (current_log_path.is_empty())
        return;

    Ref<FileAccess> f = FileAccess::open(current_log_path, FileAccess::READ);
    if (f.is_valid()) {
        // Seek to where we last left off
        f->seek(log_read_pos);

        while (f->get_position() < f->get_length()) {
            String line = f->get_line();
            // Prefix to distinguish game logs from editor logs
            UtilityFunctions::print("[Game] ", line);
        }

        log_read_pos = f->get_position();
        f->close();
    }

    // Check if process is still alive
    if (current_pid != -1) {
        if (!OS::get_singleton()->is_process_running(current_pid)) {
            UtilityFunctions::print("NanoCoverage: Game process finished.");
            log_poll_timer->stop();
            current_pid = -1;
        }
    }
}

void NanoCoverageEditorPlugin::_on_generate_report_pressed() {
    if (coverage_api.is_null()) {
        UtilityFunctions::printerr("NanoCoverage: API not initialized.");
        return;
    }

    Dictionary opts;
    opts["workspace_id"] = "default";

    UtilityFunctions::print("NanoCoverage: Generating report...");
    Dictionary result = coverage_api->generate_coverage_report(opts);

    if (result.has("status") && String(result["status"]) == "ok") {
        UtilityFunctions::print("NanoCoverage: Report generated at: ", result["report_path"]);
    } else {
        UtilityFunctions::printerr("NanoCoverage: Report generation failed.");
    }
}

void NanoCoverageEditorPlugin::_on_clear_data_pressed() {
    if (coverage_api.is_null()) {
        return;
    }

    Dictionary opts;
    opts["workspace_id"] = "default";

    coverage_api->clear_coverage_data(opts);
    UtilityFunctions::print("NanoCoverage: Coverage data cleared.");
}

}  // namespace godot