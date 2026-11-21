
#ifndef __FRAGILE_GAME_MODE_H__
#define __FRAGILE_GAME_MODE_H__

#include "godot_cpp/classes/packed_scene.hpp"
#include "info.h"

namespace fragile::framework {

class FWorld;

/**
 * @brief Base class for the rules that govern one game session.
 *
 * Mirrors Unreal Engine's AGameMode: it carries the game rules and the
 * global session state of an FWorld. Like every FInfo it is not part of
 * any level. FWorld::set_game_mode makes it the world's game mode and a
 * child node of that world; the world owns its game mode and frees it when
 * it is replaced or when the world itself is freed. Alternatively the
 * FWorld's @c game_mode_prefab property spawns it from a scene at ready.
 * A world starts out without a game mode and keeps it that way until one
 * is set explicitly.
 *
 * Never free a game mode from user code: its lifetime belongs to the
 * framework. The owning FWorld frees its game mode - when the game mode
 * is replaced, and always when the world itself is freed.
 *
 * FGameMode itself adds no rules and no state beyond FInfo. Derived
 * classes add the actual rules.
 */
class FGameMode : public FInfo {
	GDCLASS(FGameMode, FInfo)

protected:
	static void _bind_methods();

private:
	godot::PackedScene* character_prefab{ nullptr };

	godot::PackedScene* controller_prefab{ nullptr };

public:
	/**
	 * @brief Sets the scene used to spawn player characters.
	 *
	 * The scene's root node must derive from FCharacter (checked with
	 * fragile::utils::is_scene_root_derived_from); anything else is
	 * refused with an error and the previous prefab is kept. Passing
	 * @c nullptr clears the prefab.
	 *
	 * @param prefab Scene whose root is an FCharacter subclass, or @c nullptr.
	 */
	void set_character_prefab(godot::PackedScene* prefab);

	/**
	 * @brief Returns the scene used to spawn player characters.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] godot::PackedScene* get_character_prefab() const;

	/**
	 * @brief Sets the scene used to spawn player controllers.
	 *
	 * The scene's root node must derive from FController (checked with
	 * fragile::utils::is_scene_root_derived_from); anything else is
	 * refused with an error and the previous prefab is kept. Passing
	 * @c nullptr clears the prefab.
	 *
	 * @param prefab Scene whose root is an FController subclass, or @c nullptr.
	 */
	void set_controller_prefab(godot::PackedScene* prefab);

	/**
	 * @brief Returns the scene used to spawn player controllers.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] godot::PackedScene* get_controller_prefab() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_GAME_MODE_H__
