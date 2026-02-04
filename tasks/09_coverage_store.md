# Task: Implement Persistent Run Store

## Context Files
- `native/src/runtime/coverage_store.h` (New)
- `native/src/runtime/coverage_store.cpp` (New)

## Goal
A system to persist coverage data to disk so it survives across multiple Godot process runs.
We will store in binary format to avoid the need for serializing/deserializing json, we dont want overhead and we dont care about the data being readable.

## Instructions
1.  **Create `CoverageStore` class**:
    * Storage Path: `data_store_dir/<workspace_id>/runs/<run_id>.covdata`.
2.  **Implement `append_run_snapshot(run_id, snapshot)`**:
    * Serialize the snapshot to binary.
    * Save it to a new file in the storage path.
3.  **Implement `load_and_merge()`**:
    * Iterate all binary files in the storage path.
    * Sum up the hit counts for matching file/lines.
    * Return a single merged snapshot.
4.  **Unit Test**:
    * Simulate two different runs.
    * Save them.
    * Load and merge.
    * Assert totals are the sum of both runs.

## Validation
* Run `python scripts/test.py`.