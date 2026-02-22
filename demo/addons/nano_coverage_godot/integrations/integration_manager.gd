@tool
extends RefCounted
## Manages optional third-party integrations for NanoCoverage.
##
## This script monitors project settings to enable or disable integrations
## (like GdUnit4) dynamically. It ensures integrations are only active
## when the corresponding plugins are installed and the user has enabled them.

const SETTING_PATH = "nano_coverage/integrations/gdunit4/active"
const GdUnitIntegration = preload("res://addons/nano_coverage_godot/integrations/gdunit4.gd")

var _nano_api
var _active_integration

func _init(nano_api):
	_nano_api = nano_api
	_setup_project_settings()
	_update_integration_state()
	ProjectSettings.settings_changed.connect(_on_settings_changed)

func clean_up():
	if _active_integration:
		_active_integration.disable()
		_active_integration = null

func _setup_project_settings():
	if not ProjectSettings.has_setting(SETTING_PATH):
		ProjectSettings.set_setting(SETTING_PATH, false)
	
	ProjectSettings.add_property_info({
		"name": SETTING_PATH,
		"type": TYPE_BOOL,
		"hint": PROPERTY_HINT_NONE,
		"hint_string": "Enable integration with GdUnit4"
	})
	
	# This forces the setting to appear in the standard view (not just Advanced)
	ProjectSettings.set_as_basic(SETTING_PATH, true)

func _on_settings_changed():
	_update_integration_state()

func _update_integration_state():
	var should_be_active = ProjectSettings.get_setting(SETTING_PATH)
	
	# If user wants it active but the plugin is missing, force disable it.
	if should_be_active and not GdUnitIntegration.is_available():
		printerr("NanoCoverage: GdUnit4 not found! Disabling integration.")
		ProjectSettings.set_setting(SETTING_PATH, false)
		should_be_active = false
	
	if should_be_active and _active_integration == null:
		_active_integration = GdUnitIntegration.new(_nano_api)
		_active_integration.enable()
		print("NanoCoverage: GdUnit4 Integration Active")
		return

	if not should_be_active and _active_integration != null:
		_active_integration.disable()
		_active_integration = null
		print("NanoCoverage: GdUnit4 Integration Deactivated")
		return