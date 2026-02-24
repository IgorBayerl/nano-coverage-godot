extends Node
## The global NanoCoverage runtime singleton injected into the project.

const GDUNIT_SESSION_CLOSE := 9

var _api: Object = null
var _is_saved := false

func _ready() -> void:
	print("[NanoCoverage Runtime] Injected into SceneTree. Monitoring for hits...")
	if not Engine.has_meta("GdUnitSignals"):
		return
		
	print("[NanoCoverage Runtime] GdUnit4 detected. Hooking into test session signals.")
	var signals: Object = Engine.get_meta("GdUnitSignals")
	if signals.has_signal("gdunit_event") and not signals.gdunit_event.is_connected(_on_gdunit_event):
		signals.gdunit_event.connect(_on_gdunit_event)

func _exit_tree() -> void:
	print("[NanoCoverage Runtime] Exiting tree...")
	if Engine.has_meta("GdUnitSignals"):
		var signals: Object = Engine.get_meta("GdUnitSignals")
		if signals.has_signal("gdunit_event") and signals.gdunit_event.is_connected(_on_gdunit_event):
			signals.gdunit_event.disconnect(_on_gdunit_event)
				
	_do_save()

func _get_api() -> Object:
	if _api != null and is_instance_valid(_api):
		return _api
		
	# TODO: Fix this hacky way of getting the API
	for singleton_name in ["CoverageCollector", "NanoCoverage", "NanoCoverageGodot", "CoverageApi"]:
		if Engine.has_singleton(singleton_name):
			_api = Engine.get_singleton(singleton_name)
			break
			
	return _api

func hit(file_path: String, line: int) -> void:
	var api := _get_api()
	if api != null:
		api.hit(file_path, line)

func _on_gdunit_event(event: Object) -> void:
	if event.has_method("type") and event.type() == GDUNIT_SESSION_CLOSE:
		print("[NanoCoverage Runtime] Intercepted GdUnit4 Session Close event.")
		_do_save()

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		print("[NanoCoverage Runtime] Intercepted Window Close Request.")
		_do_save()

func _do_save() -> void:
	if _is_saved:
		print("[NanoCoverage Runtime] Data already saved. Skipping duplicate save.")
		return
		
	var api := _get_api()
	if api != null and is_instance_valid(api) and api.has_method("save_session"):
		print("[NanoCoverage Runtime] Flushing execution hits to disk...")
		api.save_session()
		_is_saved = true
		print("[NanoCoverage Runtime] Execution hits successfully saved.")
	else:
		printerr("[NanoCoverage Runtime] CRITICAL: Failed to flush hits. API Singleton missing!")
