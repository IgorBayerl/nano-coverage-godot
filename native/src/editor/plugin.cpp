#include "plugin.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../runtime/coverage_monitor.h"  // Needed to cast singleton
#include "temp_builder.h"
#include "../config/settings_keys.h"
#include "../config/settings_gateway.h"

namespace godot {

void NanoCoverageEditorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_run_instrumented_pressed"),
                         &NanoCoverageEditorPlugin::_on_run_instrumented_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_report_pressed"),
                         &NanoCoverageEditorPlugin::_on_generate_report_pressed);
}

NanoCoverageEditorPlugin::NanoCoverageEditorPlugin() {
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

void NanoCoverageEditorPlugin::_on_generate_report_pressed() {
    if (Engine::get_singleton()->has_singleton("NanoCoverage")) {
        Object* obj = Engine::get_singleton()->get_singleton("NanoCoverage");
        NanoCoverage* cov = Object::cast_to<NanoCoverage>(obj);
        if (cov) {
            cov->generate_report();
        } else {
            UtilityFunctions::printerr("NanoCoverage: Singleton found but cast failed.");
        }
    } else {
        UtilityFunctions::printerr("NanoCoverage: Singleton not found.");
    }
}

}  // namespace godot