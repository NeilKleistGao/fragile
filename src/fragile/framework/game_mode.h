
#ifndef __FRAGILE_GAME_MODE_H__
#define __FRAGILE_GAME_MODE_H__

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
 * is set explicitly. Freeing the game mode early clears the world's link.
 *
 * FGameMode itself adds no rules and no state beyond FInfo: serialization
 * is the inherited no-op. Derived classes add the actual rules and extend
 * serialize()/deserialize() as needed.
 */
class FGameMode : public FInfo {
	GDCLASS(FGameMode, FInfo)

protected:
	static void _bind_methods();

	void _notification(int p_what);

private:
	/**
	 * @brief World this game mode is linked to; @c nullptr when unlinked.
	 *
	 * Maintained by FWorld::set_game_mode through set_world(). The world
	 * owns this node: it lives as a child of its world and is freed with it.
	 */
	FWorld* world{ nullptr };

public:
	/**
	 * @brief Points this game mode at @p world.
	 *
	 * Called by FWorld::set_game_mode whenever the link changes: @p world
	 * when this game mode becomes the world's game mode, @c nullptr when
	 * it is replaced. Not exposed to scripts - use FWorld::set_game_mode or
	 * its @c game_mode_prefab property instead.
	 *
	 * @param world World to link to, or @c nullptr to unlink.
	 */
	void set_world(FWorld* world);

	/**
	 * @brief Returns the world this game mode is linked to.
	 *
	 * @return The linked FWorld, or @c nullptr when unlinked (including
	 *         before the first FWorld::set_game_mode).
	 */
	[[nodiscard]] FWorld* get_world() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_GAME_MODE_H__
