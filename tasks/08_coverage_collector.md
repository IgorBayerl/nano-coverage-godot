# Task: Extract Runtime Collector

## Context Files
- `native/src/runtime/coverage_monitor.cpp` (Current logic)
- `native/src/runtime/coverage_collector.h` (New)
- `native/src/runtime/coverage_collector.cpp` (New)

## Goal
Move the hash maps and mutexes out of the global singleton and into a dedicated class that can generate snapshots.

## Instructions
1.  **Create `CoverageCollector` class**:
    * Move the `hit()` logic here.
    * Keep the `std::mutex`.
2.  **Add `snapshot()` method**:
    * Returns a Dictionary (or struct) of `{ file -> { line -> count } }`.
    * Must be thread-safe (lock, copy, unlock).
3.  **Refactor NanoCoverage**:
    * The singleton should now own an instance of `CoverageCollector` and delegate `hit()` calls to it.
4.  **Test**:
    * Verify `snapshot()` returns accurate data.
    * Verify `clear()` resets the internal maps.

## Validation
* Run `python scripts/test.py`.