#include "coverage_store.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace godot {

namespace fs = std::filesystem;

CoverageStore::CoverageStore(const std::string& data_store_dir, const std::string& workspace_id) {
    fs::path base = fs::path(data_store_dir) / workspace_id;
    root_path = base.string();
    runs_path = (base / "runs").string();
}

void CoverageStore::ensure_paths() {
    if (!fs::exists(runs_path)) {
        fs::create_directories(runs_path);
    }
}

void CoverageStore::append_run_snapshot(const std::string& run_id, const CoverageData& snapshot) {
    ensure_paths();
    fs::path file_path = fs::path(runs_path) / (run_id + ".covdata");
    Persistence::append_execution_data(file_path.string(), snapshot);
}

CoverageData CoverageStore::load_and_merge() {
    CoverageData merged_data;
    if (!fs::exists(runs_path)) {
        return merged_data;
    }

    for (const auto& entry : fs::directory_iterator(runs_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".covdata") {
            Persistence::load_and_merge_execution_data(entry.path().string(), merged_data);
        }
    }
    return merged_data;
}

void CoverageStore::clear() {
    if (!fs::exists(runs_path)) {
        return;
    }

    // We only remove .covdata files to be safe
    // We only remove .covdata files to be safe
    for (const auto& entry : fs::directory_iterator(runs_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".covdata") {
            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (ec) {
                // Best effort
                std::cerr << "CoverageStore: Failed to delete " << entry.path().string() << ": " << ec.message() << std::endl;
            }
        }
    }
}

} // namespace godot
