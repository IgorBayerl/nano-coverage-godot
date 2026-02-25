class_name NanoCoverageLogger

static func info(msg: String) -> void:
	print("[NanoCoverage] " + msg)

static func warn(msg: String) -> void:
	print("[NanoCoverage] [WARN] " + msg)

static func error(msg: String) -> void:
	printerr("[NanoCoverage] [ERROR] " + msg)
