#pragma once

#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class ReportGenerator {
   public:
    /// Generates the LCOV report from collected run data and static metadata.
    /// Expects options: { "workspace_id": ... }
    /// Returns: { "status": "ok", "report_path": ... } or { "error": ... }
    static Dictionary generate(const Dictionary& options);
};

}  // namespace godot
