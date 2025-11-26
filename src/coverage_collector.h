#ifndef COVERAGE_COLLECTOR_H
#define COVERAGE_COLLECTOR_H

#include <godot_cpp/classes/engine_profiler.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {

class CoverageCollector : public EngineProfiler {
	GDCLASS(CoverageCollector, EngineProfiler)

private:
	HashMap<String, HashMap<int, int>> coverage_data;

protected:
	static void _bind_methods();

public:
	CoverageCollector();
	~CoverageCollector();

	void start_coverage();
	void stop_coverage();

	void _toggle(bool p_enable, const Array &p_options) override;
	void _add_frame(const Array &p_data) override;
	void _tick(double p_frame_time, double p_process_time, double p_physics_time, double p_physics_frame_time) override;

	void save_lcov();
};

}

#endif
