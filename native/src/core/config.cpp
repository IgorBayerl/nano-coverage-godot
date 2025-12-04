#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "nano_coverage/editor_plugin.hpp"

using namespace godot;

void initialize_nano_coverage_godot_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        ClassDB::register_class<NanoCoverageEditorPlugin>();
        UtilityFunctions::print("NanoCoverageGodot: Registered NanoCoverageEditorPlugin class.");
    }
}

void uninitialize_nano_coverage_godot_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }
}

extern "C" {
// Initialization.
#ifdef _WIN32
__declspec(dllexport) GDExtensionBool nano_coverage_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
#else
GDExtensionBool GDE_EXPORT nano_coverage_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
#endif
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_nano_coverage_godot_module);
    init_obj.register_terminator(uninitialize_nano_coverage_godot_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_EDITOR);

    return init_obj.init();
}
}
