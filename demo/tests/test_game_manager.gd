class_name TestGameManager extends GdUnitTestSuite

func test_reset_game():
	var gm = GameManager.new()
	gm.add_score(100)
	gm.reset_game()
	assert_int(gm.current_score).is_equal(0)
	assert_bool(gm.level_data["level_1"]["unlocked"]).is_true()
	gm.free()

func test_add_score():
	var gm = GameManager.new()
	gm.reset_game()
	gm.add_score(50)
	assert_int(gm.current_score).is_equal(50)
	
	# Adding negative points should not change score
	gm.add_score(-10)
	assert_int(gm.current_score).is_equal(50)
	gm.free()

func test_enemy_management():
	var gm = GameManager.new()
	gm.reset_game()
	
	var dummy_enemy = Node.new()
	var dummy_enemy2 = Node.new()
	
	gm.register_enemy(dummy_enemy)
	gm.register_enemy(dummy_enemy2)
	assert_array(gm.active_enemies).contains_exactly([dummy_enemy, dummy_enemy2])
	
	gm.enemy_defeated(dummy_enemy, 100)
	assert_int(gm.current_score).is_equal(100)
	assert_array(gm.active_enemies).contains_exactly([dummy_enemy2])
	
	# Defeating last enemy triggers level complete logic but we just test state
	gm.enemy_defeated(dummy_enemy2, 200)
	assert_int(gm.current_score).is_equal(300)
	assert_array(gm.active_enemies).is_empty()
	
	dummy_enemy.free()
	dummy_enemy2.free()
	gm.free()
