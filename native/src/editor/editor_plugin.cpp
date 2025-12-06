#include "nano_coverage/editor_plugin.hpp"
#include "nano_coverage/temp_project_builder.hpp" // Include the builder

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// ... (Binding and Constructor/Destructor remain the same) ...
void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"), &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {}
NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {}

void NanoCoverageEditorPlugin::_enter_tree() {
    run_instrumented_button = memnew(Button);
    run_instrumented_button->set_text("Run Instrumented");
    run_instrumented_button->set_tooltip_text("Run the project in a temporary environment");
    run_instrumented_button->connect("pressed", Callable(this, "_on_run_instrumented_pressed"));
    add_control_to_container(CONTAINER_TOOLBAR, run_instrumented_button);
}

void NanoCoverageEditorPlugin::_exit_tree() {
    if (run_instrumented_button) {
        remove_control_from_container(CONTAINER_TOOLBAR, run_instrumented_button);
        run_instrumented_button->queue_free();
        run_instrumented_button = nullptr;
    }
}

void NanoCoverageEditorPlugin::_on_run_instrumented_pressed() {
    UtilityFunctions::print("NanoCoverage: Preparing temporary project...");

    String temp_path = TempProjectBuilder::create_temp_project();
    
    if (temp_path.is_empty()) {
        UtilityFunctions::printerr("NanoCoverage: Aborting run (build failed).");
        return;
    }

    String godot_exe = OS::get_singleton()->get_executable_path();
    
    PackedStringArray args;
    args.append("--path");
    args.append(temp_path);
    
    // Add --verbose to help debug if it happens again (output appears in terminal running dev.py)
    args.append("--verbose"); 

    UtilityFunctions::print("NanoCoverage: Launching child process at: ", temp_path);
    
    OS::get_singleton()->create_process(godot_exe, args);
}

} // namespace godot