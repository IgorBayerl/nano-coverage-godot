#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot {

class NanoCoverageEditorPlugin : public EditorPlugin {
    GDCLASS(NanoCoverageEditorPlugin, EditorPlugin)

   private:
    Button* run_instrumented_button = nullptr;
    Button* generate_report_button = nullptr;

   protected:
    static void _bind_methods();

   public:
    NanoCoverageEditorPlugin();
    ~NanoCoverageEditorPlugin();

    void _enter_tree() override;
    void _exit_tree() override;

    void _on_run_instrumented_pressed();
    void _on_generate_report_pressed();
};

}  // namespace godot