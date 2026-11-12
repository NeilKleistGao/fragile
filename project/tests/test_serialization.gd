extends SceneTree
# Serialization unit tests for the fragile framework.
#
# Run from the repository root:
#   <godot> --headless --path project -s res://tests/test_serialization.gd
#
# Exits 0 and prints SERIALIZATION_TEST_PASS when every check passes,
# otherwise exits 1. The failure-path checks trigger errors on purpose;
# the "expected errors" notes mark where they belong.

var failures: int = 0


func check(cond: bool, what: String) -> void:
	if cond:
		print("  ok: " + what)
	else:
		failures += 1
		print("  FAIL: " + what)


func _init() -> void:
	test_scene_round_trip()
	test_f_actor()
	test_f_info()
	test_f_level()
	test_f_world()

	if failures == 0:
		print("SERIALIZATION_TEST_PASS")
	else:
		print("SERIALIZATION_TEST_FAIL: " + str(failures))
	quit(0 if failures == 0 else 1)


## Serialize and deserialize a small scene holding one instance of every
## registered class (FWorld / FLevel / FActor / FInfo), then compare the two.
## The framework persists the world -> level -> actors chain; FInfo carries no
## state and is serialized on its own.
func test_scene_round_trip() -> void:
	print("== scene round-trip ==")
	var world = ClassDB.instantiate("FWorld")
	world.name = "world"
	world.open_level("FLevel")
	var level = world.get_child(0)
	var hero = level.spawn_actor("FActor", "hero")
	hero.position = Vector2(3, 4)
	var chest = level.spawn_actor("FActor", "chest")
	chest.position = Vector2(-8, 12.5)
	var info = ClassDB.instantiate("FInfo")
	info.name = "info"
	world.add_child(info)

	var world_data: Dictionary = {}
	check(world.serialize(world_data) == OK, "world serializes")
	var info_data: Dictionary = {}
	check(info.serialize(info_data) == OK, "info serializes")

	var world2 = ClassDB.instantiate("FWorld")
	world2.name = "world"
	check(world2.deserialize(world_data) == OK, "world deserializes")
	var info2 = ClassDB.instantiate("FInfo")
	info2.name = "info"
	check(info2.deserialize(info_data) == OK, "info deserializes")

	check(world2.get_child_count() == 1, "restored world holds exactly one level")
	var level2 = world2.get_child(0)
	check(level2.get_class() == level.get_class(), "restored level keeps class " + level2.get_class())
	compare_actor_sets(level, level2)

	world.free() # releases info with it
	world2.free()
	info2.free()


## Compares the actor children of two levels: names, classes and positions.
func compare_actor_sets(a, b) -> void:
	check(b.get_child_count() == a.get_child_count(), "restored level keeps actor count (" + str(a.get_child_count()) + ")")
	for actor in a.get_children():
		var twin = b.get_node_or_null(str(actor.name))
		if twin == null:
			check(false, "restored actor missing: " + str(actor.name))
			continue
		check(twin.get_class() == actor.get_class(), "actor " + str(actor.name) + " keeps class " + actor.get_class())
		check(twin.position == actor.position, "actor " + str(actor.name) + " keeps position " + str(actor.position))


## FActor: position round-trip, key collisions and missing keys.
func test_f_actor() -> void:
	print("== FActor ==")
	var actor = ClassDB.instantiate("FActor")
	actor.position = Vector2(10, -20)

	var data: Dictionary = {}
	check(actor.serialize(data) == OK, "serialize succeeds")
	# Dictionary is passed by value but shared: the caller sees the writes.
	check(data.size() == 2, "serialize writes two keys through the caller's dictionary")

	var actor2 = ClassDB.instantiate("FActor")
	check(actor2.deserialize(data) == OK, "deserialize succeeds")
	check(actor2.position == actor.position, "position round-trips " + str(actor.position))

	print("  (expected errors follow)")
	check(actor.serialize({"local_position_x": 1}) == ERR_INVALID_DATA, "serialize refuses to overwrite keys")
	check(actor2.deserialize({"local_position_x": 1}) == ERR_INVALID_DATA, "deserialize rejects missing keys")

	actor.free()
	actor2.free()


## FInfo: state-less, serialization always succeeds and stores nothing.
func test_f_info() -> void:
	print("== FInfo ==")
	var info = ClassDB.instantiate("FInfo")
	var data: Dictionary = {}
	check(info.serialize(data) == OK, "serialize always succeeds")
	check(data.is_empty(), "serialize stores nothing")
	check(info.deserialize(data) == OK, "deserialize always succeeds")
	info.free()


