#ifndef __FRAGILE_FRAMEWORK_LEVEL_H__
#define __FRAGILE_FRAMEWORK_LEVEL_H__

#include "fragile/global.h"
#include "spawn_point.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/array.hpp>

namespace fragile::framework {

class FActor;
class FCharacter;
class FController;
class FGameMode;

/**
 * @brief A container of actors.
 *
 * Mirrors Unreal Engine's ULevel: it holds the actors that make up one part
 * of the game world. Levels are created and opened at runtime through
 * FWorld::open_level.
 *
 * A level also spawns and owns its game mode: at ready it instantiates
 * override_game_mode when set, or else the current world's game_mode_prefab,
 * then spawns the player character and controller described by that game
 * mode. The game mode and the player nodes become children of this level
 * without being registered as actors.
 */
class FLevel : public Node2D {
	GDCLASS(FLevel, Node2D)

private:
	/**
	 * @brief Actors registered through add_actor(), in insertion order.
	 *
	 * Holds raw FActor pointers; an entry is not removed when its actor is
	 * freed.
	 */
	Array actors;

	/**
	 * @brief Scene whose root derives from FGameMode, overriding the world default.
	 *
	 * Assigned through the @c override_game_mode property and validated by
	 * set_override_game_mode(). When unset, _ready() falls back to the
	 * current world's FWorld::get_game_mode_prefab().
	 */
	Ref<PackedScene> override_game_mode;

	/**
	 * @brief Game mode spawned for this level; @c nullptr when none is set.
	 *
	 * Created in _ready() by spawn_game_mode() and added as a child node of
	 * this level. Freed together with the level.
	 */
	FGameMode* game_mode{ nullptr };

	/**
	 * @brief Character spawned for the local player; @c nullptr when none.
	 *
	 * Created in _ready() from the game mode's character prefab.
	 */
	FCharacter* player{ nullptr };

	/**
	 * @brief Controller spawned for the local player; @c nullptr when none.
	 *
	 * Created in _ready() from the game mode's controller prefab; takes
	 * control of @c player.
	 */
	FController* controller{ nullptr };

	/**
	 * @brief Spawns this level's game mode and adds it as a child node.
	 *
	 * Uses override_game_mode when it is set, otherwise the current world's
	 * game_mode_prefab. Prints an error and leaves game_mode @c nullptr when
	 * no usable prefab is available.
	 */
	void spawn_game_mode();

	/**
	 * @brief Spawns the player character and controller and binds them.
	 *
	 * Both prefabs are read from game_mode, whose state is not checked here:
	 * the caller must ensure game_mode is not @c nullptr. A prefab that is
	 * unset or cannot be instantiated simply spawns nothing for that side,
	 * without printing an error.
	 *
	 * Each spawned node is added as a child of this level; the controller
	 * takes control of the character once both exist.
	 */
	void spawn_player_with_controller();

	FSpawnPoint* load_spawn_point() const;

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Registers @p actor and adds it as a child node.
	 *
	 * Does nothing when @p actor is @c nullptr. Nothing is added when an actor
	 * with the same name is already registered either; an error is printed in
	 * that case.
	 *
	 * @param actor Actor to attach to this level; @c nullptr is ignored.
	 */
	void add_actor(FActor* actor);

	/**
	 * @brief Sets the scene used as this level's game mode.
	 *
	 * The scene's root node must derive from FGameMode (checked with
	 * fragile::utils::is_scene_root_derived_from); anything else is refused
	 * with an error and the previous prefab is kept. Passing @c nullptr
	 * removes the override so the world's game_mode_prefab is used again.
	 *
	 * @param prefab Scene whose root is an FGameMode subclass, or @c nullptr.
	 */
	void set_override_game_mode(const Ref<PackedScene>& prefab);

	/**
	 * @brief Returns the scene used as this level's game mode override.
	 *
	 * @return The assigned PackedScene, or @c nullptr when no override is set.
	 */
	[[nodiscard]] Ref<PackedScene> get_override_game_mode() const;

	void _ready() override;

	// Clears the pointers to the spawned nodes when the level is torn down.
	void _notification(int p_what);

	/**
	 * @brief Returns the character spawned for the local player.
	 *
	 * @return The player character, or @c nullptr when none was spawned.
	 */
	[[nodiscard]] FCharacter* get_player_character() const;

	/**
	 * @brief Returns the controller spawned for the local player.
	 *
	 * @return The player controller, or @c nullptr when none was spawned.
	 */
	[[nodiscard]] FController* get_player_controller() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_LEVEL_H__
