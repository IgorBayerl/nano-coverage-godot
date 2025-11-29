extends SceneTree

func _init():
	print("Starting Instrumentation...")
	
	# We are running inside 'demo' project.
	var source_dir = "res://"
	# We want to build to '../build_demo' (relative to project root)
	# ProjectSettings.globalize_path("res://") gives absolute path to demo/
	var abs_source = ProjectSettings.globalize_path("res://")
	var abs_dest = abs_source.path_join("../build_demo").simplify_path()
	
	print("Source: ", abs_source)
	print("Dest: ", abs_dest)
	
	if !ClassDB.class_exists("Instrumentor"):
		printerr("Error: Instrumentor class not found. Is the GDExtension loaded?")
		quit(1)
		return

	var instrumentor = ClassDB.instantiate("Instrumentor")
	
	var err = instrumentor.process_project(abs_source, abs_dest)
	if err != OK:
		printerr("Instrumentation failed with error: ", err)
		quit(1)
	else:
		print("Instrumentation successful!")
		quit(0)
