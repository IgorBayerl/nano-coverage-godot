extends SceneTree

func _init():
	var file = FileAccess.open("res://check_output.txt", FileAccess.WRITE)
	if ClassDB.class_exists("Instrumentor"):
		file.store_line("Instrumentor exists")
	else:
		file.store_line("Instrumentor MISSING")
		
	if ClassDB.class_exists("NanoCoverageRuntime"):
		file.store_line("NanoCoverageRuntime exists")
	else:
		file.store_line("NanoCoverageRuntime MISSING")
	file.close()
	quit()
