#ifndef __FRAGILE_FRAMEWORK_LEVEL_H__
#define __FRAGILE_FRAMEWORK_LEVEL_H__

#include "actor.h"

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
class FLevel : public godot::Node2D {
	GDCLASS(FLevel, godot::Node2D)

private:
	godot::Array actors;

	/**
	 * @brief Registers @p actor and adds it as a child node.
	 *
	 * Nothing is added when an actor with the same name is already
	 * registered; an error is printed instead.
	 *
	 * @param actor Actor to attach to this level.
	 */
	void add_actor(FActor* actor);

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Creates an actor of type @p actor_type_name and attaches it.
	 *
	 * The class is instantiated through ClassDB, renamed to @p actor_name and
	 * registered with add_actor().
	 *
	 * @param actor_type_name Class name of an FActor subclass registered with ClassDB.
	 * @param actor_name Name the new actor node receives.
	 * @return The new actor, or @c nullptr when the class cannot be created.
	 */
	FActor* spawn_actor(const godot::StringName& actor_type_name, const godot::StringName& actor_name);
	// TODO: spawn actor by a packed scene?
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_LEVEL_H__
