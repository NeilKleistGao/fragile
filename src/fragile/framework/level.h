#ifndef __FRAGILE_FRAMEWORK_LEVEL_H__
#define __FRAGILE_FRAMEWORK_LEVEL_H__

#include "actor.h"
#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/array.hpp>

namespace fragile::framework {

/**
 * @brief A container of actors.
 *
 * Mirrors Unreal Engine's ULevel: it holds the actors that make up one part
 * of the game world. Levels are created and opened at runtime through
 * FWorld::open_level and can be serialized to and restored from a plain
 * dictionary together with all their actors.
 */
class FLevel : public godot::Node2D, IMPLEMENTS(ISerializable) {
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

	static constexpr const char* s_serialization_actors_name = "actors";
	static constexpr const char* s_serialization_type_name = "type";

public:
	/**
	 * @brief Serializes all registered actors into @p data.
	 *
	 * Stores a dictionary under the key @c actors that maps each actor name to
	 * a sub-dictionary containing the actor's @c type (its class name) and the
	 * state produced by FActor::serialize().
	 *
	 * @param data Dictionary that receives the serialized state.
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when the
	 *         @c actors key is already present, actor names collide, or an
	 *         actor fails to serialize.
	 */
	[[nodiscard]] godot::Error serialize(godot::Dictionary data) const override;

	/**
	 * @brief Restores the level's actors from @p data.
	 *
	 * Expects the @c actors dictionary written by serialize(): every entry
	 * must map an actor name to a dictionary carrying the actor's @c type.
	 * Each actor is recreated with spawn_actor() and then restored.
	 *
	 * @param data Dictionary previously produced by serialize().
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when the
	 *         data is malformed or an actor cannot be created or restored.
	 */
	[[nodiscard]] godot::Error deserialize(const godot::Dictionary& data) override;

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
