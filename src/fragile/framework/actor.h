#ifndef __FRAGILE_FRAMEWORK_ACTOR_H__
#define __FRAGILE_FRAMEWORK_ACTOR_H__

#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node2d.hpp>

namespace fragile::framework {

/**
 * @brief Base class for in-level entities.
 *
 * A Node2D that implements ISerializable: an actor persists its local
 * position so that levels can rebuild their contents from plain dictionaries.
 */
class FActor : public godot::Node2D, IMPLEMENTS(ISerializable) {
	GDCLASS(FActor, godot::Node2D)

protected:
	static void _bind_methods();

	static constexpr const char* s_serialization_local_position_x_name = "local_position_x";
	static constexpr const char* s_serialization_local_position_y_name = "local_position_y";

public:
	/**
	 * @brief Serializes the actor's local position into @p data.
	 *
	 * Stores the position under the keys @c local_position_x and
	 * @c local_position_y.
	 *
	 * @param data Dictionary that receives the serialized state.
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when either
	 *         position key is already present in @p data.
	 */
	[[nodiscard]] godot::Error serialize(godot::Dictionary data) const override;

	/**
	 * @brief Restores the actor's local position from @p data.
	 *
	 * Reads @c local_position_x and @c local_position_y and applies them with
	 * set_position().
	 *
	 * @param data Dictionary previously produced by serialize().
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when either
	 *         position key is missing.
	 */
	[[nodiscard]] godot::Error deserialize(const godot::Dictionary& data) override;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_ACTOR_H__
