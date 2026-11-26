#ifndef __FRAGILE_FRAMEWORK_WORLD_H__
#define __FRAGILE_FRAMEWORK_WORLD_H__

#include "fragile/framework/game_mode.h"
#include "fragile/framework/level.h"
#include "fragile/global.h"

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
