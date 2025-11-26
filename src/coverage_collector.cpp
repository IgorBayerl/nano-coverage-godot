#include "coverage_collector.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>

using namespace godot;

void CoverageCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_coverage"), &CoverageCollector::start_coverage);
	ClassDB::bind_method(D_METHOD("stop_coverage"), &CoverageCollector::stop_coverage);
	ClassDB::bind_method(D_METHOD("save_lcov"), &CoverageCollector::save_lcov);
}

CoverageCollector::CoverageCollector() {
}

CoverageCollector::~CoverageCollector() {
}

void CoverageCollector::start_coverage() {
	EngineDebugger::get_singleton()->profiler_enable("coverage", true);
}

void CoverageCollector::stop_coverage() {
	EngineDebugger::get_singleton()->profiler_enable("coverage", false);
}

void CoverageCollector::_toggle(bool p_enable, const Array &p_options) {
	if (p_enable) {
		UtilityFunctions::print("Coverage collection started!");
		coverage_data.clear();
	} else {
		UtilityFunctions::print("Coverage collection stopped!");
		save_lcov();
	}
}

void CoverageCollector::_add_frame(const Array &p_data) {
	UtilityFunctions::print("Frame added: ", p_data);
	// p_data format: [script_path, line_number, function_name, type]
	if (p_data.size() < 2) {
		return;
	}

	String script_path = p_data[0];
	int line_number = p_data[1];

	if (!coverage_data.has(script_path)) {
		coverage_data[script_path] = HashMap<int, int>();
	}
	
	coverage_data[script_path][line_number]++;
}

void CoverageCollector::_tick(double p_frame_time, double p_process_time, double p_physics_time, double p_physics_frame_time) {
	UtilityFunctions::print("Tick");
}

void CoverageCollector::save_lcov() {
	String output_path = "res://coverage/lcov.info";
	Ref<FileAccess> file = FileAccess::open(output_path, FileAccess::WRITE);
	if (file.is_null()) {
		UtilityFunctions::printerr("Failed to open file for writing: " + output_path);
		return;
	}

	for (const KeyValue<String, HashMap<int, int>> &E : coverage_data) {
		String script_path = E.key;
		const HashMap<int, int> &lines = E.value;

		file->store_line("TN:");
		file->store_line("SF:" + script_path);

		for (const KeyValue<int, int> &L : lines) {
			file->store_line("DA:" + String::num_int64(L.key) + "," + String::num_int64(L.value));
		}

		file->store_line("end_of_record");
	}
	
	file->close();
	UtilityFunctions::print("LCOV report saved to: " + output_path);
}
