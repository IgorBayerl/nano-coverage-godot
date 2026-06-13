#include "plugin.h"

#include <godot_cpp/classes/code_edit.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/script_editor_base.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "../config/settings_gateway.h"
#include "../instrumentation/disk_instrumenter.h"
#include "../runtime/coverage_monitor.h"
#include "../utils/logger.h"
#include "../utils/path_utils.h"
#include "coverage_gutter.h"
#include "coverage_panel.h"

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_instrument_toggled", "pressed"),
                         &NanoCoverageEditorPlugin::_on_instrument_toggled);
    ClassDB::bind_method(D_METHOD("_on_restore_pressed"), &NanoCoverageEditorPlugin::_on_restore_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_report_pressed"),
                         &NanoCoverageEditorPlugin::_on_generate_report_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_data_pressed"), &NanoCoverageEditorPlugin::_on_clear_data_pressed);
    ClassDB::bind_method(D_METHOD("_on_settings_changed"), &NanoCoverageEditorPlugin::_on_settings_changed);
    ClassDB::bind_method(D_METHOD("_on_editor_script_changed"), &NanoCoverageEditorPlugin::_on_editor_script_changed);
    ClassDB::bind_method(D_METHOD("_on_watch_tick"), &NanoCoverageEditorPlugin::_on_watch_tick);
    ClassDB::bind_method(D_METHOD("_on_toggle_gutters"), &NanoCoverageEditorPlugin::_on_toggle_gutters);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {
    coverage_api.instantiate();
}

NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {
}

void NanoCoverageEditorPlugin::_enter_tree() {
    // Register Project Settings
    SettingsGateway::register_settings();

    // Create toolbar buttons inside a shared container
    toolbar_button_container = memnew(HBoxContainer);

    instrument_toggle = memnew(CheckButton);
    instrument_toggle->set_text("Instrument");
    instrument_toggle->set_tooltip_text("Toggle disk instrumentation for manual coverage");
    instrument_toggle->connect("toggled", Callable(this, "_on_instrument_toggled"));
    toolbar_button_container->add_child(instrument_toggle);

    restore_button = memnew(Button);
    restore_button->set_tooltip_text("Restore original scripts from backup");
    restore_button->set_visible(false);
    restore_button->connect("pressed", Callable(this, "_on_restore_pressed"));
    toolbar_button_container->add_child(restore_button);

    generate_report_button = memnew(Button);
    generate_report_button->set_tooltip_text("Generate Report");
    generate_report_button->connect("pressed", Callable(this, "_on_generate_report_pressed"));
    toolbar_button_container->add_child(generate_report_button);

    clear_data_button = memnew(Button);
    clear_data_button->set_tooltip_text("Clear Data");
    clear_data_button->connect("pressed", Callable(this, "_on_clear_data_pressed"));
    toolbar_button_container->add_child(clear_data_button);

    // Place toolbar buttons (attempts to position near play buttons)
    _place_toolbar_buttons();

    // Toggle gutters button (CheckButton with label, placed in script editor status bar)
    toggle_gutters_button = memnew(CheckButton);
    toggle_gutters_button->set_text("Show Coverage");
    toggle_gutters_button->set_tooltip_text("Toggle coverage gutters in the script editor");
    toggle_gutters_button->set_pressed(true);
    toggle_gutters_button->connect("pressed", Callable(this, "_on_toggle_gutters"));
    // Will be placed in the status bar when the script editor is available
    _place_gutters_button_in_status_bar();

    // Coverage metrics bottom panel
    coverage_panel = memnew(CoveragePanel);
    coverage_panel->setup();
    panel_button = add_control_to_bottom_panel(coverage_panel, "Coverage");

    // File watcher timer (LCOV report + new coverage data)
    watch_timer = memnew(Timer);
    watch_timer->set_wait_time(2.0);
    watch_timer->set_autostart(false);
    watch_timer->connect("timeout", Callable(this, "_on_watch_tick"));
    add_child(watch_timer);

    // Connect to settings changed
    ProjectSettings::get_singleton()->connect("settings_changed", Callable(this, "_on_settings_changed"));

    // Connect to script tab changes for gutter updates
    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (script_editor) {
        script_editor->connect("editor_script_changed", Callable(this, "_on_editor_script_changed"));
    }

    _update_visibility();

    // State recovery: check if files are instrumented on disk
    _sync_instrument_state();

    // Start the file watcher if any watching feature is enabled
    _update_watch_timer(SettingsGateway::load());
}

void NanoCoverageEditorPlugin::_exit_tree() {
    // Warn if files are still instrumented on disk
    {
        Ref<DiskInstrumenter> di;
        di.instantiate();
        if (di->is_instrumented()) {
            Logger::warn("Files are still instrumented on disk. Remember to restore before committing.");
        }
    }

    if (watch_timer) {
        watch_timer->stop();
        watch_timer->queue_free();
        watch_timer = nullptr;
    }

    if (toolbar_button_container) {
        toolbar_button_container->queue_free();
        toolbar_button_container = nullptr;
        instrument_toggle = nullptr;
        restore_button = nullptr;
        generate_report_button = nullptr;
        clear_data_button = nullptr;
    }

    if (toggle_gutters_button) {
        toggle_gutters_button->queue_free();
        toggle_gutters_button = nullptr;
    }
    status_bar_ref = nullptr;

    if (coverage_panel) {
        remove_control_from_bottom_panel(coverage_panel);
        coverage_panel->queue_free();
        coverage_panel = nullptr;
        panel_button = nullptr;
    }

    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (script_editor &&
        script_editor->is_connected("editor_script_changed", Callable(this, "_on_editor_script_changed"))) {
        script_editor->disconnect("editor_script_changed", Callable(this, "_on_editor_script_changed"));
    }

    if (ProjectSettings::get_singleton()->is_connected("settings_changed", Callable(this, "_on_settings_changed"))) {
        ProjectSettings::get_singleton()->disconnect("settings_changed", Callable(this, "_on_settings_changed"));
    }
}

void NanoCoverageEditorPlugin::_update_visibility() {
    CoverageSettings settings = SettingsGateway::load();

    if (instrument_toggle) {
        instrument_toggle->set_visible(settings.ui_show_run_instrumented);
    }
    if (restore_button && !settings.ui_show_run_instrumented) {
        restore_button->set_visible(false);
    }
    if (generate_report_button) {
        generate_report_button->set_visible(settings.ui_show_generate_report);
    }
    if (clear_data_button) {
        clear_data_button->set_visible(settings.ui_show_clear_data);
    }
    if (panel_button) {
        panel_button->set_visible(settings.ui_show_coverage_panel);
    }
}

void NanoCoverageEditorPlugin::_on_settings_changed() {
    _update_visibility();
    _update_watch_timer(SettingsGateway::load());
}

void NanoCoverageEditorPlugin::_update_watch_timer(const CoverageSettings& settings) {
    if (!watch_timer)
        return;

    bool should_run = settings.watch_lcov_file || settings.auto_generate_report;
    if (should_run && watch_timer->is_stopped()) {
        watch_timer->start();
    } else if (!should_run && !watch_timer->is_stopped()) {
        watch_timer->stop();
    }
}

// --- UI placement helpers ---

void NanoCoverageEditorPlugin::_place_toolbar_buttons() {
    if (!toolbar_button_container)
        return;

    // Try to find the play/run bar and insert our container right before it.
    // In Godot 4.x the run bar is typically named "@EditorRunBar@..." inside the title bar.
    Control* base = EditorInterface::get_singleton()->get_base_control();
    if (!base) {
        // Fallback: add via standard container API
        add_control_to_container(CONTAINER_TOOLBAR, toolbar_button_container);
        return;
    }

    // The EditorRunBar is a direct or nested child of the base control's root.
    // Search for it by class name pattern.
    TypedArray<Node> run_bars = base->find_children("*", "EditorRunBar", true, false);
    if (run_bars.size() > 0) {
        Node* run_bar = Object::cast_to<Node>(run_bars[0]);
        Node* parent = run_bar->get_parent();
        if (parent) {
            parent->add_child(toolbar_button_container);
            // Move our container right before the run bar
            int run_bar_idx = run_bar->get_index();
            parent->move_child(toolbar_button_container, run_bar_idx);

            // Apply editor theme icons to buttons
            Ref<Texture2D> file_icon = base->get_theme_icon("File", "EditorIcons");
            Ref<Texture2D> clear_icon = base->get_theme_icon("Remove", "EditorIcons");
            Ref<Texture2D> reload_icon = base->get_theme_icon("Reload", "EditorIcons");

            if (reload_icon.is_valid() && restore_button)
                restore_button->set_button_icon(reload_icon);
            if (file_icon.is_valid() && generate_report_button)
                generate_report_button->set_button_icon(file_icon);
            if (clear_icon.is_valid() && clear_data_button)
                clear_data_button->set_button_icon(clear_icon);

            return;
        }
    }

    // Fallback: use standard toolbar container
    add_control_to_container(CONTAINER_TOOLBAR, toolbar_button_container);
}

void NanoCoverageEditorPlugin::_place_gutters_button_in_status_bar() {
    if (!toggle_gutters_button)
        return;

    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (!script_editor)
        return;

    ScriptEditorBase* current = script_editor->get_current_editor();
    if (!current)
        return;

    CodeEdit* code_edit = Object::cast_to<CodeEdit>(current->get_base_editor());
    if (!code_edit)
        return;

    // The CodeEdit sits inside a CodeTextEditor (VBoxContainer).
    // The status bar is an HBoxContainer that is a sibling below the CodeEdit.
    Node* code_text_editor = code_edit->get_parent();
    if (!code_text_editor)
        return;

    // Find the status bar HBoxContainer among siblings
    HBoxContainer* status_bar = nullptr;
    for (int i = code_text_editor->get_child_count() - 1; i >= 0; i--) {
        HBoxContainer* hbox = Object::cast_to<HBoxContainer>(code_text_editor->get_child(i));
        if (hbox) {
            status_bar = hbox;
            break;
        }
    }

    if (!status_bar)
        return;

    // Already placed in this status bar
    if (status_bar_ref == status_bar && toggle_gutters_button->get_parent() == status_bar)
        return;

    // Remove from previous parent if needed
    if (toggle_gutters_button->get_parent()) {
        toggle_gutters_button->get_parent()->remove_child(toggle_gutters_button);
    }

    status_bar->add_child(toggle_gutters_button);
    status_bar_ref = status_bar;
}

// --- Coverage display ---

void NanoCoverageEditorPlugin::refresh_gutters() {
    CoverageSettings settings = SettingsGateway::load();
    String lcov_path = settings.report_dir.path_join(settings.report_lcov_filename);
    String global_path = ProjectSettings::get_singleton()->globalize_path(lcov_path);

    cached_report = LcovParser::parse_file(global_path.utf8().get_data());
    if (gutters_enabled) {
        update_active_editor_gutter();
    }
}

void NanoCoverageEditorPlugin::update_active_editor_gutter() {
    ScriptEditor* script_editor = EditorInterface::get_singleton()->get_script_editor();
    if (!script_editor)
        return;

    ScriptEditorBase* current = script_editor->get_current_editor();
    if (!current)
        return;

    CodeEdit* code_edit = Object::cast_to<CodeEdit>(current->get_base_editor());
    if (!code_edit)
        return;

    // Clean up previous gutter instance
    if (active_gutter.is_valid()) {
        active_gutter->remove();
        active_gutter.unref();
    }

    Ref<Script> script = script_editor->get_current_script();
    if (script.is_null())
        return;

    String res_path = script->get_path();
    std::string lcov_key = PathUtils::res_to_lcov_std(res_path);

    auto it = cached_report.files.find(lcov_key);
    if (it != cached_report.files.end()) {
        active_gutter.instantiate();
        active_gutter->setup(code_edit, it->second);
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
    // Move the gutters toggle to the new editor's status bar
    _place_gutters_button_in_status_bar();

    if (gutters_enabled && !cached_report.files.empty()) {
        update_active_editor_gutter();
    }
}

void NanoCoverageEditorPlugin::_on_toggle_gutters() {
    gutters_enabled = toggle_gutters_button && toggle_gutters_button->is_pressed();

    if (gutters_enabled) {
        // Re-apply gutters if we have data
        if (!cached_report.files.empty()) {
            update_active_editor_gutter();
        }
    } else {
        // Remove active gutter
        if (active_gutter.is_valid()) {
            active_gutter->remove();
            active_gutter.unref();
        }
    }
}

// --- File watcher ---

void NanoCoverageEditorPlugin::_on_watch_tick() {
    CoverageSettings settings = SettingsGateway::load();

    // Auto-generate a report when new coverage data appears (e.g. an
    // instrumented Play session just ended).
    if (settings.auto_generate_report) {
        _check_auto_report(settings);
    }

    if (settings.watch_lcov_file) {
        String lcov_path = settings.report_dir.path_join(settings.report_lcov_filename);
        if (FileAccess::file_exists(lcov_path)) {
            uint64_t mod_time = FileAccess::get_modified_time(lcov_path);
            if (mod_time != lcov_last_modified) {
                lcov_last_modified = mod_time;
                Logger::info("LCOV file changed, refreshing coverage display...");
                refresh_gutters();
                refresh_metrics_panel();
            }
        }
    }
}

void NanoCoverageEditorPlugin::_check_auto_report(const CoverageSettings& settings) {
    String runs_dir = settings.data_store_dir.path_join("default").path_join("runs");

    Ref<DirAccess> da = DirAccess::open(runs_dir);
    if (da.is_null())
        return;

    uint64_t latest = 0;
    da->list_dir_begin();
    for (String name = da->get_next(); !name.is_empty(); name = da->get_next()) {
        if (da->current_is_dir() || !name.ends_with(".covdata"))
            continue;
        uint64_t mod_time = FileAccess::get_modified_time(runs_dir.path_join(name));
        if (mod_time > latest)
            latest = mod_time;
    }
    da->list_dir_end();

    if (latest == 0)
        return;

    // Data that already existed when the editor started doesn't trigger a
    // report; only changes observed while running do.
    if (!covdata_seeded) {
        covdata_seeded = true;
        covdata_last_modified = latest;
        return;
    }

    if (latest <= covdata_last_modified)
        return;
    covdata_last_modified = latest;

    Logger::info("New coverage data detected, generating report...");
    _generate_report();
}

// --- Disk instrumentation state ---

void NanoCoverageEditorPlugin::_sync_instrument_state() {
    Ref<DiskInstrumenter> di;
    di.instantiate();
    bool on_disk = di->is_instrumented();

    if (instrument_toggle) {
        instrument_toggle->set_pressed_no_signal(on_disk);
        instrument_toggle->set_text(on_disk ? "Instrumented" : "Instrument");
    }
    if (restore_button) {
        restore_button->set_visible(on_disk);
    }
}

// --- Button handlers ---

void NanoCoverageEditorPlugin::_on_instrument_toggled(bool pressed) {
    if (pressed) {
        Ref<DiskInstrumenter> di;
        di.instantiate();
        Dictionary result = di->instrument_to_disk();
        if (String(result["status"]) == "ok") {
            // Keep the editor's view of the scripts in sync with the
            // instrumented files on disk.
            EditorInterface::get_singleton()->get_resource_filesystem()->scan();
        } else {
            Logger::error("Instrumentation failed: " + String(result.get("error", "")));
            if (instrument_toggle) {
                instrument_toggle->set_pressed_no_signal(false);
            }
        }
    } else {
        _on_restore_pressed();
    }
    _sync_instrument_state();
}

void NanoCoverageEditorPlugin::_on_restore_pressed() {
    Ref<DiskInstrumenter> di;
    di.instantiate();
    Dictionary result = di->restore_from_disk();
    if (String(result["status"]) == "ok") {
        // Trigger editor script reload so the editor picks up restored files
        EditorInterface::get_singleton()->get_resource_filesystem()->scan();
    }
    _sync_instrument_state();
}

void NanoCoverageEditorPlugin::_on_generate_report_pressed() {
    _generate_report();
}

void NanoCoverageEditorPlugin::_generate_report() {
    if (coverage_api.is_null()) {
        Logger::error("API not initialized.");
        return;
    }

    // Flush any in-memory coverage data before generating
    if (Engine::get_singleton()->has_singleton("NanoCoverage")) {
        Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
        NanoCoverage* nc = Object::cast_to<NanoCoverage>(obj);
        if (nc && nc->get_total_hit_count() > 0) {
            nc->save_session();
            nc->reset();
        }
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
        Logger::error("Report generation failed: " + String(result.get("error", "Unknown error")));
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
    if (active_gutter.is_valid()) {
        active_gutter->remove();
        active_gutter.unref();
    }
    if (coverage_panel) {
        coverage_panel->clear_data();
    }
}

}  // namespace godot
