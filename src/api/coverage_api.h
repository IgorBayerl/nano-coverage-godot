#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "../data/persistence.h"

namespace godot {

class CoverageApi : public RefCounted {
    GDCLASS(CoverageApi, RefCounted)

   private:
    CoverageMetadata session_metadata;

    static void _bind_methods();

   public:
    /// Instruments a single script's source code and tracks coverable lines.
    /// Returns: { "code": modified_string, "lines": [coverable_lines] }
    Dictionary instrument_script(const String& source_code, const String& file_path);

    /// Saves the accumulated metadata to coverage.meta
    void save_static_metadata();



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
