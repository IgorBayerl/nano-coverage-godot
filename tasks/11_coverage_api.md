# Task: Implement CoverageApi Facade

## Context Files
- `native/src/api/coverage_api.h` (New)
- `native/src/api/coverage_api.cpp` (New)
- `native/src/godot/register_types.cpp`

## Goal
Create the public-facing API that EditorPlugin and external tools (CI) will call.

## Instructions
1.  **Define Class `CoverageApi`** (inherits `Object` or `RefCounted`):
    * Register it in `register_types.cpp` so it is accessible to GDScript.
2.  **Implement Methods**:
    * `instrument_project(options)`: Calls `InstrumentedProjectBuilder`. Returns workspace ID.
    * `run_instrumented_project(options)`: Launches Godot via `OS::create_process` or `execute`. Support `blocking: bool`.
    * `generate_coverage_report(options)`: Calls `Store::load_and_merge` -> `LcovWriter::write`. **Do not clear data.**
    * `clear_coverage_data(options)`: Clears the Store and the Collector.
3.  **Contract Test**:
    * Write a test calling these methods to ensure they accept the Dictionaries and return expected keys.

## Validation
* Run `python scripts/test.py`.