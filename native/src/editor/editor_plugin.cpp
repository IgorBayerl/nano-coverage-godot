#include "nano_coverage/editor_plugin.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"), &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {
}

NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {
}

void NanoCoverageEditorPlugin::_enter_tree() {
    // Create the button
    run_instrumented_button = memnew(Button);
    run_instrumented_button->set_text("Run Instrumented");
    run_instrumented_button->set_tooltip_text("Run the project with code coverage instrumentation");
    
    // Connect the pressed signal
    run_instrumented_button->connect("pressed", Callable(this, "_on_run_instrumented_pressed"));

    // Add to the editor toolbar
    // CONTAINER_TOOLBAR is usually where play buttons are
    add_control_to_container(CONTAINER_TOOLBAR, run_instrumented_button);
    
    UtilityFunctions::print("NanoCoverageEditorPlugin initialized.");
}

void NanoCoverageEditorPlugin::_exit_tree() {
    if (run_instrumented_button) {
        remove_control_from_container(CONTAINER_TOOLBAR, run_instrumented_button);
        run_instrumented_button->queue_free();
        run_instrumented_button = nullptr;
    }
    UtilityFunctions::print("NanoCoverageEditorPlugin deinitialized.");
}

void NanoCoverageEditorPlugin::_on_run_instrumented_pressed() {
    UtilityFunctions::print("Run Instrumented button pressed!");
    // TODO: Implement instrumentation and run logic
}

} // namespace godot
