#ifndef __FRAGILE_SERIALIZABLE_H__
#define __FRAGILE_SERIALIZABLE_H__

#include <godot_cpp/variant/variant.hpp>

namespace fragile::interface {

/**
 * @brief Interface for objects that persist their state in a plain Dictionary.
 *
 * Implementations convert their state to and from flat Dictionary entries so
 * that a whole framework tree (FWorld / FLevel / FActor) can be saved and
 * restored through nested dictionaries.
 */
class ISerializable {
public:
	virtual ~ISerializable() = default;

	/**
	 * @brief Writes this object's state into @p data.
	 *
	 * Each serialized property is stored under its own key. Implementations
	 * must not overwrite entries that already exist in @p data.
	 *
	 * @param data Dictionary that receives the serialized state.
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when @p data
	 *         already contains conflicting entries.
	 * @note Dictionaries are reference types in Godot: writes through @p data
	 *       stay visible to the caller although it is passed by value.
	 */
	[[nodiscard]] virtual godot::Error serialize(godot::Dictionary data) const = 0;

	/**
	 * @brief Restores this object's state from @p data.
	 *
	 * @param data Dictionary previously produced by serialize().
	 * @return @c godot::OK on success; @c godot::ERR_INVALID_DATA when @p data
	 *         is malformed or incomplete for this object.
	 */
	[[nodiscard]] virtual godot::Error deserialize(const godot::Dictionary& data) = 0;
};

}	 // namespace fragile::interface

#endif	// __FRAGILE_SERIALIZABLE_H__
