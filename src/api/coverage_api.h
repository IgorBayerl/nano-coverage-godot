#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class CoverageApi : public RefCounted {
    GDCLASS(CoverageApi, RefCounted)

   private:
    static void _bind_methods();

   public:
    /// Instruments the project found at 'res://' (or configured path).
    /// Returns: { "workspace_id": ..., "output_path": ... }
    Dictionary instrument_project(const Dictionary& options);

    /// Runs the instrumented project.
    /// Expects options: { "output_path": ..., "workspace_id": ..., "blocking": bool }
    /// Returns: { "pid": ..., "output_file": ... }
    Dictionary run_instrumented_project(const Dictionary& options);

    /// Generates the LCOV report from collected run data.
    /// Expects options: { "workspace_id": ... }
    /// Returns: { "report_path": ... }
    Dictionary generate_coverage_report(const Dictionary& options);

    /// Clears data from previous runs.
    /// Expects options: { "workspace_id": ... }
    /// Returns: { "status": "ok" }
    Dictionary clear_coverage_data(const Dictionary& options);
};

}  // namespace godot
