extends GdUnitTestSessionHook
## NanoCoverage session hook for GdUnit4.
##
## Runs inside the GdUnit4 test runner process: instruments all scripts in
## memory before the tests start, then flushes hits and generates the LCOV
## report when the session shuts down.

const WORKSPACE_ID = "gdunit4"


func _init() -> void:
	super("NanoCoverage", "Instrument scripts in memory and generate LCOV reports for GdUnit4 tests.")


func startup(session: GdUnitTestSession) -> GdUnitResult:
	NanoCoverageLogger.info("GdUnit4 session starting: instrumenting scripts in memory...")
	session.send_message("NanoCoverage: Starting memory instrumentation...")

	var api = CoverageApi.new()
	api.clear_coverage_data({"workspace_id": WORKSPACE_ID})

	var bootstrapper = ProjectBootstrapper.new()
	bootstrapper.instrument_all_scripts()

	NanoCoverageLogger.info("Instrumentation complete. Yielding to GdUnit4 tests.")
	session.send_message("NanoCoverage: Instrumentation complete. Yielding to tests.")

	return GdUnitResult.success()


func shutdown(session: GdUnitTestSession) -> GdUnitResult:
	NanoCoverageLogger.info("GdUnit4 session finished. Generating report...")
	session.send_message("NanoCoverage: Tests complete. Generating report...")

	# 1. Force the runtime to dump hits to disk
	if not Engine.has_singleton("NanoCoverage"):
		NanoCoverageLogger.error("NanoCoverage singleton not found. Coverage data was not collected.")
		session.send_message("NanoCoverage: Singleton not found. Skipping report generation.")
		return GdUnitResult.success()

	var nc = Engine.get_singleton("NanoCoverage")

	var hits = nc.get_total_hit_count()
	if hits == 0:
		NanoCoverageLogger.warn("No execution hits collected. Check the ignore patterns in Project Settings (nano_coverage/general).")
	else:
		NanoCoverageLogger.info("Collected %d execution hits." % hits)
	session.send_message("NanoCoverage: Collected %d execution hits." % hits)

	nc.save_session(WORKSPACE_ID)
	nc.reset()

	# 2. Generate the report
	var api = CoverageApi.new()
	var result = api.generate_coverage_report({"workspace_id": WORKSPACE_ID})

	if result.has("status") and result.status == "ok":
		NanoCoverageLogger.info("Report generated at: %s" % result.report_path)
		session.send_message("NanoCoverage: LCOV report generated at: %s" % result.report_path)
	else:
		var err = result.get("error", "Unknown")
		NanoCoverageLogger.error("Report generation failed: %s" % err)
		session.send_message("NanoCoverage: LCOV report generation failed: %s" % err)

	return GdUnitResult.success()
