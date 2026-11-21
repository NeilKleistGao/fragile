extends SceneTree
# Unit tests for FGameMode, its character/controller prefabs, and the
# world's game mode ownership and game_mode_prefab feature
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
	test_game_mode_ownership()
	# The scene tests wait for the first frame, so _ready has fired:
	# game_mode_prefab spawning and the scene-child behaviour can only be
	# observed with a live scene tree (see _process below).


func _process(_delta: float) -> bool:
	test_game_mode_prefab()
	test_prefabs()
	test_scene_child()

	if failures == 0:
		print("GAME_MODE_TEST_PASS")
	else:
		print("GAME_MODE_TEST_FAIL: " + str(failures))
	quit(0 if failures == 0 else 1)
	return true


## FGameMode is registered with ClassDB and instantiable by class name.
## Its script-visible surface is the two prefab properties; the link to
## the world is driven through FWorld::set_game_mode instead.
func test_registration() -> void:
	print("== registration ==")
	check(ClassDB.class_exists("FGameMode"), "ClassDB knows FGameMode")
	check(ClassDB.can_instantiate("FGameMode"), "FGameMode can be instantiated")
	check(ClassDB.is_parent_class("FGameMode", "FInfo"), "FGameMode derives from FInfo")
	var mode = ClassDB.instantiate("FGameMode")
	check(mode != null, "ClassDB.instantiate returns an instance")
	check(mode.get_class() == "FGameMode", "instance reports class FGameMode")
	check(mode.has_method("set_character_prefab") and mode.has_method("get_character_prefab"), "character_prefab is exposed to scripts")
	check(mode.has_method("set_controller_prefab") and mode.has_method("get_controller_prefab"), "controller_prefab is exposed to scripts")
	mode.free()


## The world owns its game mode: set_game_mode makes it a child node and
## frees the previous one; freeing the world frees its game mode too. A
## game mode is never stolen from another parent, while one authored
## below this world is linked in place.
func test_game_mode_ownership() -> void:
	print("== game mode ownership ==")
	var world = ClassDB.instantiate("FWorld")
	var mode = ClassDB.instantiate("FGameMode")
	check(world.get_game_mode() == null, "fresh world has no game mode")
	check(mode.get_parent() == null, "fresh game mode has no parent")

	world.set_game_mode(mode)
	check(world.get_game_mode() == mode, "set_game_mode stores the game mode")
	check(mode.get_parent() == world, "the game mode becomes a child of the world")

	world.set_game_mode(mode) # setting the same game mode again is a no-op
	check(world.get_game_mode() == mode and mode.get_parent() == world, "re-setting the same game mode is a no-op")

	var stranger = ClassDB.instantiate("FGameMode")
	var other = Node2D.new()
	other.add_child(stranger) # parented to somebody else, not the world
	print("  (expected errors follow)")
	world.set_game_mode(stranger) # never stolen from an existing parent
	check(world.get_game_mode() == mode, "a game mode parented elsewhere is refused")
	check(stranger.get_parent() == other, "the refused game mode keeps its parent")
	other.free()

	var mode2 = ClassDB.instantiate("FGameMode")
	world.set_game_mode(mode2)
	check(world.get_game_mode() == mode2, "set_game_mode replaces the game mode")
	check(mode2.get_parent() == world, "the new game mode is the world's child")
	check(mode.get_parent() == null, "the replaced game mode leaves the tree")
	check(mode.is_queued_for_deletion(), "the replaced game mode is freed by the world")

	world.set_game_mode(null)
	check(world.get_game_mode() == null, "null clears the game mode")
	check(mode2.get_parent() == null and mode2.is_queued_for_deletion(), "the cleared game mode is freed and unlinked")

	var mode3 = ClassDB.instantiate("FGameMode")
	world.set_game_mode(mode3)
	world.free() # the world frees its game mode child with it
	check(mode3.is_queued_for_deletion(), "the world frees its game mode")


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
	root.remove_child(world)
	world.free()
	check(spawned and mode.is_queued_for_deletion(), "the spawned game mode is freed with the world")


## The character and controller prefabs follow the same rules as the
## world's game_mode_prefab: the scene root must derive from the matching
## fragile class, and null clears the prefab.
func test_prefabs() -> void:
	print("== character/controller prefabs ==")
	var mode = ClassDB.instantiate("FGameMode")
	check(mode.get_character_prefab() == null, "fresh game mode has no character prefab")
	check(mode.get_controller_prefab() == null, "fresh game mode has no controller prefab")

	var character_scene = PackedScene.new()
	var character_template = ClassDB.instantiate("FCharacter")
	character_scene.pack(character_template)
	character_template.free()
	mode.set_character_prefab(character_scene)
	check(mode.get_character_prefab() == character_scene, "an FCharacter-rooted scene is accepted as the character prefab")

	var controller_scene = PackedScene.new()
	var controller_template = ClassDB.instantiate("FPlayerController")
	controller_scene.pack(controller_template)
	controller_template.free()
	mode.set_controller_prefab(controller_scene)
	check(mode.get_controller_prefab() == controller_scene, "an FController-rooted scene is accepted as the controller prefab")

	print("  (expected errors follow)")
	mode.set_character_prefab(controller_scene) # wrong root class
	check(mode.get_character_prefab() == character_scene, "a non-FCharacter-rooted scene is refused")
	mode.set_controller_prefab(character_scene) # wrong root class
	check(mode.get_controller_prefab() == controller_scene, "a non-FController-rooted scene is refused")

	mode.set_character_prefab(null)
	check(mode.get_character_prefab() == null, "null clears the character prefab")
	mode.set_controller_prefab(null)
	check(mode.get_controller_prefab() == null, "null clears the controller prefab")
	mode.free()


## The default test project's main scene ships without a game mode, and a
## game mode node authored in a scene stays unloaded by default - the
## world links it only when set explicitly, and then in place: the node
## stays where the scene put it and dies with its parent like any other
## node.
func test_scene_child() -> void:
	print("== scene child ==")
	var main: PackedScene = load("res://main.tscn")
	check(main != null, "main.tscn loads")
	var default_world = main.instantiate()
	root.add_child(default_world) # let _ready fire first
	check(default_world.get_node_or_null("FGameMode") == null, "main.tscn holds no FGameMode child")
	check(default_world.get_game_mode() == null, "the default scene ships without a game mode")
	root.remove_child(default_world)
	default_world.free()

	# The scene-child behaviour is covered with a scene packed here:
	# the default main scene carries no game mode to author against.
	var template = ClassDB.instantiate("FWorld")
	var mode_template = ClassDB.instantiate("FGameMode")
	mode_template.name = "FGameMode"
	template.add_child(mode_template)
	mode_template.owner = template
	var scene = PackedScene.new()
	check(scene.pack(template) == OK, "a scene with a game mode child can be packed")
	template.free()
	var world = scene.instantiate()
	root.add_child(world) # let _ready fire first
	check(world.get_game_mode() == null, "a scene child game mode is not loaded automatically")
	var mode = world.get_node_or_null("FGameMode")
	check(mode != null, "the packed scene holds an FGameMode child")
	var linked: bool = mode != null # a freed reference reads back as null
	if linked:
		world.set_game_mode(mode) # already below this world: linked in place
		check(world.get_game_mode() == mode, "explicit set_game_mode links the scene child")
		check(mode.get_parent() == world, "the linked game mode stays where the scene put it")
	root.remove_child(world)
	world.free()
	check(linked and mode.is_queued_for_deletion(), "the world frees its game mode child")
