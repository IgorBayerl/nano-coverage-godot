#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "editor/plugin.h"
#ifdef TESTS_ENABLED
#include "tests/test_main.h"
#endif

using namespace godot;

void initialize_nano_coverage_godot_module(ModuleInitializationLevel p_level) {
    // 1. SCENE LEVEL: Register Runtime classes (TestRunner, Singletons, etc.)
    // These are available in both the Editor and the running Game.
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        #ifdef TESTS_ENABLED
        ClassDB::register_class<NanoCoverageTestRunner>();
#endif
        // Future: ClassDB::register_class<NanoCoverageRuntime>();
    }

    // 2. EDITOR LEVEL: Register Editor-only classes (Plugins, UI, etc.)
    // These are only available when the Editor is open.
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        ClassDB::register_class<NanoCoverageEditorPlugin>();
        UtilityFunctions::print("NanoCoverageGodot: Editor classes registered.");
    }
}

void uninitialize_nano_coverage_godot_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        // Cleanup runtime singletons here if needed
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
    
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}