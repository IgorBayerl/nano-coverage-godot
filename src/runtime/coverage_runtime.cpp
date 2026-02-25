#include "coverage_runtime.h"
#include <godot_cpp/classes/engine.hpp>
#include "../utils/logger.h"
#include "coverage_monitor.h"

namespace godot {

void CoverageRuntime::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_gdunit_event", "event"), &CoverageRuntime::_on_gdunit_event);
}

void CoverageRuntime::_ready() {
    Logger::info("CoverageRuntime active. Monitoring for application exit...");

    // Hook into GdUnit4 if present
    Engine* engine = Engine::get_singleton();
    if (engine->has_meta("GdUnitSignals")) {
        Logger::info("GdUnit4 detected. Hooking into test session signals.");
        Object* signals = Object::cast_to<Object>(engine->get_meta("GdUnitSignals"));
        if (signals && signals->has_signal("gdunit_event")) {
            signals->connect("gdunit_event", Callable(this, "_on_gdunit_event"));
        }
    }
}

void CoverageRuntime::_exit_tree() {
    flush_data();
}

void CoverageRuntime::_notification(int p_what) {
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        Logger::info("Intercepted Window Close Request.");
        flush_data();
    }
}

void CoverageRuntime::_on_gdunit_event(const Variant& event) {
    // GDUNIT_SESSION_CLOSE enum is mapped to 9 natively in GdUnit
    Object* ev_obj = Object::cast_to<Object>(event);
    if (!ev_obj || !ev_obj->has_method("type")) return;

    int type = ev_obj->call("type");
    if (type == 9) { // Session Closed
        Logger::info("Intercepted GdUnit4 Session Close event.");
        flush_data();
    }
}

void CoverageRuntime::flush_data() {
    if (is_saved) return;

    Object* singleton = Engine::get_singleton()->get_singleton("NanoCoverage");
    NanoCoverage* api = Object::cast_to<NanoCoverage>(singleton);
    
    if (api) {
        Logger::info("Flushing execution hits to disk...");
        api->save_session();
        is_saved = true;
        Logger::info("Execution hits successfully saved.");
    } else {
        Logger::error("CRITICAL: Failed to flush hits. API Singleton missing!");
    }
}

} // namespace godot
