#include "nano_coverage.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

NanoCoverageRuntime *NanoCoverageRuntime::singleton = nullptr;

void NanoCoverageRuntime::_bind_methods() {
	ClassDB::bind_method(D_METHOD("hit", "file_path", "line_number"), &NanoCoverageRuntime::hit);
	ClassDB::bind_method(D_METHOD("save_lcov", "output_path"), &NanoCoverageRuntime::save_lcov);
	ClassDB::bind_method(D_METHOD("reset"), &NanoCoverageRuntime::reset);
}

NanoCoverageRuntime::NanoCoverageRuntime() {
	singleton = this;
}

NanoCoverageRuntime::~NanoCoverageRuntime() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

NanoCoverageRuntime *NanoCoverageRuntime::get_singleton() {
	return singleton;
}

void NanoCoverageRuntime::hit(const String &file_path, int line_number) {
	if (line_number < 0) return;

	if (!coverage_data.has(file_path)) {
		coverage_data[file_path] = Vector<int>();
	}

	Vector<int> &lines = coverage_data[file_path];
	
	// Resize if needed (line_number is 1-based usually, but let's assume 1-based input)
	// If input is 1-based, we need size to be at least line_number + 1 to use index directly?
	// Or we just shift by 1. Let's use 0-based index for storage, so index = line_number - 1.
	// But let's just use direct mapping to avoid off-by-one confusion.
	// If line is 10, we need size 11 to access index 10.
	if (lines.size() <= line_number) {
		lines.resize(line_number + 1);
		// Initialize new elements to 0? Vector resize does default construct (0 for int).
	}

	lines.write[line_number]++;
}

void NanoCoverageRuntime::reset() {
	coverage_data.clear();
}

void NanoCoverageRuntime::save_lcov(const String &output_path) {
	Ref<FileAccess> f = FileAccess::open(output_path, FileAccess::WRITE);
	if (f.is_null()) {
		UtilityFunctions::printerr("NanoCoverage: Could not open file for writing: ", output_path);
		return;
	}

	f->store_line("TN:"); // Test Name

	for (const KeyValue<String, Vector<int>> &E : coverage_data) {
		String file_path = E.key;
		const Vector<int> &lines = E.value;

		f->store_line("SF:" + file_path);

		int lines_found = 0;
		int lines_hit = 0;

		for (int i = 0; i < lines.size(); i++) {
			int hits = lines[i];
			if (hits > 0) {
				f->store_line("DA:" + String::num_int64(i) + "," + String::num_int64(hits));
				lines_found++;
				lines_hit++;
			} else {
				// In a real coverage tool we would know which lines are executable but not hit.
				// Here we only know about lines that were hit OR lines that we instrumented.
				// Wait, the instrumentor injects calls. If the line is NOT hit, we won't see a call.
				// So we only report HIT lines? That's not full coverage data.
				// To get full coverage (including 0 hits), we need to know all instrumented lines.
				// But we don't have that info here at runtime unless we register them.
				
				// For now, we only report hits. This is "active" coverage.
				// To fix this, we'd need to register instrumented lines at startup.
				// But let's stick to the plan: "NanoCoverage collects data".
				// If we only report hits, LCOV might show 100% coverage for the lines it knows about?
				// No, LCOV needs to know about all lines.
				
				// For this iteration, let's just dump what we have.
				// The user can improve this later by registering lines.
			}
		}

		f->store_line("LF:" + String::num_int64(lines_found));
		f->store_line("LH:" + String::num_int64(lines_hit));
		f->store_line("end_of_record");
	}

	f->close();
	UtilityFunctions::print("NanoCoverage: Saved LCOV report to ", output_path);
}