## FLevel: spawning, round-trip and the structural failure paths.
func test_f_level() -> void:
	print("== FLevel ==")
	var level = ClassDB.instantiate("FLevel")
	var hero = level.spawn_actor("FActor", "hero")
	check(hero != null and level.get_child_count() == 1, "spawn_actor attaches the actor")
	hero.position = Vector2(1, 2)

	print("  (expected errors follow)")
	var dup = level.spawn_actor("FActor", "hero")
	check(level.get_child_count() == 1, "duplicate actor name is refused")
	if dup != null and dup.get_parent() == null:
		dup.free()
	check(level.spawn_actor("NoSuchClass", "x") == null, "unknown actor type returns null")

	var data: Dictionary = {}
	check(level.serialize(data) == OK, "serialize succeeds")
	var level2 = ClassDB.instantiate("FLevel")
	check(level2.deserialize(data) == OK, "deserialize succeeds")
	compare_actor_sets(level, level2)

	# Hand-written data: String keys and String type values must work too.
	var hand_made: Dictionary = {"actors": {"hero2": {"type": "FActor", "local_position_x": 5.0, "local_position_y": 6.0}}}
	var level3 = ClassDB.instantiate("FLevel")
	check(level3.deserialize(hand_made) == OK, "hand-made data with String keys deserializes")
	var hero2 = level3.get_node_or_null("hero2")
	check(hero2 != null and hero2.position == Vector2(5, 6), "hand-made actor is restored")

	# Structural failure paths (all expect ERR_INVALID_DATA).
	check(level.serialize({"actors": {}}) == ERR_INVALID_DATA, "serialize refuses to overwrite")
	check(level.deserialize({}) == ERR_INVALID_DATA, "rejects missing actors key")
	check(level.deserialize({"actors": 1}) == ERR_INVALID_DATA, "rejects non-dictionary actors")
	check(level.deserialize({"actors": {"a": 1}}) == ERR_INVALID_DATA, "rejects non-dictionary entry")
	check(level.deserialize({"actors": {1: {}}}) == ERR_INVALID_DATA, "rejects non-string actor key")
	check(level.deserialize({"actors": {"a": {}}}) == ERR_INVALID_DATA, "rejects entry without type")
	check(level.deserialize({"actors": {&"a": {"type": 1}}}) == ERR_INVALID_DATA, "rejects non-string type")
	check(level.deserialize({"actors": {&"a": {"type": "NoSuchClass"}}}) == ERR_INVALID_DATA, "rejects unknown actor type")
	check(level.deserialize({"actors": {&"a": {"type": "Node"}}}) == ERR_INVALID_DATA, "rejects non-FActor type")

	level.free()
	level2.free()
	level3.free()


## FWorld: level round-trip and the structural failure paths.
func test_f_world() -> void:
	print("== FWorld ==")
	var world = ClassDB.instantiate("FWorld")
	world.open_level("FLevel")
	var level = world.get_child(0)
	var hero = level.spawn_actor("FActor", "hero")
	hero.position = Vector2(9, 9)

	var data: Dictionary = {}
	check(world.serialize(data) == OK, "serialize succeeds")
	check(data.has("level") and data["level"] is Dictionary, "serialize stores the level dictionary")

	var world2 = ClassDB.instantiate("FWorld")
	check(world2.deserialize(data) == OK, "deserialize succeeds")
	check(world2.get_child_count() == 1, "deserialize opens the level")
	compare_actor_sets(level, world2.get_child(0))

	print("  (expected errors follow)")
	check(world.serialize({"level": {}}) == ERR_INVALID_DATA, "serialize refuses to overwrite")
	check(world.deserialize({"level": 1}) == ERR_INVALID_DATA, "rejects non-dictionary level")
	check(world.deserialize({"level": {}}) == ERR_INVALID_DATA, "rejects level without type")
	check(world.deserialize({"level": {"type": 1}}) == ERR_INVALID_DATA, "rejects non-string type")
	check(world.deserialize({"level": {"type": "NoSuchClass"}}) == ERR_INVALID_DATA, "rejects unknown level type")
	check(world.deserialize({"level": {"type": "Node"}}) == ERR_INVALID_DATA, "rejects non-FLevel type")
	check(world.get_child_count() == 1, "failed deserialize keeps the previous level")

	# A dictionary without the level key is a no-op.
	check(world2.deserialize({}) == OK, "deserialize without level key succeeds")
	check(world2.get_child_count() == 1, "no-op keeps the open level")

	world.free()
	world2.free()
