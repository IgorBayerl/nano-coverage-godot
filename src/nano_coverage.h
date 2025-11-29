#ifndef NANO_COVERAGE_H
#define NANO_COVERAGE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class NanoCoverageRuntime : public Node {
	GDCLASS(NanoCoverageRuntime, Node);

private:
	static NanoCoverageRuntime *singleton;
	
	// Map: FilePath -> (LineNumber -> HitCount)
	// Using Vector for lines where index = line_number. 
	// We'll resize dynamically.
	HashMap<String, Vector<int>> coverage_data;

protected:
	static void _bind_methods();

public:
	NanoCoverageRuntime();
	~NanoCoverageRuntime();

	static NanoCoverageRuntime *get_singleton();

	void hit(const String &file_path, int line_number);
	void save_lcov(const String &output_path);
	
	// Helper to clear data
	void reset();
};

}

#endif
