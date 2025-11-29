#ifndef INSTRUMENTOR_H
#define INSTRUMENTOR_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

// Forward decls
struct TSLanguage;
struct TSParser;
struct TSTree;

namespace godot {

class Instrumentor : public RefCounted {
	GDCLASS(Instrumentor, RefCounted);

protected:
	static void _bind_methods();

public:
	Instrumentor();
	~Instrumentor();
	
	// Main entry point exposed to GDScript
	Error process_project(const String &source_dir, const String &dest_dir);

private:
	TSParser *parser = nullptr;
	const TSLanguage *lang_gdscript = nullptr;

	Error copy_recursive(const String &src, const String &dst);
	Error instrument_file(const String &file_path, const String &project_root_dest);
	
	// Helper to inject the autoload entry into project.godot
	void inject_autoload(const String &project_godot_path);
};

}
#endif
