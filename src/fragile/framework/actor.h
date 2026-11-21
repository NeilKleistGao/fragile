#ifndef __FRAGILE_FRAMEWORK_ACTOR_H__
#define __FRAGILE_FRAMEWORK_ACTOR_H__

#include <godot_cpp/classes/node2d.hpp>

namespace fragile::framework {

/**
 * @brief Base class for objects that can be placed or spawned in a level.
 *
 * Mirrors Unreal Engine's AActor: the basic building block of the game
 * world. An actor lives inside an FLevel and can be spawned at runtime by
 * class name (FLevel::spawn_actor).
 */
class FActor : public godot::Node2D {
	GDCLASS(FActor, godot::Node2D)

protected:
	static void _bind_methods();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_ACTOR_H__
