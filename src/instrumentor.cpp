#include "instrumentor.h"
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/templates/hash_set.hpp>

// Tree-sitter includes
#include <tree_sitter/api.h>

// Declaration for the external grammar function
extern "C" const TSLanguage *tree_sitter_gdscript();

using namespace godot;

void Instrumentor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("process_project", "source_dir", "dest_dir"), &Instrumentor::process_project);
}

Instrumentor::Instrumentor() {
	parser = ts_parser_new();
	lang_gdscript = tree_sitter_gdscript();
	ts_parser_set_language(parser, lang_gdscript);
}

Instrumentor::~Instrumentor() {
	if (parser) {
		ts_parser_delete(parser);
	}
}

Error Instrumentor::process_project(const String &source_dir, const String &dest_dir) {
	// 1. Copy Project
	Error err = copy_recursive(source_dir, dest_dir);
	if (err != OK) return err;

	// 2. Inject Autoload
	inject_autoload(dest_dir.path_join("project.godot"));
	
	return OK;
}

Error Instrumentor::copy_recursive(const String &src, const String &dst) {
	Ref<DirAccess> dir = DirAccess::open(src);
	if (dir.is_null()) return ERR_CANT_OPEN;

	dir->make_dir_recursive_absolute(dst);
	dir->list_dir_begin();
	String name = dir->get_next();

	while (name != "") {
		if (name == "." || name == "..") {
			name = dir->get_next();
			continue;
		}

		String src_path = src.path_join(name);
		String dst_path = dst.path_join(name);

		if (dir->current_is_dir()) {
			if (name != ".git" && name != ".godot" && name != "coverage" && name != "addons") {
				copy_recursive(src_path, dst_path);
			}
		} else {
			Ref<FileAccess> f_src = FileAccess::open(src_path, FileAccess::READ);
			Ref<FileAccess> f_dst = FileAccess::open(dst_path, FileAccess::WRITE);
			if (f_src.is_valid() && f_dst.is_valid()) {
				f_dst->store_buffer(f_src->get_buffer(f_src->get_length()));
				f_src->close();
				f_dst->close();

				if (name.get_extension() == "gd") {
					// Instrument the copied file in-place
					instrument_file(dst_path, dst);
				}
			}
		}
		name = dir->get_next();
	}
	return OK;
}

// --- TREE WALKING LOGIC ---

// Recursive helper to find instrumentable lines
void collect_lines(TSNode node, HashSet<int> &lines) {
	// Get node type
	const char *type = ts_node_type(node);
	String type_str = String(type);

	// These are node types in tree-sitter-gdscript that roughly correspond to "statements"
	// that we want to track coverage for.
	bool is_statement = 
		type_str == "expression_statement" ||
		type_str == "variable_statement" ||
		type_str == "return_statement" ||
		type_str == "break_statement" ||
		type_str == "continue_statement" ||
		type_str == "pass_statement" || 
		type_str == "if_statement" ||     // Covers the 'if' line itself
		type_str == "for_statement" ||
		type_str == "while_statement";

	if (is_statement) {
		TSPoint start = ts_node_start_point(node);
		// Tree-sitter uses 0-based rows. LCOV and Godot editor use 1-based.
		// We store 0-based here for array indexing, convert later.
		lines.insert(start.row);
	}

	// Recurse children
	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++) {
		collect_lines(ts_node_child(node, i), lines);
	}
}

Error Instrumentor::instrument_file(const String &file_path, const String &project_root_dest) {
	Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
	if (f.is_null()) return ERR_FILE_CANT_READ;
	
	String content = f->get_as_text();
	f->close();

	// 1. Parse content
	CharString cs = content.utf8();
	TSTree *tree = ts_parser_parse_string(parser, NULL, cs.get_data(), cs.length());
	TSNode root_node = ts_tree_root_node(tree);

	// 2. Identify lines to instrument
	HashSet<int> lines_to_hit;
	collect_lines(root_node, lines_to_hit);

	// 3. Reconstruct file
	// We read line by line and prepend the hit counter if the line index is in our set.
	PackedStringArray file_lines = content.split("\n");
	String new_content = "";

	// Calculate "Original Resource Path" for the report
	String relative_path = file_path.replace(project_root_dest, "").replace("\\", "/");
	if (relative_path.begins_with("/")) relative_path = relative_path.substr(1);
	String res_path = "res://" + relative_path;

	for (int i = 0; i < file_lines.size(); i++) {
		String line = file_lines[i];
		
		if (lines_to_hit.has(i)) {
			// Calculate indentation
			int indent_len = 0;
			while (indent_len < line.length() && (line[indent_len] == ' ' || line[indent_len] == '\t')) {
				indent_len++;
			}
			
			String indent = line.substr(0, indent_len);
			String code = line.substr(indent_len);
			
			// If line starts with annotation or comments, skip (double check, though AST should handle this)
			if (code.begins_with("#") || code.begins_with("@")) {
				new_content += line + "\n";
			} else {
				// Inject: NanoCoverage.hit("res://...", line_num); original_code
				new_content += indent + "NanoCoverage.hit('" + res_path + "', " + String::num_int64(i + 1) + "); " + code + "\n";
			}
		} else {
			new_content += line + "\n";
		}
	}

	ts_tree_delete(tree);

	f = FileAccess::open(file_path, FileAccess::WRITE);
	f->store_string(new_content);
	f->close();

	return OK;
}

void Instrumentor::inject_autoload(const String &project_godot_path) {
	// Same logic as previous answer: append [autoload] NanoCoverage=...
	Ref<FileAccess> f = FileAccess::open(project_godot_path, FileAccess::READ_WRITE);
	if (f.is_null()) return;

	f->seek_end();
	f->store_line("");
	f->store_line("[autoload]");
	f->store_line("NanoCoverage=\"*res://addons/nano_coverage_dummy.gd\"");
	f->close();

	// Create dummy script
	String addons_dir = project_godot_path.get_base_dir().path_join("addons");
	String dummy_path = addons_dir.path_join("nano_coverage_dummy.gd");
	
	Ref<DirAccess> da = DirAccess::open(project_godot_path.get_base_dir());
	if (da.is_valid()) {
		da->make_dir_recursive_absolute(addons_dir);
	}
	
	Ref<FileAccess> fd = FileAccess::open(dummy_path, FileAccess::WRITE);
	if (fd.is_valid()) {
		fd->store_line("extends NanoCoverageRuntime");
		fd->close();
	}
}
