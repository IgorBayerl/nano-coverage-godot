#include "coverage_collector.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void CoverageCollector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_coverage"), &CoverageCollector::start_coverage);
	ClassDB::bind_method(D_METHOD("stop_coverage"), &CoverageCollector::stop_coverage);
}

CoverageCollector::CoverageCollector() {
	// Initialize any variables here.
}

CoverageCollector::~CoverageCollector() {
	// Add your cleanup here.
}

void CoverageCollector::start_coverage() {
	UtilityFunctions::print("Coverage collection started!");
}

void CoverageCollector::stop_coverage() {
	UtilityFunctions::print("Coverage collection stopped!");
}
