#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class DiskInstrumenter : public RefCounted {
    GDCLASS(DiskInstrumenter, RefCounted)

   private:
    static constexpr const char* MARKER = "# __NANO_COVERAGE_INSTRUMENTED__";
    static constexpr const char* MANIFEST_FILENAME = "manifest.json";

    // Autoload that flushes in-memory hits to disk when the game exits.
    // Registered while files are instrumented, removed on restore.
    static constexpr const char* AUTOLOAD_SETTING = "autoload/NanoCoverageRuntime";
    static constexpr const char* AUTOLOAD_SCRIPT = "res://addons/nano_coverage_godot/runtime/coverage_autoload.gd";

    static String compute_file_hash(const String& path);
    static bool file_has_marker(const String& path);
    static String get_backup_path(const String& backup_dir, const String& res_path);
    static bool write_file_atomic(const String& path, const String& content);

    static void register_runtime_autoload(bool save_settings);
    static void unregister_runtime_autoload(bool save_settings);

   protected:
    static void _bind_methods();

   public:
    /// Instruments all .gd files to disk with backups and registers the
    /// runtime autoload so a normal Play session flushes coverage on exit.
    /// Options: { "save_project_settings": bool (default true) }
    /// Returns: { "status": "ok"|"error", "instrumented_count", "ignored_count", "failed_count", "error": ... }
    Dictionary instrument_to_disk(const Dictionary& options = Dictionary());

    /// Restores all .gd files from backup and removes the runtime autoload.
    /// Options: { "save_project_settings": bool (default true) }
    /// Returns: { "status": "ok"|"error", "restored_count", "warnings": [...], "error": ... }
    Dictionary restore_from_disk(const Dictionary& options = Dictionary());

    /// Checks if the manifest exists (files are instrumented on disk).
    bool is_instrumented() const;
};

}  // namespace godot
