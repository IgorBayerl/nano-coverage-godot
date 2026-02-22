extends Node
## The global NanoCoverage runtime singleton injected into the project.
##
## Responsible for receiving hit notifications from instrumented scripts
## and safely flushing the execution data to disk when the game exits.

const GDUNIT_SESSION_CLOSE := 9

var _api: Object = null
var _is_saved := false

func _ready() -> void:
	# Hook into GdUnit4 test session completion if available.
	# This ensures we dump the coverage data before Godot's chaotic teardown phase.
	if not Engine.has_meta("GdUnitSignals"):
		return
		
	var signals: Object = Engine.get_meta("GdUnitSignals")
	if signals.has_signal("gdunit_event") and not signals.gdunit_event.is_connected(_on_gdunit_event):
		signals.gdunit_event.connect(_on_gdunit_event)

func _exit_tree() -> void:
	if Engine.has_meta("GdUnitSignals"):
		var signals: Object = Engine.get_meta("GdUnitSignals")
		if signals.has_signal("gdunit_event") and signals.gdunit_event.is_connected(_on_gdunit_event):
			signals.gdunit_event.disconnect(_on_gdunit_event)
				
	# Fallback save if not triggered gracefully beforehand
	_do_save()

## Safely fetches the C++ singleton dynamically exactly when needed
func _get_api() -> Object:
	if _api != null and is_instance_valid(_api):
		return _api
		
	# Try fetching the Singleton by potential registered names
	for singleton_name in ["CoverageCollector", "NanoCoverage", "NanoCoverageGodot", "CoverageApi"]:
		if Engine.has_singleton(singleton_name):
			_api = Engine.get_singleton(singleton_name)
			break
			
	return _api

## Records a line execution hit
func hit(file_path: String, line: int) -> void:
	var api := _get_api()
	if api != null:
		api.hit(file_path, line)

func _on_gdunit_event(event: Object) -> void:
	# Save immediately when the session ends to avoid C++ std::ofstream 
	# crashing during Godot's late teardown phases.
	if event.has_method("type") and event.type() == GDUNIT_SESSION_CLOSE:
		_do_save()

func _notification(what: int) -> void:
	# Handle standard application exit requests
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_do_save()

func _do_save() -> void:
	if _is_saved:
		return
		
	var api := _get_api()
	if api != null and is_instance_valid(api) and api.has_method("save_session"):
		api.save_session()
		_is_saved = true