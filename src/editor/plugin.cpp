#include "plugin.h"

#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/script_editor_base.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "../config/settings_gateway.h"
#include "../utils/logger.h"
#include "../utils/path_utils.h"
#include "coverage_gutter.h"
#include "coverage_panel.h"

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"),
                         &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_report_pressed"),
                         &NanoCoverageEditorPlugin::_on_generate_report_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_data_pressed"), &NanoCoverageEditorPlugin::_on_clear_data_pressed);
    ClassDB::bind_method(D_METHOD("_on_settings_changed"), &NanoCoverageEditorPlugin::_on_settings_changed);
    ClassDB::bind_method(D_METHOD("_on_editor_script_changed"), &NanoCoverageEditorPlugin::_on_editor_script_changed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {
    coverage_api.instantiate();
}

NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {
}

void NanoCoverageEditorPlugin::_enter_tree() {
    // Register Project Settings
    SettingsGateway::register_settings();

    // Create toolbar buttons
    run_instrumented_button = memnew(Button);
    run_instrumented_button->set_text("Run Instrumented");
    run_instrumented_button->set_tooltip_text("Run the project in a temporary environment with coverage enabled");
    run_instrumented_button->connect("pressed", Callable(this, "_on_run_instrumented_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, run_instrumented_button);

    generate_report_button = memnew(Button);
    generate_report_button->set_text("Generate Report");
    generate_report_button->set_tooltip_text("Merge coverage data and generate lcov report");
    generate_report_button->connect("pressed", Callable(this, "_on_generate_report_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, generate_report_button);

    clear_data_button = memnew(Button);
    clear_data_button->set_text("Clear Data");
    clear_data_button->set_tooltip_text("Clear all collected coverage data");
    clear_data_button->connect("pressed", Callable(this, "_on_clear_data_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, clear_data_button);

    // Coverage metrics bottom panel
    coverage_panel = memnew(CoveragePanel);
    coverage_panel->setup();
    panel_button = add_control_to_bottom_panel(coverage_panel, "Coverage");

    // Connect to settings changed
    ProjectSettings::get_singleton()->connect("settings_changed", Callable(this, "_on_settings_changed"));

    // Connect to script tab changes for gutter updates
    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (script_editor) {
        script_editor->connect("editor_script_changed", Callable(this, "_on_editor_script_changed"));
    }

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

    if (coverage_panel) {
        remove_control_from_bottom_panel(coverage_panel);
        coverage_panel->queue_free();
        coverage_panel = nullptr;
        panel_button = nullptr;
    }

    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (script_editor && script_editor->is_connected("editor_script_changed", Callable(this, "_on_editor_script_changed"))) {
        script_editor->disconnect("editor_script_changed", Callable(this, "_on_editor_script_changed"));
    }

    if (ProjectSettings::get_singleton()->is_connected("settings_changed", Callable(this, "_on_settings_changed"))) {
        ProjectSettings::get_singleton()->disconnect("settings_changed", Callable(this, "_on_settings_changed"));
    }
}

void NanoCoverageEditorPlugin::_update_visibility() {
    CoverageSettings settings = SettingsGateway::load();

    if (run_instrumented_button) {
        run_instrumented_button->set_visible(settings.ui_show_run_instrumented);
    }
    if (generate_report_button) {
        generate_report_button->set_visible(settings.ui_show_generate_report);
    }
    if (clear_data_button) {
        clear_data_button->set_visible(settings.ui_show_clear_data);
    }
}

void NanoCoverageEditorPlugin::_on_settings_changed() {
    _update_visibility();
}

// --- Coverage display ---

void NanoCoverageEditorPlugin::refresh_gutters() {
    CoverageSettings settings = SettingsGateway::load();
    String lcov_path = settings.report_dir.path_join(settings.report_lcov_filename);
    String global_path = ProjectSettings::get_singleton()->globalize_path(lcov_path);

    cached_report = LcovParser::parse_file(global_path.utf8().get_data());
    update_active_editor_gutter();
}

void NanoCoverageEditorPlugin::update_active_editor_gutter() {
    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (!script_editor) return;

    ScriptEditorBase* current = script_editor->get_current_editor();
    if (!current) return;

    CodeEdit* code_edit = Object::cast_to<CodeEdit>(current->get_base_editor());
    if (!code_edit) return;

    Ref<Script> script = script_editor->get_current_script();
    if (script.is_null()) return;

    String res_path = script->get_path();
    std::string lcov_key = PathUtils::res_to_lcov_std(res_path);

    auto it = cached_report.files.find(lcov_key);
    if (it != cached_report.files.end()) {
        CoverageGutter::apply(code_edit, it->second);
    } else {
        CoverageGutter::clear(code_edit);
    }
}

void NanoCoverageEditorPlugin::refresh_metrics_panel() {
    if (coverage_panel) {
        coverage_panel->update_data(cached_report);
    }
}

void NanoCoverageEditorPlugin::_on_editor_script_changed(const Ref<Script>& script) {
    if (!cached_report.files.empty()) {
        update_active_editor_gutter();
    }
}

// --- Button handlers ---

void NanoCoverageEditorPlugin::_on_run_instrumented_pressed() {
    Logger::info("Launching game with hot-patch coverage...");

    PackedStringArray args;
    args.push_back("--path");
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
        refresh_gutters();
        refresh_metrics_panel();
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

    // Clear coverage display
    cached_report = LcovReport();
    update_active_editor_gutter();
    if (coverage_panel) {
        coverage_panel->clear_data();
    }
}

}  // namespace godot
