extends SceneTree
# Unit tests for FGameMode and the game_mode_prefab feature
# (src/fragile/framework/game_mode.h, src/fragile/framework/world.h).
#
# Run from the repository root:
#   <godot> --headless --path project -s res://tests/test_game_mode.gd
#
# Exits 0 and prints GAME_MODE_TEST_PASS when every check passes,
# otherwise exits 1.

var failures: int = 0


func check(cond: bool, what: String) -> void:
	if cond:
		print("  ok: " + what)
	else:
		failures += 1
		print("  FAIL: " + what)


func _init() -> void:
	test_registration()
	test_world_link()
	# The scene tests wait for the first frame, so _ready has fired:
	# game_mode_prefab spawning and the main.tscn child-node behaviour
	# can only be observed with a live scene tree (see _process below).


func _process(_delta: float) -> bool:
	test_game_mode_prefab()
	test_scene_child()

	if failures == 0:
		print("GAME_MODE_TEST_PASS")
	else:
		print("GAME_MODE_TEST_FAIL: " + str(failures))
	quit(0 if failures == 0 else 1)
	return true


## FGameMode is registered with ClassDB and instantiable by class name.
## The script-visible surface is get_world(); set_world() stays internal
## and is driven through FWorld::set_game_mode.
func test_registration() -> void:
	print("== registration ==")
	check(ClassDB.class_exists("FGameMode"), "ClassDB knows FGameMode")
	check(ClassDB.can_instantiate("FGameMode"), "FGameMode can be instantiated")
	check(ClassDB.is_parent_class("FGameMode", "FInfo"), "FGameMode derives from FInfo")
	var mode = ClassDB.instantiate("FGameMode")
	check(mode != null, "ClassDB.instantiate returns an instance")
	check(mode.get_class() == "FGameMode", "instance reports class FGameMode")
	check(mode.has_method("get_world"), "get_world is exposed to scripts")
	check(not mode.has_method("set_world"), "set_world stays internal")
	mode.free()


## The world owns its game mode: set_game_mode adds it as a child node
## and frees the previous one; freeing the world frees its game mode too.
func test_world_link() -> void:
	print("== world link ==")
	var world = ClassDB.instantiate("FWorld")
	var mode = ClassDB.instantiate("FGameMode")
	check(world.get_game_mode() == null, "fresh world has no game mode")
	check(mode.get_world() == null, "fresh game mode is unlinked")

	world.set_game_mode(mode)
	check(world.get_game_mode() == mode, "set_game_mode stores the game mode")
	check(mode.get_world() == world, "the game mode points back at the world")
	check(mode.get_parent() == world, "the game mode becomes a child of the world")

	world.set_game_mode(mode) # setting the same game mode again is a no-op
	check(world.get_game_mode() == mode and mode.get_parent() == world, "re-setting the same game mode is a no-op")

	var parented = ClassDB.instantiate("FGameMode")
	world.add_child(parented) # a plain child, not the game mode
	print("  (expected errors follow)")
	world.set_game_mode(parented) # it has a parent: never stolen
	check(world.get_game_mode() == mode, "a game mode that already has a parent is refused")
	check(parented.get_parent() == world, "the refused game mode keeps its parent")
	world.remove_child(parented)
	parented.free()

	var mode2 = ClassDB.instantiate("FGameMode")
	world.set_game_mode(mode2)
	check(world.get_game_mode() == mode2, "set_game_mode replaces the game mode")
	check(mode2.get_parent() == world and mode2.get_world() == world, "the new game mode is the world's child")
	check(mode.get_parent() == null and mode.get_world() == null, "the replaced game mode leaves the tree unlinked")
	check(mode.is_queued_for_deletion(), "the replaced game mode is freed by the world")

	world.set_game_mode(null)
	check(world.get_game_mode() == null, "null clears the game mode")
	check(mode2.get_world() == null and mode2.is_queued_for_deletion(), "the cleared game mode is freed and unlinked")

	var mode3 = ClassDB.instantiate("FGameMode")
	world.set_game_mode(mode3)
	mode3.free() # freeing the game mode clears the world's link
	check(world.get_game_mode() == null, "freeing the game mode clears the world's link")

	var mode4 = ClassDB.instantiate("FGameMode")
	world.set_game_mode(mode4)
	world.free() # the world frees its game mode child with it
	check(mode4.is_queued_for_deletion(), "the world frees its game mode")


## game_mode_prefab: the root must derive from FGameMode, the prefab can
## be cleared, and ready spawns the game mode from it.
func test_game_mode_prefab() -> void:
	print("== game_mode_prefab ==")
	var world = ClassDB.instantiate("FWorld")
	check(world.game_mode_prefab == null, "fresh world has no game mode prefab")

	var good = PackedScene.new()
	var mode_template = ClassDB.instantiate("FGameMode")
	good.pack(mode_template)
	mode_template.free()
	world.set_game_mode_prefab(good)
	check(world.get_game_mode_prefab() == good, "an FGameMode-rooted scene is accepted")

	print("  (expected errors follow)")
	var bad = PackedScene.new()
	var actor_template = ClassDB.instantiate("FActor")
	bad.pack(actor_template)
	actor_template.free()
	world.set_game_mode_prefab(bad)
	check(world.get_game_mode_prefab() == good, "a non-FGameMode-rooted scene is refused")

	world.set_game_mode_prefab(null)
	check(world.get_game_mode_prefab() == null, "null clears the prefab")
	world.set_game_mode_prefab(good)

	root.add_child(world) # entering the tree runs _ready
	var mode = world.get_game_mode()
	var spawned: bool = mode != null # a freed reference reads back as null
	check(spawned, "ready spawns the game mode from the prefab")
	if spawned:
		check(mode.get_class() == "FGameMode", "the spawned game mode has the prefab's class")
		check(mode.get_parent() == world, "the spawned game mode is the world's child")
		check(mode.get_world() == world, "the spawned game mode links back to the world")
	root.remove_child(world)
	world.free()
	check(spawned and mode.is_queued_for_deletion(), "the spawned game mode is freed with the world")


## A game mode node authored in a scene (main.tscn) stays unloaded by
## default - the world uses it only when set explicitly. The node stays
## where the scene put it and dies with its parent like any other node.
func test_scene_child() -> void:
	print("== scene child ==")
	var scene: PackedScene = load("res://main.tscn")
	check(scene != null, "main.tscn loads")
	var world = scene.instantiate()
	root.add_child(world) # let _ready fire first
	check(world.get_game_mode() == null, "a scene child game mode is not loaded automatically")
	var mode = world.get_node_or_null("FGameMode")
	check(mode != null, "main.tscn holds an FGameMode child")
	var linked: bool = mode != null # a freed reference reads back as null
	if linked:
		print("  (expected errors follow)")
		world.set_game_mode(mode) # still parented: refused, never stolen
		check(world.get_game_mode() == null, "set_game_mode refuses the still-parented scene child")
		world.remove_child(mode) # the caller detaches; set_game_mode never reparents
		world.set_game_mode(mode)
		check(world.get_game_mode() == mode, "explicit set_game_mode links the detached scene child")
		check(mode.get_parent() == world, "the linked game mode is the world's child again")
		check(mode.get_world() == world, "the linked game mode points back at the world")
	root.remove_child(world)
	world.free()
	check(linked and mode.is_queued_for_deletion(), "the world frees its game mode child")
