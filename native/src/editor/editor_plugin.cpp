#include "nano_coverage/editor_plugin.hpp"
#include "nano_coverage/temp_project_builder.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/global_constants.hpp> 

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"), &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {}
NanoCoverageEditorPlugin::~NanoCoverageEditorPlugin() {}

void NanoCoverageEditorPlugin::_enter_tree() {
    // --- 1. Register Project Setting ---
    ProjectSettings *ps = ProjectSettings::get_singleton();
    String setting_path = "nano_coverage/general/temp_directory";

    // Define the default value (Empty string = System Temp Directory)
    if (!ps->has_setting(setting_path)) {
        ps->set_setting(setting_path, ""); 
    }
    
    // Register the default value so the "Revert" icon appears if changed
    ps->set_initial_value(setting_path, "");

    // Define how it looks in the UI
    Dictionary property_info;
    property_info["name"] = setting_path;
    property_info["type"] = Variant::STRING;
    
    // PROPERTY_HINT_GLOBAL_DIR allows selecting absolute paths on the disk (C:/...)
    // PROPERTY_HINT_DIR forces paths inside res://
    property_info["hint"] = PROPERTY_HINT_GLOBAL_DIR; 
    
    // The hint string acts as the description/title for the folder picker dialog
    property_info["hint_string"] = "Folder to store the instrumented project. Leave empty to use system temp.";
    
    ps->add_property_info(property_info);
    ps->set_as_basic(setting_path, true); // Visible without "Advanced" toggle

    // --- 2. Create UI Button ---
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
    args.append("--verbose");

    UtilityFunctions::print("NanoCoverage: Launching child process at: ", temp_path);
    
    OS::get_singleton()->create_process(godot_exe, args);
}

} // namespace godot