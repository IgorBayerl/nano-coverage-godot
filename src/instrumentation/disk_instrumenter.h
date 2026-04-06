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

    static String compute_file_hash(const String& path);
    static bool file_has_marker(const String& path);
    static String get_backup_path(const String& backup_dir, const String& res_path);

   protected:
    static void _bind_methods();

   public:
    /// Instruments all .gd files to disk with backups.
    /// Returns: { "status": "ok"|"error", "instrumented_count", "ignored_count", "failed_count", "error": ... }
    Dictionary instrument_to_disk();

    /// Restores all .gd files from backup.
    /// Returns: { "status": "ok"|"error", "restored_count", "warnings": [...], "error": ... }
    Dictionary restore_from_disk();

    /// Checks if the manifest exists (files are instrumented on disk).
    bool is_instrumented() const;
};

}  // namespace godot
