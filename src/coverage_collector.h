#ifndef COVERAGE_COLLECTOR_H
#define COVERAGE_COLLECTOR_H

#include <godot_cpp/classes/node.hpp>

namespace godot {

class CoverageCollector : public Node {
	GDCLASS(CoverageCollector, Node)

protected:
	static void _bind_methods();

public:
	CoverageCollector();
	~CoverageCollector();

	void start_coverage();
	void stop_coverage();
};

}

#endif
