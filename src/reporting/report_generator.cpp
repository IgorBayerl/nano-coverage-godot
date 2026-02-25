#include "report_generator.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../config/settings_gateway.h"
#include "../data/persistence.h"
#include "../data/coverage_store.h"
#include "../reporting/lcov_writer.h"
#include "../utils/logger.h"

namespace godot {

Dictionary ReportGenerator::generate(const Dictionary& options) {
    Dictionary result;

    String workspace_id = options.get("workspace_id", "default");

    CoverageSettings settings = SettingsGateway::load();
    String data_store_dir = settings.data_store_dir;

    String data_store_base = ProjectSettings::get_singleton()->globalize_path(data_store_dir);

    // Convert to std::string for CoverageStore
    std::string base_str = data_store_base.utf8().get_data();
    std::string ws_id_str = workspace_id.utf8().get_data();

    // Load Execution Hits
    CoverageStore store(base_str, ws_id_str);
    CoverageData raw_hits = store.load_and_merge();

    // Load Static Metadata (Coverable Lines)
    CoverageMetadata meta;

    // Construct path using Godot String to handle separators gracefully on all OS
    String meta_path_godot = data_store_base.path_join("coverage.meta");

    // Use Persistence directly to check/load
    if (!Persistence::load_metadata(meta_path_godot.utf8().get_data(), meta)) {
        Logger::error("Failed to load coverage.meta from " + meta_path_godot);
        result["error"] = "Metadata file missing or unreadable. Did you run instrumentation?";
        return result;
    }

    // Merge Hits into Metadata
    CoverageData final_data;

    for (const auto& meta_kv : meta) {
        const std::string& file_path = meta_kv.first;
        const std::vector<uint32_t>& coverable_lines = meta_kv.second;

        std::unordered_map<uint32_t, uint64_t>& file_lines = final_data[file_path];

        auto hit_file_it = raw_hits.find(file_path);
        bool has_hits = (hit_file_it != raw_hits.end());

        for (uint32_t line : coverable_lines) {
            uint64_t count = 0;

            if (has_hits) {
                auto hit_line_it = hit_file_it->second.find(line);
                if (hit_line_it != hit_file_it->second.end()) {
                    count = hit_line_it->second;
                }
            }
            file_lines[line] = count;
        }
    }

    // Write Report
    LCOVWriter::write_lcov_report(final_data, settings);

    result["status"] = "ok";
    result["report_path"] = settings.report_dir;

    return result;
}

}  // namespace godot
