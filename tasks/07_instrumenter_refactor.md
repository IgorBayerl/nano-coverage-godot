# Task: Pure Instrumentation Pipeline

## Context Files
- `native/src/instrumentation/instrumenter.h`
- `native/src/instrumentation/instrumenter.cpp`
- `native/src/instrumentation/source_reader.h`

## Goal
Split the Instrumenter into a "Pure Logic" phase (String -> String) and an "I/O Wrapper" (File -> File).

## Instructions
1.  **Refactor/Create `GdScriptInstrumenter`**:
    * Method `instrument_text(utf8_code, res_path)`: Returns struct `{ code, insertions }`. Does **no** file I/O.
    * Method `instrument_file(path, res_path)`:
        * Uses `SourceReader` to get content.
        * Calls `instrument_text`.
        * Writes result back to disk.
2.  **Update Existing Code**:
    * Ensure `TempProjectBuilder` calls `instrument_file`.
    * Update the "Golden Test" (Task 1) to use `instrument_text` directly (removing filesystem dependency for that test).

## Validation
* Run `python scripts/test.py`.