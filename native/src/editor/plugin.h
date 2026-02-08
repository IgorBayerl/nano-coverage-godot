#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/timer.hpp> 
#include "../api/coverage_api.h"

namespace godot {

class NanoCoverageEditorPlugin : public EditorPlugin {
    GDCLASS(NanoCoverageEditorPlugin, EditorPlugin)

   private:
    Ref<CoverageApi> coverage_api;
    Button* run_instrumented_button = nullptr;
    Button* generate_report_button = nullptr;
    Button* clear_data_button = nullptr;

    // Log Tailing State
    Timer* log_poll_timer = nullptr;
    String current_log_path;
    int64_t current_pid = -1;
    uint64_t log_read_pos = 0;

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

    // Called by Timer to read new log lines
    void _on_log_poll_timeout();
};

}  // namespace godot