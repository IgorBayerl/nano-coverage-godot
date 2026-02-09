#include "coverage_collector.h"

#include <gtest/gtest.h>

namespace godot {

TEST(CoverageCollectorTest, RecordHitIncrementsCount) {
    CoverageCollector collector;
    collector.record_hit("res://test.gd", 1);
    collector.record_hit("res://test.gd", 1);
    collector.record_hit("res://test.gd", 2);

    EXPECT_EQ(collector.get_total_hits(), 3);
}

TEST(CoverageCollectorTest, SnapshotReturnsCopy) {
    CoverageCollector collector;
    collector.record_hit("res://test.gd", 1);

    CoverageData snap = collector.snapshot();
    EXPECT_EQ(snap["res://test.gd"][1], 1);

    // Modify collector, snapshot should remain unchanged
    collector.record_hit("res://test.gd", 1);
    EXPECT_EQ(snap["res://test.gd"][1], 1);
    EXPECT_EQ(collector.get_total_hits(), 2);
}

TEST(CoverageCollectorTest, ClearResetsData) {
    CoverageCollector collector;
    collector.record_hit("res://test.gd", 1);
    EXPECT_EQ(collector.get_total_hits(), 1);

    collector.clear();
    EXPECT_EQ(collector.get_total_hits(), 0);

    CoverageData snap = collector.snapshot();
    EXPECT_TRUE(snap.empty());
}

TEST(CoverageCollectorTest, IgnoreInvalidInput) {
    CoverageCollector collector;
    collector.record_hit("", 1);
    collector.record_hit("res://test.gd", 0);
    collector.record_hit("res://test.gd", -1);

    EXPECT_EQ(collector.get_total_hits(), 0);
}

}  // namespace godot
