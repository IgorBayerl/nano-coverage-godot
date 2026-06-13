#include "disk_instrumenter.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "../api/coverage_api.h"
#include "../config/settings_gateway.h"
#include "../utils/logger.h"
#include "script_scanner.h"

namespace godot {

void DiskInstrumenter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("instrument_to_disk", "options"), &DiskInstrumenter::instrument_to_disk,
                         DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("restore_from_disk", "options"), &DiskInstrumenter::restore_from_disk,
                         DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("is_instrumented"), &DiskInstrumenter::is_instrumented);
}

// --- Helpers ---

String DiskInstrumenter::compute_file_hash(const String& path) {
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
    if (f.is_null()) {
        return "";
    }

    PackedByteArray data = f->get_buffer(f->get_length());
    f->close();

    Ref<HashingContext> ctx;
    ctx.instantiate();
    ctx->start(HashingContext::HASH_SHA256);
    ctx->update(data);
    PackedByteArray hash = ctx->finish();

    return hash.hex_encode();
}

bool DiskInstrumenter::file_has_marker(const String& path) {
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
    if (f.is_null()) {
        return false;
    }

    String first_line = f->get_line();
    f->close();
    return first_line.strip_edges() == MARKER;
}

String DiskInstrumenter::get_backup_path(const String& backup_dir, const String& res_path) {
    // res://src/player.gd -> backup_dir/backup/src/player.gd
    String relative = res_path;
    if (relative.begins_with("res://")) {
        relative = relative.substr(6);  // strip "res://"
    }
    return backup_dir.path_join("backup").path_join(relative);
}

// Writes content to a temp file first, then renames it over the target so a
// crash mid-write never leaves a half-written script behind.
bool DiskInstrumenter::write_file_atomic(const String& path, const String& content) {
    String dir_path = path.get_base_dir();
    if (!DirAccess::dir_exists_absolute(dir_path)) {
        DirAccess::make_dir_recursive_absolute(dir_path);
    }

    String tmp_path = path + String(".tmp");
    Ref<FileAccess> f = FileAccess::open(tmp_path, FileAccess::WRITE);
    if (f.is_null()) {
        return false;
    }
    f->store_string(content);
    f->close();

    Ref<DirAccess> da = DirAccess::open(dir_path);
    if (da.is_null()) {
        return false;
    }
    return da->rename(tmp_path, path) == OK;
}

// --- Runtime autoload registration ---

void DiskInstrumenter::register_runtime_autoload(bool save_settings) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    String value = String("*") + AUTOLOAD_SCRIPT;
    if (ps->has_setting(AUTOLOAD_SETTING) && String(ps->get_setting(AUTOLOAD_SETTING)) == value) {
        return;
    }
    ps->set_setting(AUTOLOAD_SETTING, value);
    if (save_settings) {
        ps->save();
    }
    Logger::info("Registered runtime autoload (flushes coverage data when the game exits).");
}

void DiskInstrumenter::unregister_runtime_autoload(bool save_settings) {
    ProjectSettings* ps = ProjectSettings::get_singleton();
    if (!ps->has_setting(AUTOLOAD_SETTING)) {
        return;
    }
    ps->set_setting(AUTOLOAD_SETTING, Variant());
    if (save_settings) {
        ps->save();
    }
    Logger::info("Removed runtime autoload.");
}

// --- Recursive directory removal ---

static void remove_dir_recursive(const String& dir_path) {
    Ref<DirAccess> da = DirAccess::open(dir_path);
    if (da.is_null())
        return;

    da->list_dir_begin();
    String name = da->get_next();
    while (!name.is_empty()) {
        if (name != "." && name != "..") {
            String full = dir_path.path_join(name);
            if (da->current_is_dir()) {
                remove_dir_recursive(full);
            } else {
                da->remove(name);
            }
        }
        name = da->get_next();
    }
    da->list_dir_end();

    // Remove the now-empty directory
    DirAccess::remove_absolute(dir_path);
}

// --- Main operations ---

