#ifndef __FRAGILE_FRAMEWORK_WORLD_H__
#define __FRAGILE_FRAMEWORK_WORLD_H__

#include "fragile/framework/level.h"
#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node2d.hpp>

namespace fragile::framework {

/**
 * @brief Root node of the framework: owns the currently open level.
 *
 * Opens levels by class name and serializes the world as the current level
 * wrapped in its own dictionary.
 */
class FWorld : public godot::Node2D, IMPLEMENTS(ISerializable) {
	GDCLASS(FWorld, godot::Node2D)

private:
	FLevel* current_level{ nullptr };

protected:
	static void _bind_methods();

	static constexpr const char* s_serialization_level_name = "level";
	static constexpr const char* s_serialization_type_name = "type";

public:
	/**
	 * @brief Serializes the current level into @p data.
	 *
	 * When a level is open, a dictionary is stored under the key @c level
	 * containing the level's @c type (its class name) and the state produced
	 * by FLevel::serialize().
	 *
	 * @param data Dictionary that receives the serialized state.
	 * @return @c godot::OK on success (also when no level is open);
	 *         @c godot::ERR_INVALID_DATA when the @c level key is already
	 *         present or the level fails to serialize.
	 */
	[[nodiscard]] godot::Error serialize(godot::Dictionary data) const override;

	/**
	 * @brief Restores the current level from @p data.
	 *
	 * When the @c level key is present, the referenced level class is opened
	 * with open_level() and then restored from the same dictionary. A
	 * dictionary without the @c level key leaves the world untouched.
	 *
	 * @param data Dictionary previously produced by serialize().
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when the
	 *         data is malformed or the level cannot be created or restored.
	 */
	[[nodiscard]] godot::Error deserialize(const godot::Dictionary& data) override;

	/**
	 * @brief Opens the level of type @p level_name as the current level.
	 *
	 * The class is instantiated through ClassDB and added as a child. A
	 * previously opened level is removed and freed. An error is printed when
	 * the level cannot be created.
	 *
	 * @param level_name Class name of an FLevel subclass registered with ClassDB.
	 */
	void open_level(const godot::StringName& level_name);
	// TODO: open a level by packed scene?
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_WORLD_H__
