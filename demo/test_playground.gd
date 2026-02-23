extends SceneTree

func _init():
    var api = CoverageApi.new()
    var path = "res://dummy.gd"
    var script = load(path)
    
    if not script is GDScript:
        print("Not a GDScript")
        return

    # 1. Instrument it
    var res = api.instrument_script(script.source_code, path)
    if res.has("success") and res["success"]:
        script.source_code = res["code"]
        script.reload(true)
        print("Script patched successfully!")
    else:
        print("Failed to patch: ", res.get("error"))
        return
        
    # 2. Invoke it
    var obj = script.new()
    var r = obj.foo()
    print("Dummy returned: ", r)
    
    # 3. Check hits
    var NanoCoverage = Engine.get_singleton("NanoCoverage")
    if NanoCoverage:
        var snapshot = NanoCoverage.get_snapshot()
        print("Snapshot: ", snapshot)
        
    api.save_static_metadata()
    print("Playground test complete")
