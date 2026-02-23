class_name NanoCoverageBootstrap
extends RefCounted

static func _get_all_files(path: String, extension: String) -> Array[String]:
	print("AAAAAAAAAAAAAAAAAA")
	var files: Array[String] = []
	var dir = DirAccess.open(path)
	if dir:
		print("DDDDD")
		dir.list_dir_begin()
		var file_name = dir.get_next()
		while file_name != "":
			print("CCCCCC")
			if dir.current_is_dir() and not file_name.begins_with("."):
				var sub_path = path.path_join(file_name)
				print("BBBBBB")
				
				# CHANGE: Only exclude NanoCoverage and GdUnit4
				# We want to allow instrumenting other addons
				if sub_path.begins_with("res://addons/nano_coverage_godot"):
					file_name = dir.get_next()
					continue
					
				files.append_array(_get_all_files(sub_path, extension))
			elif not dir.current_is_dir():
				if file_name.ends_with(extension):
					files.append(path.path_join(file_name))
			file_name = dir.get_next()
	return files

static func instrument_all_scripts() -> void:
	var api = CoverageApi.new()
	var files = _get_all_files("res://", ".gd")

	var instrumented_count = 0
	for path in files:
		var script = load(path)
		if script is GDScript:
			var res = api.instrument_script(script.source_code, path)
			if res.has("success") and res["success"] == true:
				script.source_code = res.get("code", "")
				script.reload(true)
				instrumented_count += 1
			else:
				push_error("NanoCoverage: Failed to instrument ", path, " - ", res.get("error", "Unknown error"))

	api.save_static_metadata()
	print("NanoCoverage: Successfully instrumented %d scripts in memory." % instrumented_count)