bool DiskInstrumenter::is_instrumented() const {
    CoverageSettings settings = SettingsGateway::load();
    String manifest_path = settings.backup_dir.path_join(MANIFEST_FILENAME);
    return FileAccess::file_exists(manifest_path);
}

Dictionary DiskInstrumenter::instrument_to_disk(const Dictionary& options) {
    Dictionary result;
    bool save_project_settings = options.get("save_project_settings", true);

    CoverageSettings settings = SettingsGateway::load();
    String backup_dir = settings.backup_dir;
    String manifest_path = backup_dir.path_join(MANIFEST_FILENAME);

    // Double-instrument protection
    if (FileAccess::file_exists(manifest_path)) {
        result["status"] = "error";
        result["error"] = "Files are already instrumented. Restore first.";
        Logger::error("Files are already instrumented. Restore first.");
        return result;
    }

    Array files = ScriptScanner::scan_project(settings);

    Logger::info("Disk instrumentation: found " + String::num_int64(files.size()) + " .gd files");

    // Check for marker in any file before proceeding
    for (int i = 0; i < files.size(); ++i) {
        String path = files[i];
        if (file_has_marker(path)) {
            result["status"] = "error";
            result["error"] =
                "File already contains instrumentation marker: " + path + ". A previous backup may have been lost.";
            Logger::error("Aborting: file already contains marker: " + path);
            return result;
        }
    }

    Ref<CoverageApi> api;
    api.instantiate();
    int instrumented_count = 0;
    int ignored_count = 0;
    int failed_count = 0;
    Array manifest_files;

    for (int i = 0; i < files.size(); ++i) {
        String path = files[i];

        // Read source from disk
        Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
        if (f.is_null()) {
            Logger::error("Cannot read file: " + path);
            failed_count++;
            continue;
        }
        String source = f->get_as_text();
        f->close();

        // Instrument
        Dictionary res = api->instrument_script(source, path);

        if (!res.has("success") || !bool(res["success"])) {
            Logger::error("Failed to instrument: " + path);
            failed_count++;
            continue;
        }

        if (res.has("ignored") && bool(res["ignored"])) {
            ignored_count++;
            continue;
        }

        String instrumented_code = res["code"];

        // Compute hash of original
        String original_hash = compute_file_hash(path);

        // Backup original
        String bkp_path = get_backup_path(backup_dir, path);
        if (!write_file_atomic(bkp_path, source)) {
            Logger::error("Cannot write backup for: " + path);
            failed_count++;
            continue;
        }

        // Write instrumented code with marker to original path
        String marked_code = String(MARKER) + "\n" + instrumented_code;
        if (!write_file_atomic(path, marked_code)) {
            Logger::error("Cannot write instrumented file: " + path);
            failed_count++;
            continue;
        }

        // Record manifest entry
        Dictionary entry;
        entry["path"] = path;
        entry["backup_path"] = bkp_path;
        entry["original_hash"] = "sha256:" + original_hash;
        manifest_files.push_back(entry);

        instrumented_count++;
    }

    // Save manifest (only if we instrumented something)
    if (instrumented_count > 0) {
        Dictionary manifest;
        manifest["version"] = 1;
        manifest["files"] = manifest_files;

        String json_str = JSON::stringify(manifest, "  ");
        String manifest_dir = manifest_path.get_base_dir();
        if (!DirAccess::dir_exists_absolute(manifest_dir)) {
            DirAccess::make_dir_recursive_absolute(manifest_dir);
        }
        Ref<FileAccess> mf = FileAccess::open(manifest_path, FileAccess::WRITE);
        if (mf.is_valid()) {
            mf->store_string(json_str);
            mf->close();
        }

        // Save static metadata for coverage reporting
        api->save_static_metadata();

        // Make sure hits collected during a normal Play session get flushed
        register_runtime_autoload(save_project_settings);
    }

    result["status"] = "ok";
    result["instrumented_count"] = instrumented_count;
    result["ignored_count"] = ignored_count;
    result["failed_count"] = failed_count;

    Logger::info("--- Disk Instrumentation Complete ---");
    Logger::info("Scripts Instrumented: " + String::num_int64(instrumented_count));
    Logger::info("Scripts Ignored: " + String::num_int64(ignored_count));
    if (failed_count > 0) {
        Logger::error("Scripts Failed: " + String::num_int64(failed_count));
    }

    return result;
}

