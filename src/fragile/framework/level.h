#ifndef __FRAGILE_FRAMEWORK_LEVEL_H__
#define __FRAGILE_FRAMEWORK_LEVEL_H__

#include "actor.h"
#include "fragile/global.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/array.hpp>

namespace fragile::framework {

/**
 * @brief A container of actors.
 *
 * Mirrors Unreal Engine's ULevel: it holds the actors that make up one part
 * of the game world. Levels are created and opened at runtime through
 * FWorld::open_level.
 */
class FLevel : public Node2D {
	GDCLASS(FLevel, Node2D)

private:
	Array actors;

	Ref<PackedScene> override_game_mode;

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Registers @p actor and adds it as a child node.
	 *
	 * Nothing is added when an actor with the same name is already
	 * registered; an error is printed instead.
	 *
	 * @param actor Actor to attach to this level.
	 */
	void add_actor(FActor* actor);

	void set_override_game_mode(const Ref<PackedScene>& prefab);
	[[nodiscard]] Ref<PackedScene> get_override_game_mode() const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_LEVEL_H__
