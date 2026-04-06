#include "disk_instrumenter.h"

#include <gtest/gtest.h>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include "../config/settings_keys.h"
#include "../testing/test_utils.h"

using namespace godot;

static const char* TEST_DIR_STR = "res://_test_disk_instr";
static const char* TEST_BACKUP_DIR_STR = "res://_test_disk_instr_backup";

class DiskInstrumenterTest : public ::testing::Test {
   protected:
    String TEST_DIR;
    String TEST_BACKUP_DIR;

    void SetUp() override {
        TEST_DIR = TEST_DIR_STR;
        TEST_BACKUP_DIR = TEST_BACKUP_DIR_STR;
        TestUtils::clean_dir(TEST_DIR);
        TestUtils::clean_dir(TEST_BACKUP_DIR);
        DirAccess::make_dir_recursive_absolute(TEST_DIR);
    }

    void TearDown() override {
        TestUtils::clean_dir(TEST_DIR);
        TestUtils::clean_dir(TEST_BACKUP_DIR);
    }

    static void write_gd_file(const String& path, const String& content) {
        String dir = path.get_base_dir();
        if (!DirAccess::dir_exists_absolute(dir)) {
            DirAccess::make_dir_recursive_absolute(dir);
        }
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
        f->store_string(content);
        f->close();
    }

    static String read_file(const String& path) {
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
        if (f.is_null()) return "";
        String content = f->get_as_text();
        f->close();
        return content;
    }
};

TEST_F(DiskInstrumenterTest, InstrumentAndRestoreRoundTrip) {
    String file1 = TEST_DIR.path_join("player.gd");
    String file2 = TEST_DIR.path_join("enemy.gd");
    String src1 = "extends Node\n\nfunc _ready():\n\tvar x = 1\n";
    String src2 = "extends Node\n\nfunc _process(delta):\n\tpass\n";

    write_gd_file(file1, src1);
    write_gd_file(file2, src2);

    // Configure settings to only instrument our test dir
    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    ASSERT_FALSE(di->is_instrumented());

    // Instrument
    Dictionary result = di->instrument_to_disk();
    ASSERT_EQ(String(result["status"]), "ok");
    EXPECT_GT(int(result["instrumented_count"]), 0);

    EXPECT_TRUE(di->is_instrumented());

    // Verify files contain marker
    String patched1 = read_file(file1);
    EXPECT_TRUE(patched1.begins_with("# __NANO_COVERAGE_INSTRUMENTED__"));
    EXPECT_NE(patched1.find("NanoCoverage.hit("), -1);

    // Verify backups exist
    String bkp1 = TEST_BACKUP_DIR.path_join("backup").path_join("_test_disk_instr/player.gd");
    EXPECT_TRUE(FileAccess::file_exists(bkp1));

    // Verify manifest exists
    String manifest_path = TEST_BACKUP_DIR.path_join("manifest.json");
    EXPECT_TRUE(FileAccess::file_exists(manifest_path));

    // Restore
    Dictionary restore_result = di->restore_from_disk();
    ASSERT_EQ(String(restore_result["status"]), "ok");
    EXPECT_GT(int(restore_result["restored_count"]), 0);

    EXPECT_FALSE(di->is_instrumented());

    // Verify files match original content
    String restored1 = read_file(file1);
    EXPECT_EQ(restored1, src1);

    String restored2 = read_file(file2);
    EXPECT_EQ(restored2, src2);

    // Verify backup directory is cleaned up
    EXPECT_FALSE(FileAccess::file_exists(manifest_path));
}

TEST_F(DiskInstrumenterTest, RefusesDoubleInstrument) {
    String file1 = TEST_DIR.path_join("script.gd");
    write_gd_file(file1, "extends Node\n\nfunc _ready():\n\tpass\n");

    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    // First instrument should succeed
    Dictionary result1 = di->instrument_to_disk();
    ASSERT_EQ(String(result1["status"]), "ok");

    // Second instrument should fail
    Dictionary result2 = di->instrument_to_disk();
    EXPECT_EQ(String(result2["status"]), "error");
    EXPECT_TRUE(String(result2["error"]).find("already instrumented") != -1);

    // Clean up
    di->restore_from_disk();
}

