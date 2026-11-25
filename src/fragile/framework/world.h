#ifndef __FRAGILE_FRAMEWORK_WORLD_H__
#define __FRAGILE_FRAMEWORK_WORLD_H__

#include "fragile/global.h"
#include "fragile/framework/game_mode.h"
#include "fragile/framework/level.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace fragile::framework {

/**
 * @brief The primary container for everything that comprises a level or scene.
 *
 * Mirrors Unreal Engine's UWorld: it owns the currently open level with its
 * actors and switches levels at runtime (open_level). It also owns its game
 * mode: set_game_mode makes the game mode a child node of this world and
 * frees the previous one, and game_mode_prefab spawns it at ready.
 */
class FWorld : public Node2D {
	GDCLASS(FWorld, Node2D)

private:
	FLevel* current_level{ nullptr };

	/**
	 * @brief Game mode owned by this world; @c nullptr when none is set.
	 *
	 * A child node of this world, created by set_game_mode() or spawned
	 * from game_mode_prefab at ready. Freed together with the world.
	 */
	FGameMode* game_mode{ nullptr };

	/**
	 * @brief Scene whose root derives from FGameMode, spawned at ready.
	 *
	 * Assigned through the @c game_mode_prefab property and validated by
	 * set_game_mode_prefab(). Held as @c Ref so the scene stays
	 * alive for as long as this world references it - a bare
	 * PackedScene* owns no reference and dangles as soon as the
	 * last Ref elsewhere drops the resource (RefCounted members must be
	 * Ref<T>, never a raw pointer).
	 */
	Ref<PackedScene> game_mode_prefab;

	Ref<PackedScene> default_level_prefab;

	static FWorld* instance;

protected:
	static void _bind_methods();

	void _notification(int p_what);

public:
	/**
	 * @brief Opens the level of type @p level_name as the current level.
	 *
	 * The class is instantiated through ClassDB and added as a child. A
	 * previously opened level is removed and freed. An error is printed when
	 * the level cannot be created.
	 *
	 * @param level_name Class name of an FLevel subclass registered with ClassDB.
	 */
	void open_level(const StringName& level_name);
	// TODO: open a level by packed scene?

	/**
	 * @brief Makes @p game_mode this world's game mode and child node.
	 *
	 * The world owns its game mode: @p game_mode becomes a child node of
	 * this world and a previously set game mode is removed from the tree
	 * and freed. Passing @c nullptr clears the game mode the same way.
	 * A game mode authored below this world in a scene is linked in place
	 * and stays where the scene put it; a game mode that already has
	 * another parent is refused with an error and never stolen.
	 *
	 * @param game_mode Game mode to own - parentless or already a child
	 *                  of this world - or @c nullptr to clear.
	 * @param keep
	 */
	void set_game_mode(FGameMode* game_mode, bool keep);

	/**
	 * @brief Returns this world's game mode.
	 *
	 * @return The owned FGameMode (also a child node of this world), or
	 *         @c nullptr when none is set.
	 */
	[[nodiscard]] FGameMode* get_game_mode() const;

	/**
	 * @brief Sets the scene used to spawn the game mode at ready.
	 *
	 * The scene's root node must derive from FGameMode (checked with
	 * fragile::utils::is_scene_root_derived_from); anything else is
	 * refused with an error and the previous prefab is kept. Passing
	 * @c nullptr clears the prefab.
	 *
	 * @param prefab Scene whose root is an FGameMode subclass, or @c nullptr.
	 */
	void set_game_mode_prefab(const Ref<PackedScene>& prefab);

	/**
	 * @brief Returns the scene used to spawn the game mode at ready.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] Ref<PackedScene> get_game_mode_prefab() const;

	void set_default_level_prefab(const Ref<PackedScene>& prefab);

	[[nodiscard]] Ref<PackedScene> get_default_level_prefab() const;

	void _ready() override;

	void _enter_tree() override;

	void _exit_tree() override;

	/**
	 * @brief Returns the one world that currently owns the scene tree.
	 *
	 * Bound to scripts as the static @c FWorld.get_current_world(). The
	 * current world is the FWorld that has entered the scene tree; it is
	 * cleared again when that world leaves the tree or is freed.
	 *
	 * @return The current FWorld, or @c nullptr while no world is in the
	 *         scene tree.
	 */
	static FWorld* get_current_world();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_WORLD_H__
