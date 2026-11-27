
#ifndef __FRAGILE_GAME_MODE_H__
#define __FRAGILE_GAME_MODE_H__

#include "fragile/framework/info.h"
#include "fragile/global.h"

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace fragile::framework {

/**
 * @brief Base class for the rules that govern one game session.
 *
 * Mirrors Unreal Engine's AGameMode: it carries the game rules and the
 * session state of one level. Like every FInfo it is a plain Node with no
 * transform, but it is not free-floating: FLevel::spawn_game_mode makes it a
 * child node of the level that loaded it, and the level owns it.
 *
 * A game mode describes the player a level spawns: character_prefab and
 * controller_prefab are instantiated by FLevel at ready. The level chooses
 * the game mode scene itself (FLevel::set_override_game_mode) or falls back
 * to the current world's FWorld::get_game_mode_prefab().
 *
 * Never free a game mode from user code: its lifetime belongs to the
 * framework. The owning FLevel frees it together with the level.
 *
 * FGameMode itself adds no rules beyond the player prefabs. Derived classes
 * add the actual rules.
 */
class FGameMode : public FInfo {
	GDCLASS(FGameMode, FInfo)

protected:
	static void _bind_methods();

private:
	/**
	 * @brief Scene whose root derives from FCharacter, used to spawn the player.
	 *
	 * Assigned through the @c character_prefab property and validated by
	 * set_character_prefab(). @c nullptr means the level spawns no player
	 * character.
	 */
	Ref<PackedScene> character_prefab;

	/**
	 * @brief Scene whose root derives from FController, used to spawn the player controller.
	 *
	 * Assigned through the @c controller_prefab property and validated by
	 * set_controller_prefab(). @c nullptr means the level spawns no player
	 * controller.
	 */
	Ref<PackedScene> controller_prefab;

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
	void set_character_prefab(const Ref<PackedScene>& prefab);

	/**
	 * @brief Returns the scene used to spawn player characters.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] Ref<PackedScene> get_character_prefab() const;

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
	void set_controller_prefab(const Ref<PackedScene>& prefab);

	/**
	 * @brief Returns the scene used to spawn player controllers.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] Ref<PackedScene> get_controller_prefab() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_GAME_MODE_H__
