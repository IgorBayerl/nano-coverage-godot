extends Node

func _ready():
	print("Starting coverage test...")
	await get_tree().create_timer(1.0).timeout
	
	# Enable coverage collection
	EngineDebugger.profiler_enable("coverage", true)
	
	# Run some code to be covered
	_test_function()
	
	# Disable coverage collection (triggers save)
	EngineDebugger.profiler_enable("coverage", false)
	
	print("Coverage test complete")
	get_tree().quit()

func _test_function():
	var a = 10
	if a > 5:
		print("a is greater than 5")
	else:
		print("a is not greater than 5")
