#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "api/coverage_api.h"
#include "editor/plugin.h"
#include "runtime/coverage_monitor.h"
#ifdef TESTS_ENABLED
#include "testing/test_main.h"
#endif

using namespace godot;

static NanoCoverage* g_nano_coverage_singleton = nullptr;

void initialize_nano_coverage_godot_module(ModuleInitializationLevel p_level) {
    // 1. SCENE LEVEL: Register Runtime classes (TestRunner, Singletons, etc.)
    // These are available in both the Editor and the running Game.
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifdef TESTS_ENABLED
        ClassDB::register_class<NanoCoverageTestRunner>();
#endif

        ClassDB::register_class<NanoCoverage>();
        ClassDB::register_class<CoverageApi>();

        if (g_nano_coverage_singleton == nullptr) {
            g_nano_coverage_singleton = memnew(NanoCoverage);
            Engine::get_singleton()->register_singleton("NanoCoverage", g_nano_coverage_singleton);
        }

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
        if (g_nano_coverage_singleton != nullptr) {
            Engine::get_singleton()->unregister_singleton("NanoCoverage");
            memdelete(g_nano_coverage_singleton);
            g_nano_coverage_singleton = nullptr;
        }
    }
}

extern "C" {
// Initialization.
#ifdef _WIN32
__declspec(dllexport) GDExtensionBool nano_coverage_godot_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization* r_initialization) {
#else
GDExtensionBool GDE_EXPORT nano_coverage_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                            const GDExtensionClassLibraryPtr p_library,
                                                            GDExtensionInitialization* r_initialization) {
#endif
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_nano_coverage_godot_module);
    init_obj.register_terminator(uninitialize_nano_coverage_godot_module);

    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}