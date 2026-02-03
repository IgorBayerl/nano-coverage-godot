# Task: Document Integration Contract

## Context Files
- `docs/integration_guide.md` (New)

## Goal
Provide clear instructions for external tools (like gdUnit4) on how to interface with this extension.

## Instructions
1.  **Create `docs/integration_guide.md`**.
2.  **Content Requirements**:
    * **API Class**: Identify `CoverageApi` as the entry point.
    * **CI Workflow**: Document the sequence:
        1.  `instrument_project`
        2.  `run_instrumented_project` (Loop)
        3.  `generate_coverage_report`
    * **Options Reference**: List the Dictionary keys for each method (e.g., `blocking`, `workspace_id`).
    * **Hook Example**: Provide a small GDScript snippet showing how a test runner should call `NanoCoverage.snapshot()` and flush to store at the end of a session.

## Validation
* Review the markdown for clarity.
* Ensure the documented API matches the code in `CoverageApi.h`.