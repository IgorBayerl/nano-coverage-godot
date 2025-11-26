#include "register_types.h"

#include "coverage_collector.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/engine_debugger.hpp>

using namespace godot;

static Ref<CoverageCollector> coverage_collector;

void initialize_nano_coverage_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<CoverageCollector>();

	coverage_collector.instantiate();
	EngineDebugger::get_singleton()->register_profiler("coverage", coverage_collector);
}

void uninitialize_nano_coverage_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (EngineDebugger::get_singleton()) {
		EngineDebugger::get_singleton()->unregister_profiler("coverage");
	}
	coverage_collector.unref();
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
