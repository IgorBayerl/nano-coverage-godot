extends Node
## The global NanoCoverage runtime singleton injected into the project.

const GDUNIT_SESSION_CLOSE := 9

var _api: Object = null
var _is_saved := false

func _ready() -> void:
	NanoCoverageLogger.info("Injected into SceneTree. Monitoring for hits...")
	if not Engine.has_meta("GdUnitSignals"):
		return
		
	NanoCoverageLogger.info("GdUnit4 detected. Hooking into test session signals.")
	var signals: Object = Engine.get_meta("GdUnitSignals")
	if signals.has_signal("gdunit_event") and not signals.gdunit_event.is_connected(_on_gdunit_event):
		signals.gdunit_event.connect(_on_gdunit_event)

func _exit_tree() -> void:
	NanoCoverageLogger.info("Exiting tree...")
	if Engine.has_meta("GdUnitSignals"):
		var signals: Object = Engine.get_meta("GdUnitSignals")
		if signals.has_signal("gdunit_event") and signals.gdunit_event.is_connected(_on_gdunit_event):
			signals.gdunit_event.disconnect(_on_gdunit_event)
				
	_do_save()


func _on_gdunit_event(event: Object) -> void:
	if event.has_method("type") and event.type() == GDUNIT_SESSION_CLOSE:
		NanoCoverageLogger.info("Intercepted GdUnit4 Session Close event.")
		_do_save()

func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		NanoCoverageLogger.info("Intercepted Window Close Request.")
		_do_save()

func _do_save() -> void:
	if _is_saved:
		NanoCoverageLogger.info("Data already saved. Skipping duplicate save.")
		return
		
	var api = Engine.get_singleton("NanoCoverage")
	if api != null and is_instance_valid(api) and api.has_method("save_session"):
		NanoCoverageLogger.info("Flushing execution hits to disk...")
		api.save_session()
		_is_saved = true
		NanoCoverageLogger.info("Execution hits successfully saved.")
	else:
		NanoCoverageLogger.error("CRITICAL: Failed to flush hits. API Singleton missing!")