Dictionary DiskInstrumenter::restore_from_disk(const Dictionary& options) {
    Dictionary result;
    bool save_project_settings = options.get("save_project_settings", true);

    CoverageSettings settings = SettingsGateway::load();
    String backup_dir = settings.backup_dir;
    String manifest_path = backup_dir.path_join(MANIFEST_FILENAME);

    if (!FileAccess::file_exists(manifest_path)) {
        result["status"] = "error";
        result["error"] = "No manifest found. Nothing to restore.";
        Logger::warn("Restore called but no manifest found.");
        return result;
    }

    // Read and parse manifest
    Ref<FileAccess> mf = FileAccess::open(manifest_path, FileAccess::READ);
    if (mf.is_null()) {
        result["status"] = "error";
        result["error"] = "Cannot read manifest file.";
        return result;
    }
    String json_str = mf->get_as_text();
    mf->close();

    Variant parsed = JSON::parse_string(json_str);
    if (parsed.get_type() != Variant::DICTIONARY) {
        result["status"] = "error";
        result["error"] = "Manifest is not valid JSON.";
        return result;
    }

    Dictionary manifest = parsed;
    Array files = manifest.get("files", Array());
    int restored_count = 0;
    Array warnings;

    for (int i = 0; i < files.size(); ++i) {
        Dictionary entry = files[i];
        String path = entry["path"];
        String bkp_path = entry["backup_path"];
        String expected_hash = entry.get("original_hash", "");

        // Read backup file
        Ref<FileAccess> bkp_f = FileAccess::open(bkp_path, FileAccess::READ);
        if (bkp_f.is_null()) {
            Logger::error("Cannot read backup file: " + bkp_path);
            warnings.push_back("Missing backup: " + bkp_path);
            continue;
        }
        String backup_content = bkp_f->get_as_text();
        bkp_f->close();

        // Verify hash
        if (!expected_hash.is_empty()) {
            String actual_hash = "sha256:" + compute_file_hash(bkp_path);
            if (actual_hash != expected_hash) {
                Logger::warn("Hash mismatch for backup of " + path + " (expected: " + expected_hash +
                             ", got: " + actual_hash + "). Restoring anyway.");
                warnings.push_back("Hash mismatch: " + path);
            }
        }

        // Write restored content to original path
        Ref<FileAccess> out_f = FileAccess::open(path, FileAccess::WRITE);
        if (out_f.is_null()) {
            Logger::error("Cannot write restored file: " + path);
            warnings.push_back("Cannot write: " + path);
            continue;
        }
        out_f->store_string(backup_content);
        out_f->close();

        restored_count++;
    }

    // Delete manifest
    {
        Ref<DirAccess> da = DirAccess::open(backup_dir);
        if (da.is_valid()) {
            da->remove(MANIFEST_FILENAME);
        }
    }

    // Delete backup directory recursively
    {
        String backup_subdir = backup_dir.path_join("backup");
        if (DirAccess::dir_exists_absolute(backup_subdir)) {
            remove_dir_recursive(backup_subdir);
        }
        // Try to remove the backup_dir itself if empty
        DirAccess::remove_absolute(backup_dir);
    }

    unregister_runtime_autoload(save_project_settings);

    result["status"] = "ok";
    result["restored_count"] = restored_count;
    result["warnings"] = warnings;

    Logger::info("--- Disk Restore Complete ---");
    Logger::info("Scripts Restored: " + String::num_int64(restored_count));
    if (warnings.size() > 0) {
        Logger::warn("Warnings: " + String::num_int64(warnings.size()));
    }

    return result;
}

}  // namespace godot
