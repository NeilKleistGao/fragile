#ifndef __FRAGILE_F_INFO_H__
#define __FRAGILE_F_INFO_H__

#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node.hpp>

namespace fragile::framework {

/**
 * @brief Base class of all "utility" objects not placed in the game world.
 *
 * Mirrors Unreal Engine's AInfo: it carries game information (rules, global
 * state) without being part of a level. FInfo has no state of its own; its
 * serialization is a no-op that derived classes extend.
 */
class FInfo : public godot::Node, IMPLEMENTS(ISerializable) {
	GDCLASS(FInfo, godot::Node)

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Serializes nothing: FInfo has no state of its own.
	 *
	 * @param data Dictionary that would receive the serialized state; unused.
	 * @return Always @c godot::OK.
	 */
	[[nodiscard]] godot::Error serialize(godot::Dictionary data) const override;

	/**
	 * @brief Restores nothing: FInfo has no state of its own.
	 *
	 * @param data Dictionary previously produced by serialize(); unused.
	 * @return Always @c godot::OK.
	 */
	[[nodiscard]] godot::Error deserialize(const godot::Dictionary& data) override;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_F_INFO_H__