TEST_F(DiskInstrumenterTest, RefusesInstrumentIfMarkerPresent) {
    String file1 = TEST_DIR.path_join("marked.gd");
    write_gd_file(file1, "# __NANO_COVERAGE_INSTRUMENTED__\nextends Node\n\nfunc _ready():\n\tpass\n");

    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    Dictionary result = di->instrument_to_disk();
    EXPECT_EQ(String(result["status"]), "error");
    EXPECT_TRUE(String(result["error"]).find("marker") != -1);
}

TEST_F(DiskInstrumenterTest, RestoreWithoutManifestIsNoOp) {
    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    Dictionary result = di->restore_from_disk();
    EXPECT_EQ(String(result["status"]), "error");
    EXPECT_TRUE(String(result["error"]).find("No manifest") != -1);
}

TEST_F(DiskInstrumenterTest, HashVerificationOnRestore) {
    String file1 = TEST_DIR.path_join("hash_test.gd");
    write_gd_file(file1, "extends Node\n\nfunc _ready():\n\tvar x = 1\n");

    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    Dictionary result = di->instrument_to_disk();
    ASSERT_EQ(String(result["status"]), "ok");

    // Corrupt the backup file
    String bkp_path = TEST_BACKUP_DIR.path_join("backup").path_join("_test_disk_instr/hash_test.gd");
    if (FileAccess::file_exists(bkp_path)) {
        Ref<FileAccess> f = FileAccess::open(bkp_path, FileAccess::WRITE);
        f->store_string("corrupted content");
        f->close();
    }

    // Restore should still work but with warnings
    Dictionary restore_result = di->restore_from_disk();
    EXPECT_EQ(String(restore_result["status"]), "ok");

    Array warnings = restore_result["warnings"];
    EXPECT_GT(warnings.size(), 0);
}

TEST_F(DiskInstrumenterTest, ManifestJsonStructure) {
    String file1 = TEST_DIR.path_join("struct_test.gd");
    write_gd_file(file1, "extends Node\n\nfunc _ready():\n\tvar x = 1\n");

    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, PackedStringArray());
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    Dictionary result = di->instrument_to_disk();
    ASSERT_EQ(String(result["status"]), "ok");

    // Read and verify manifest
    String manifest_path = TEST_BACKUP_DIR.path_join("manifest.json");
    ASSERT_TRUE(FileAccess::file_exists(manifest_path));

    String json_str = read_file(manifest_path);
    Variant parsed = JSON::parse_string(json_str);
    ASSERT_EQ(parsed.get_type(), Variant::DICTIONARY);

    Dictionary manifest = parsed;
    EXPECT_EQ(int(manifest["version"]), 1);
    EXPECT_TRUE(manifest.has("files"));

    Array files = manifest["files"];
    EXPECT_GT(files.size(), 0);

    // Verify first file entry structure
    Dictionary entry = files[0];
    EXPECT_TRUE(entry.has("path"));
    EXPECT_TRUE(entry.has("backup_path"));
    EXPECT_TRUE(entry.has("original_hash"));

    String hash = entry["original_hash"];
    EXPECT_TRUE(hash.begins_with("sha256:"));

    // Clean up
    di->restore_from_disk();
}

TEST_F(DiskInstrumenterTest, IgnorePatternsRespected) {
    String src_file = TEST_DIR.path_join("src/main.gd");
    String test_file = TEST_DIR.path_join("test/main_test.gd");

    write_gd_file(src_file, "extends Node\n\nfunc _ready():\n\tvar x = 1\n");
    write_gd_file(test_file, "extends Node\n\nfunc test_something():\n\tvar y = 2\n");

    PackedStringArray ignores;
    ignores.push_back("**/*_test.gd");

    SettingsOverride s_backup(SettingsKeys::BACKUP_DIR, TEST_BACKUP_DIR);
    SettingsOverride s_ignore(SettingsKeys::IGNORE_PATHS, ignores);
    SettingsOverride s_addons(SettingsKeys::IGNORE_ADDONS, true);

    Ref<DiskInstrumenter> di;
    di.instantiate();

    Dictionary result = di->instrument_to_disk();
    ASSERT_EQ(String(result["status"]), "ok");

    // src file should be instrumented (has marker)
    String patched_src = read_file(src_file);
    EXPECT_TRUE(patched_src.begins_with("# __NANO_COVERAGE_INSTRUMENTED__"));

    // test file should NOT be instrumented (was ignored by pattern)
    String test_content = read_file(test_file);
    EXPECT_FALSE(test_content.begins_with("# __NANO_COVERAGE_INSTRUMENTED__"));

    // Clean up
    di->restore_from_disk();
}
