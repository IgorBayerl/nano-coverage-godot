# Task: Implement Encoding-Aware SourceReader

## Context Files
- `native/src/instrumentation/source_reader.h` (New)
- `native/src/instrumentation/source_reader.cpp` (New)

## Goal
Read files robustly, handling BOMs (Byte Order Marks) to ensure we always process valid UTF-8.

## Instructions
1.  **Create struct `ReadTextResult`**: Contains `bool ok`, `std::string utf8_text`, `std::string error`.
2.  **Implement `read_text_file(path)`**:
    * Read file bytes.
    * **Check BOM**:
        * If UTF-8 BOM (`0xEF,0xBB,0xBF`): Strip it.
        * If UTF-16 LE/BE: Convert to UTF-8 (simple internal conversion).
        * If No BOM: Validate as UTF-8.
    * If validation fails, return an error (do not guess encoding yet).
3.  **Unit Tests**:
    * Test reading a file with a UTF-8 BOM.
    * Test reading a file with UTF-16 LE BOM.
    * Test reading invalid UTF-8 (should fail).

## Validation
* Run `python scripts/test.py`.