@tool
extends RefCounted
## Integrates NanoCoverage seamlessly into the GdUnit4 Editor UI.
##
## Locates the GdUnit4 Inspector panel and injects custom coverage execution buttons.
## When triggered, it intercepts the standard GdUnit4 test session, instruments the
## project, and reroutes the execution to the temporary instrumented project.
##
## It listens to GdUnit4's TCP events to detect when a test session completes, ensuring
## the coverage data is safely merged and reported.

const PLUGIN_CFG := "res://addons/gdUnit4/plugin.cfg"
const GDUNIT_INSPECTOR_GROUP := "GdUnit4Inspector"
const TOOLBAR_SCRIPT_PATH := "res://addons/gdUnit4/src/ui/parts/InspectorToolBar.gd"
const GDUNIT_SESSION_CLOSE := 9

var _nano_api: Object
var _run_button: Button
var _run_all_button: Button
var _target_toolbar: Control
var _target_container: Control
var _is_coverage_run := false
var _runner_pid: int = -1

func _init(nano_api: Object) -> void:
	_nano_api = nano_api

## Returns true if the GdUnit4 plugin is present in the project.
static func is_available() -> bool:
	return FileAccess.file_exists(PLUGIN_CFG)

## Enables the integration by injecting the UI and hooking into the event system.
func enable() -> void:
	call_deferred("_inject_ui")
	
	if not Engine.has_meta("GdUnitSignals"):
		return
		
	var signals: Object = Engine.get_meta("GdUnitSignals")
	if signals.has_signal("gdunit_event") and not signals.gdunit_event.is_connected(_on_gdunit_event):
		signals.gdunit_event.connect(_on_gdunit_event)

## Disables the integration, removing UI elements and signal connections.
func disable() -> void:
	if _run_button:
		_run_button.queue_free()
		_run_button = null

	if _run_all_button:
		_run_all_button.queue_free()
		_run_all_button = null
		
	_target_toolbar = null
	_target_container = null
	
	if not Engine.has_meta("GdUnitSignals"):
		return
		
	var signals: Object = Engine.get_meta("GdUnitSignals")
	if signals.has_signal("gdunit_event") and signals.gdunit_event.is_connected(_on_gdunit_event):
		signals.gdunit_event.disconnect(_on_gdunit_event)

func _inject_ui() -> void:
	var base := EditorInterface.get_base_control()
	var inspector := _find_node_by_meta(base, GDUNIT_INSPECTOR_GROUP)
	if not inspector:
		return

	_target_toolbar = _find_node_by_script(inspector, TOOLBAR_SCRIPT_PATH)
	if not _target_toolbar:
		return

	_target_container = _target_toolbar.find_child("controls", true, false)
	if not _target_container:
		return

	if _run_button:
		return

	# 1. Create "Run Selected" Button
	_run_button = Button.new()
	_apply_button_style(_run_button)
	_run_button.text = "Cov"
	_run_button.tooltip_text = "Run Selected Tests with Nano Coverage"
	
	# 2. Create "Run All" Button
	_run_all_button = Button.new()
	_apply_button_style(_run_all_button)
	_run_all_button.text = "All"
	_run_all_button.tooltip_text = "Run ALL Tests with Nano Coverage"

	var editor_theme := EditorInterface.get_editor_theme()
	if editor_theme.has_icon("Play", "EditorIcons"):
		var icon = editor_theme.get_icon("Play", "EditorIcons")
		_run_button.icon = icon
		_run_all_button.icon = icon
		
	_run_button.pressed.connect(_on_run_pressed)
	_run_all_button.pressed.connect(_on_run_all_pressed)

	_target_container.add_child(_run_button)
	_target_container.add_child(_run_all_button)
	
	# Move the buttons right next to the standard GdUnit run buttons
	# We place "All" first, then "Selected" to match standard UI patterns
	var pos = max(0, _target_container.get_child_count() - 3)
	_target_container.move_child(_run_all_button, pos)
	_target_container.move_child(_run_button, pos + 1)
	
	print("[NanoCoverage] GdUnit4 UI Integration active.")

func _apply_button_style(btn: Button) -> void:
	# Match GdUnit Style by copying from a sibling
	var sibling: Button = null
	for child in _target_container.get_children():
		if child is Button:
			sibling = child
			break
	
	if sibling:
		btn.flat = sibling.flat
		btn.theme_type_variation = sibling.theme_type_variation
		btn.focus_mode = sibling.focus_mode
		btn.mouse_default_cursor_shape = sibling.mouse_default_cursor_shape
		btn.size_flags_vertical = sibling.size_flags_vertical
		btn.size_flags_horizontal = sibling.size_flags_horizontal
		return

	# Fallback
	btn.flat = true
	btn.focus_mode = Control.FOCUS_NONE

func _on_run_pressed() -> void:
	var base_control := EditorInterface.get_base_control()
	var inspector: Object = base_control.get_meta("GdUnit4Inspector")
	if not inspector:
		printerr("[NanoCoverage] Could not find GdUnit4 inspector panel.")
		return
		
	var selected_item: TreeItem = inspector._tree.get_selected()
	var tests_to_execute: Array = inspector.collect_test_cases(selected_item)
	
	_run_coverage_session(tests_to_execute)

