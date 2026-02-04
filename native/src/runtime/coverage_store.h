#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "../data/persistence.h"

namespace godot {

class CoverageStore {
public:
    CoverageStore(const std::string& data_store_dir, const std::string& workspace_id);

    // Persists the given snapshot to a file named <run_id>.covdata in the store directory.
    // The format is binary as defined by Persistence::append_execution_data.
    void append_run_snapshot(const std::string& run_id, const CoverageData& snapshot);

    // Iterates all .covdata files in the store directory and merges them into a single CoverageData map.
    CoverageData load_and_merge();

private:
    std::string root_path;
    std::string runs_path;
    
    void ensure_paths();
};

} // namespace godot
