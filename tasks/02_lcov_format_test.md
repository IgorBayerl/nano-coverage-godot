# Task: Strengthen LCOV Output Tests

## Context Files
- `native/src/runtime/lcov_writer.h`
- `native/src/runtime/lcov_writer.cpp`
- Existing tests in `native/src/tests/`

## Goal
Ensure the LCOV writer produces valid tracefiles (`SF`, `DA`, `end_of_record`) even when handling multiple files and lines.

## Instructions
1.  **Locate or Create** the LCOV writer test file.
2.  **Add/Extend a Test Case:**
    * Simulate coverage data for **two distinct files**.
    * Simulate hits on **multiple lines** for each file.
    * Call the write function.
3.  **Assertions:**
    * Verify `SF:` (Source File) appears twice.
    * Verify `DA:<line>,<hits>` lines match your input data exactly.
    * Verify `end_of_record` appears at the end of each file block.
4.  **Cleanup:** If the test writes to `res://`, ensure it cleans up the generated file after the test runs.

## Validation
* Run `python scripts/test.py`.
* The test must pass.