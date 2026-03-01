extends SceneTree
func _init():
    var runner = NanoCoverageTestRunner.new()
    var result = runner.run_all_tests()
    quit(result)
