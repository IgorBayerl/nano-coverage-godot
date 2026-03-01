class_name TestPlayer extends GdUnitTestSuite

func test_initialization():
	var player = DemoPlayer.new()
	assert_int(player.current_health).is_equal(100)
	assert_int(player.max_health).is_equal(100)
	assert_int(player.current_state).is_equal(DemoPlayer.State.IDLE)
	player.free()

func test_take_damage_and_heal():
	var player = DemoPlayer.new()
	
	# Taking damage
	player.take_damage(20)
	assert_int(player.current_health).is_equal(80)
	
	# Healing
	player.heal(10)
	assert_int(player.current_health).is_equal(90)
	
	# Healing beyond max health
	player.heal(50)
	assert_int(player.current_health).is_equal(100)
	
	player.free()

func test_death():
	var player = DemoPlayer.new()
	player.take_damage(100)
	
	assert_int(player.current_health).is_equal(0)
	assert_int(player.current_state).is_equal(DemoPlayer.State.DEAD)
	
	# Check that taking damage while dead does nothing
	player.take_damage(10)
	assert_int(player.current_health).is_equal(0)
	
	# Check that healing while dead does nothing
	player.heal(50)
	assert_int(player.current_health).is_equal(0)
	
	player.free()
