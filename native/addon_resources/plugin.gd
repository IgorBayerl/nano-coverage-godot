@tool
extends EditorPlugin

var plugin_instance
var _integration_manager

func _enter_tree():
	print("--- 🔌 NanoCoverage Plugin Loading ---")
	
	# 1. Instantiate C++ Plugin
	if ClassDB.class_exists("NanoCoverageEditorPlugin"):
		plugin_instance = ClassDB.instantiate("NanoCoverageEditorPlugin")
		add_child(plugin_instance)
		print("✅ C++ Editor Plugin instantiated")
	else:
		printerr("❌ Error: NanoCoverageEditorPlugin class not found!")

	# 2. Check CoverageApi
	if not ClassDB.class_exists("CoverageApi"):
		printerr("❌ Error: CoverageApi class not found in ClassDB!")
		return
	
	var api = ClassDB.instantiate("CoverageApi")
	if not api:
		printerr("❌ Error: Failed to instantiate CoverageApi")
		return
	print("✅ CoverageApi instantiated")

	# 3. Load Integration Manager Script
	var manager_path = "res://addons/nano_coverage_godot/integrations/integration_manager.gd"
	if not FileAccess.file_exists(manager_path):
		printerr("❌ Error: Integration Manager file missing at: ", manager_path)
		return
		
	var ManagerScript = load(manager_path)
	if not ManagerScript:
		printerr("❌ Error: Failed to load Integration Manager script (Check for syntax errors in integration_manager.gd)")
		return
		
	# 4. Initialize Manager
	_integration_manager = ManagerScript.new(api)
	print("✅ Integration Manager initialized")

func _exit_tree():
	if _integration_manager:
		_integration_manager.clean_up()
	if plugin_instance:
		remove_child(plugin_instance)
		plugin_instance.queue_free()
		plugin_instance = null