func _on_run_all_pressed() -> void:
	# FIX: Instead of passing "res://", we grab the root of the test tree 
	# and collect actual GdUnitTestCase objects from it.
	var base_control := EditorInterface.get_base_control()
	var inspector: Object = base_control.get_meta("GdUnit4Inspector")
	if not inspector:
		printerr("[NanoCoverage] Could not find GdUnit4 inspector panel.")
		return
	
	# Access the Tree directly
	var tree: Tree = inspector._tree
	if not tree or not tree.get_root():
		printerr("[NanoCoverage] No tests discovered yet. Please wait for GdUnit to scan.")
		return
		
	var root: TreeItem = tree.get_root()
	var tests_to_execute: Array = inspector.collect_test_cases(root)
	
	_run_coverage_session(tests_to_execute)

func _run_coverage_session(tests_to_execute: Array) -> void:
	print("\n--------------------------------------------------")
	print("[NanoCoverage] Starting GdUnit4 Test Session...")

	if tests_to_execute.is_empty():
		printerr("[NanoCoverage] No tests selected.")
		return
		
	_nano_api.clear_coverage_data({"workspace_id": "gdunit4"})
	
	var command_handler: Object = Engine.get_meta("GdUnitCommandHandler") if Engine.has_meta("GdUnitCommandHandler") else null
	if not command_handler:
		printerr("[NanoCoverage] Could not find GdUnitCommandHandler.")
		return
		
	# Prepare the test session (Generates GdUnitRunner.cfg and boots TCP server)
	# Expects Array[GdUnitTestCase]
	var test_session_command: Object = command_handler.test_session_command
	test_session_command._prepare_test_session(tests_to_execute)
	
	# Instrument Project
	var instr_opts := {
		"exclude": [
			"res://addons/nano_coverage_godot"
		]
	}
	
	var result: Dictionary = _nano_api.instrument_project(instr_opts)
	if result.has("error"):
		printerr("[NanoCoverage] Instrumentation failed: ", result.error)
		return
		
	var temp_path: String = result.output_path
	print("[NanoCoverage] Project instrumented at: ", temp_path)
	
	# Copy the GdUnit configuration to the Temp project
	var config_src := "res://addons/gdUnit4/GdUnitRunner.cfg"
	var config_dst := temp_path.path_join("addons/gdUnit4/GdUnitRunner.cfg")
	var dir := DirAccess.open("res://")
	
	if not dir.file_exists(config_src):
		printerr("[NanoCoverage] Could not find GdUnitRunner.cfg.")
		return
		
	DirAccess.copy_absolute(ProjectSettings.globalize_path(config_src), config_dst)
	
	# Modify project.godot in Temp project to directly boot the GdUnit Test Runner
	var cfg := ConfigFile.new()
	var proj_file := temp_path.path_join("project.godot")
	if cfg.load(proj_file) == OK:
		cfg.set_value("application", "run/main_scene", "res://addons/gdUnit4/src/core/runners/GdUnitTestRunner.tscn")
		cfg.save(proj_file)
	else:
		printerr("[NanoCoverage] Failed to modify project.godot in temp path.")
		return
	
	var run_opts := {
		"output_path": temp_path,
		"workspace_id": "gdunit4",
		"blocking": false
	}
	
	var run_result: Dictionary = _nano_api.run_instrumented_project(run_opts)
	if run_result.has("error"):
		printerr("[NanoCoverage] Run failed: ", run_result.error)
		return
		
	if run_result.has("pid"):
		_runner_pid = run_result.pid
		# Inject the PID back into GdUnit4 so the native "Stop" button functions
		test_session_command._current_runner_process_id = _runner_pid
		test_session_command._is_running = true
		_is_coverage_run = true
		print("[NanoCoverage] GdUnit4 Test Runner started (PID: ", _runner_pid, ")")
	
	print("--------------------------------------------------\n")

## Intercepts GdUnit4 events to detect when the session ends.
func _on_gdunit_event(event: Object) -> void:
	if not _is_coverage_run:
		return
		
	if event.has_method("type") and event.type() == GDUNIT_SESSION_CLOSE:
		_is_coverage_run = false
		print("\n[NanoCoverage] GdUnit4 Test Session finished. Waiting for runner process to exit...")
		_wait_for_process_and_generate.call_deferred()

func _wait_for_process_and_generate() -> void:
	if _runner_pid != -1:
		# Wait until OS process is fully dead to ensure the file is completely flushed
		while OS.is_process_running(_runner_pid):
			await EditorInterface.get_base_control().get_tree().create_timer(0.2).timeout
		_runner_pid = -1
		
	# A tiny margin to ensure the OS has released all file locks
	await EditorInterface.get_base_control().get_tree().create_timer(0.2).timeout
		
	print("[NanoCoverage] Generating report...")
	var report_opts := {"workspace_id": "gdunit4"}
	var report_result: Dictionary = _nano_api.generate_coverage_report(report_opts)
	
	if report_result.has("status") and report_result.status == "ok":
		print("[NanoCoverage] Report generated successfully at: ", report_result.report_path)
	else:
		printerr("[NanoCoverage] Report generation failed: ", report_result.get("error", "Unknown error"))

func _find_node_by_meta(root: Node, meta_key: String) -> Node:
	if root.has_meta(meta_key):
		return root
		
	for child in root.get_children():
		var res := _find_node_by_meta(child, meta_key)
		if res: return res
		
	return null

func _find_node_by_script(root: Node, script_path: String) -> Node:
	var scr: Script = root.get_script()
	if scr and scr.resource_path == script_path:
		return root
		
	for child in root.get_children():
		var res := _find_node_by_script(child, script_path)
		if res: return res
		
	return null