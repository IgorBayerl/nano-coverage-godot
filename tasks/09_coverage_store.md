# Task: Implement Persistent Run Store

## Context Files
- `native/src/runtime/coverage_store.h` (New)
- `native/src/runtime/coverage_store.cpp` (New)

## Goal
A system to persist coverage data to disk so it survives across multiple Godot process runs.

## Instructions
1.  **Create `CoverageStore` class**:
    * Storage Path: `data_store_dir/<workspace_id>/runs/<run_id>.json`.
2.  **Implement `append_run_snapshot(run_id, snapshot)`**:
    * Serialize the snapshot to JSON.
    * Save it to a new file in the storage path.
3.  **Implement `load_and_merge()`**:
    * Iterate all JSON files in the storage path.
    * Sum up the hit counts for matching file/lines.
    * Return a single merged snapshot.
4.  **Unit Test**:
    * Simulate two different runs.
    * Save them.
    * Load and merge.
    * Assert totals are the sum of both runs.

## Validation
* Run `python scripts/test.py`.