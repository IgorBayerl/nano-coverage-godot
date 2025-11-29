#include "register_types.h"

#include "nano_coverage.h"
#include "instrumentor.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static NanoCoverageRuntime *nano_coverage_singleton = nullptr;

void initialize_nano_coverage_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	UtilityFunctions::print("NanoCoverage: Initializing module...");
	ClassDB::register_class<NanoCoverageRuntime>();
	ClassDB::register_class<Instrumentor>();

	// We don't instantiate NanoCoverage here because it will be instantiated 
	// by the Autoload system (via the dummy script that extends it).
}

void uninitialize_nano_coverage_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// No need to delete if it's managed by the SceneTree (Autoload)
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT nano_coverage_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_nano_coverage_module);
	init_obj.register_terminator(uninitialize_nano_coverage_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
