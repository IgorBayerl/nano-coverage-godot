class_name NanoCoverageBootstrap
extends RefCounted

static func _is_ignored(path: String) -> bool:
	var ignore_paths = ProjectSettings.get_setting("nano_coverage/paths_ignore", [
		"res://addons/nano_coverage_godot",
		"res://addons/gdUnit4/src/network",
		"res://addons/gdUnit4/src/core/runners",
		"res://addons/gdUnit4/src/ui",
		"res://addons/gdUnit4/bin",
		"res://addons/gdUnit4/src/core/hooks",
		"res://addons/gdUnit4/test"
	])
	
	for ignored in ignore_paths:
		if path.begins_with(ignored):
			return true
	return false

static func _get_all_files(path: String, extension: String) -> Array[String]:
	var files: Array[String] = []
	var dir = DirAccess.open(path)
	
	if dir:
		dir.list_dir_begin()
		var file_name = dir.get_next()
		while file_name != "":
			if dir.current_is_dir() and not file_name.begins_with("."):
				var sub_path = path.path_join(file_name)
				
				if _is_ignored(sub_path):
					file_name = dir.get_next()
					continue
					
				files.append_array(_get_all_files(sub_path, extension))
			elif not dir.current_is_dir():
				if file_name.ends_with(extension):
					var full_path = path.path_join(file_name)
					if not _is_ignored(full_path):
						files.append(full_path)
			file_name = dir.get_next()
	else:
		NanoCoverageLogger.error("Failed to open directory: " + path)
		
	return files

static func instrument_all_scripts() -> void:
	NanoCoverageLogger.info("--- Starting Memory Instrumentation ---")
	var api = CoverageApi.new()
	var files = _get_all_files("res://", ".gd")
	
	NanoCoverageLogger.info("Total GDScript files found to instrument: " + str(files.size()))

	var instrumented_count = 0
	var ignored_count = 0
	
	for path in files:
		var script = load(path)
		
		if script is GDScript:
			var res = api.instrument_script(script.source_code, path)
			if res.has("success") and res["success"] == true:
				if res.has("ignored") and res["ignored"] == true:
					# print("[NanoCoverage]  -> Ignored (0 coverable lines): ", path)
					ignored_count += 1
				else:
					script.source_code = res.get("code", "")
					script.reload(true)
					instrumented_count += 1
			else:
				NanoCoverageLogger.error(" -> Failed to instrument: " + path + " | " + res.get("error", "Unknown error"))

	NanoCoverageLogger.info("Saving static coverage metadata...")
	api.save_static_metadata()
	NanoCoverageLogger.info("--- Instrumentation Complete ---")
	NanoCoverageLogger.info("Scripts Patched: " + str(instrumented_count))
	NanoCoverageLogger.info("Scripts Ignored (0 Lines): " + str(ignored_count))