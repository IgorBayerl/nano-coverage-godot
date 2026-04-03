#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/timer.hpp>

#include "../api/coverage_api.h"
#include "../reporting/lcov_parser.h"
#include "coverage_gutter.h"

namespace godot {

class CoveragePanel;

class NanoCoverageEditorPlugin : public EditorPlugin {
    GDCLASS(NanoCoverageEditorPlugin, EditorPlugin)

   private:
    Ref<CoverageApi> coverage_api;

    // Toolbar buttons (placed near play buttons)
    HBoxContainer* toolbar_button_container = nullptr;
    Button* run_instrumented_button = nullptr;
    Button* generate_report_button = nullptr;
    Button* clear_data_button = nullptr;

    // Coverage display
    CoveragePanel* coverage_panel = nullptr;
    Button* panel_button = nullptr;
    CheckButton* toggle_gutters_button = nullptr;
    HBoxContainer* status_bar_ref = nullptr;
    bool gutters_enabled = true;
    LcovReport cached_report;
    Ref<CoverageGutter> active_gutter;

    // LCOV file watcher
    Timer* lcov_watch_timer = nullptr;
    uint64_t lcov_last_modified = 0;

    // Game process polling
    Timer* process_poll_timer = nullptr;
    int32_t game_pid = -1;

    void refresh_gutters();
    void update_active_editor_gutter();
    void refresh_metrics_panel();
    void _place_gutters_button_in_status_bar();
    void _place_toolbar_buttons();

   protected:
    static void _bind_methods();
    void _update_visibility();

   public:
    NanoCoverageEditorPlugin();
    ~NanoCoverageEditorPlugin();

    void _enter_tree() override;
    void _exit_tree() override;

    void _on_run_instrumented_pressed();
    void _on_generate_report_pressed();
    void _on_clear_data_pressed();
    void _on_settings_changed();
    void _on_editor_script_changed(const Ref<Script>& script);
    void _on_lcov_watch_tick();
    void _on_poll_game_process();
    void _on_toggle_gutters();
};

}  // namespace godot
