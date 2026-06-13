@tool
extends RefCounted
## Manages optional third-party integrations for NanoCoverage.
##
## This script monitors project settings to enable or disable integrations
## (like GdUnit4) dynamically. It ensures integrations are only active
## when the corresponding plugins are installed and the user has enabled them.

const SETTING_GDUNIT4 = "nano_coverage/integrations/gdunit4"
## Pre-1.0 versions stored the toggle under a nested "active" key.
const LEGACY_SETTING_GDUNIT4 = "nano_coverage/integrations/gdunit4/active"

const GdUnitIntegration = preload("res://addons/nano_coverage_godot/integrations/gdunit4.gd")

var _nano_api
var _active_integration


func _init(nano_api):
	_nano_api = nano_api
	_setup_project_settings()
	_update_integration_state()
	ProjectSettings.settings_changed.connect(_on_settings_changed)


func clean_up():
	# Only drop the in-memory reference. The hook registration in project
	# settings is persistent config: removing it here would unregister the
	# hook on every editor shutdown and break CLI/CI test runs. It is only
	# removed when the user turns the integration setting off.
	_active_integration = null


func _setup_project_settings():
	_migrate_legacy_setting()

	if not ProjectSettings.has_setting(SETTING_GDUNIT4):
		ProjectSettings.set_setting(SETTING_GDUNIT4, false)

	ProjectSettings.set_initial_value(SETTING_GDUNIT4, false)
	ProjectSettings.add_property_info({
		"name": SETTING_GDUNIT4,
		"type": TYPE_BOOL,
		"hint": PROPERTY_HINT_NONE,
		"hint_string": "Enable integration with GdUnit4"
	})

	# This forces the setting to appear in the standard view (not just Advanced)
	ProjectSettings.set_as_basic(SETTING_GDUNIT4, true)


func _migrate_legacy_setting():
	if not ProjectSettings.has_setting(LEGACY_SETTING_GDUNIT4):
		return

	var legacy_value: bool = ProjectSettings.get_setting(LEGACY_SETTING_GDUNIT4, false)
	ProjectSettings.set_setting(SETTING_GDUNIT4, legacy_value)
	# Assigning null removes the setting entirely.
	ProjectSettings.set_setting(LEGACY_SETTING_GDUNIT4, null)
	ProjectSettings.save()
	NanoCoverageLogger.info("Migrated setting '%s' -> '%s'." % [LEGACY_SETTING_GDUNIT4, SETTING_GDUNIT4])


func _on_settings_changed():
	_update_integration_state()


func _update_integration_state():
	var should_be_active: bool = ProjectSettings.get_setting(SETTING_GDUNIT4, false)

	# If user wants it active but the plugin is missing, force disable it.
	if should_be_active and not GdUnitIntegration.is_available():
		NanoCoverageLogger.error("GdUnit4 not found! Disabling integration.")
		ProjectSettings.set_setting(SETTING_GDUNIT4, false)
		should_be_active = false

	if should_be_active and _active_integration == null:
		_active_integration = GdUnitIntegration.new(_nano_api)
		_active_integration.enable()
		return

	if not should_be_active and _active_integration != null:
		_active_integration.disable()
		_active_integration = null
		return
