# Task: Create Instrumentation Golden Test

## Context Files
- `native/src/instrumentation/instrumenter.h`
- `native/src/instrumentation/instrumenter.cpp`
- `native/src/tests/` (Check existing test structure here)

## Goal
Add a "golden" unit test that validates GDScript instrumentation output against a static expected string. This ensures future refactors don't silently break code injection.

## Instructions
1.  **Create a new test file** (e.g., `native/src/tests/instrumentation_golden_test.cpp`).
2.  **Define the Test Case:**
    * Input: A raw string simulating a `.gd` file. It must contain:
        * A function definition.
        * An `if` statement.
        * A comment and a blank line (to ensure we don't instrument these).
    * Expected Output: A hardcoded string representing exactly how the code *should* look after instrumentation (including `NanoCoverage.hit(...)` calls).
3.  **Implementation:**
    * Instantiate the current `Instrumenter` class.
    * Run the instrumentation method on the input string.
    * Normalize line endings (convert `\r\n` to `\n`) before comparing.
    * Assert that `actual_output == expected_output`.

## Validation
* Run `python scripts/test.py`.
* The test must pass.
* **Manual Check:** Modify the expected output string slightly and verify the test fails (ensures the test is actually asserting).