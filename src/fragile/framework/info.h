#ifndef __FRAGILE_F_INFO_H__
#define __FRAGILE_F_INFO_H__

#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node.hpp>

namespace fragile::framework {

/**
 * @brief Base class for side information nodes.
 *
 * A plain Node that implements ISerializable without carrying any state of
 * its own; serialization always succeeds and stores nothing. Derived classes
 * are expected to extend the serialization.
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
