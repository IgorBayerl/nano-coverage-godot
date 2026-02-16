@tool
extends RefCounted

const PLUGIN_CFG = "res://addons/gdUnit4/plugin.cfg"
const GDUNIT_INSPECTOR_GROUP = "GdUnit4Inspector"
const TOOLBAR_SCRIPT_PATH = "res://addons/gdUnit4/src/ui/parts/InspectorToolBar.gd"

var _nano_api
var _run_button: Button
var _target_toolbar: Control
var _target_container: Control

func _init(nano_api):
	_nano_api = nano_api

static func is_available() -> bool:
	return FileAccess.file_exists(PLUGIN_CFG)

func enable():
	call_deferred("_inject_ui")

func disable():
	if _run_button:
		_run_button.queue_free()
		_run_button = null
	_target_toolbar = null
	_target_container = null

func _inject_ui():
	var base = EditorInterface.get_base_control()
	var inspector = _find_node_by_meta(base, GDUNIT_INSPECTOR_GROUP)
	if not inspector: return

	_target_toolbar = _find_node_by_script(inspector, TOOLBAR_SCRIPT_PATH)
	if not _target_toolbar: return

	_target_container = _target_toolbar.find_child("controls", true, false)
	if not _target_container: return

	if not _run_button:
		_run_button = Button.new()
		_run_button.text = "Cov"
		_run_button.tooltip_text = "Run Selected Tests with Nano Coverage"
		_run_button.flat = true
		_run_button.focus_mode = Control.FOCUS_NONE
		
		var editor_theme = EditorInterface.get_editor_theme()
		if editor_theme.has_icon("Play", "EditorIcons"):
			_run_button.icon = editor_theme.get_icon("Play", "EditorIcons")
			
		_run_button.pressed.connect(_on_run_pressed)
		_target_container.add_child(_run_button)
		_target_container.move_child(_run_button, max(0, _target_container.get_child_count() - 2))
		print("NanoCoverage: UI Injected successfully.")

func _on_run_pressed():
	print("\n==================================================")
	print("🚀 NanoCoverage: Preparing Intercepted Test Run...")
	
	# --- 1. Instrument Project FIRST ---
	# This safely copies the pristine GdUnit4 files to the Temp folder
	var result = _nano_api.instrument_project({})
	if result.has("error"):
		printerr("NanoCoverage Error: ", result.error)
		return
		
	var temp_path = result.output_path
	print("✅ NanoCoverage: Project instrumented at: ", temp_path)
	
	# --- 2. Backup original GdUnitTestRunner ---
	var runner_path = "res://addons/gdUnit4/src/core/runners/GdUnitTestRunner.gd"
	var original_code = FileAccess.get_file_as_string(runner_path)
	
	# --- 3. Write Interceptor to MAIN project ---
	# This forces the normal GdUnit run to redirect to our Temp folder!
	var interceptor_code = """extends Node
func _ready():
	print("NanoCoverage: Interceptor active! Forwarding to instrumented project...")
	var temp_path = "%s"
	
	# Copy the fresh config (which contains the new dynamic port) to Temp project
	var config_src = "res://addons/gdUnit4/GdUnitRunner.cfg"
	var config_dst = temp_path.path_join("addons/gdUnit4/GdUnitRunner.cfg")
	
	var dir = DirAccess.open("res://")
	if dir.file_exists(config_src):
		dir.copy(config_src, config_dst)
		
	# Launch the instrumented project
	var args = ["--path", temp_path, "res://addons/gdUnit4/src/core/runners/GdUnitTestRunner.tscn"]
	OS.create_process(OS.get_executable_path(), args)
	
	# Quit this dummy uninstrumented process
	get_tree().quit()
""" % temp_path.replace("\\", "/")

	var f = FileAccess.open(runner_path, FileAccess.WRITE)
	f.store_string(interceptor_code)
	f.close()
	
	# --- 4. Trigger the normal GdUnit4 Run button ---
	var run_btn = null
	for child in _target_container.get_children():
		if child is Button and child != _run_button:
			run_btn = child
			break
			
	if run_btn:
		print("NanoCoverage: Triggering GdUnit4 Server and Run...")
		run_btn.pressed.emit()
	else:
		printerr("NanoCoverage Error: Could not find GdUnit4 Run button!")
		
	# --- 5. Restore original script ---
	# We wait 3 seconds to ensure Godot has fully read and launched the interceptor
	await EditorInterface.get_base_control().get_tree().create_timer(3.0).timeout
	f = FileAccess.open(runner_path, FileAccess.WRITE)
	f.store_string(original_code)
	f.close()
	print("✅ NanoCoverage: Original GdUnit4 runner restored.")
	print("==================================================\n")

func _find_node_by_meta(root: Node, meta_key: String) -> Node:
	if root.has_meta(meta_key): return root
	for child in root.get_children():
		var res = _find_node_by_meta(child, meta_key)
		if res: return res
	return null

func _find_node_by_script(root: Node, script_path: String) -> Node:
	var scr = root.get_script()
	if scr and scr.resource_path == script_path: return root
	for child in root.get_children():
		var res = _find_node_by_script(child, script_path)
		if res: return res
	return null