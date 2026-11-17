#ifndef __FRAGILE_FRAMEWORK_WORLD_H__
#define __FRAGILE_FRAMEWORK_WORLD_H__

#include "fragile/framework/game_mode.h"
#include "fragile/framework/level.h"
#include "fragile/interface/interfaces.h"
#include "fragile/interface/serializable.h"

#include <godot_cpp/classes/node2d.hpp>

namespace fragile::framework {

/**
 * @brief The primary container for everything that comprises a level or scene.
 *
 * Mirrors Unreal Engine's UWorld: it owns the currently open level with its
 * actors and switches levels at runtime (open_level). It also owns its game
 * mode: set_game_mode makes the game mode a child node of this world and
 * frees the previous one, and game_mode_prefab spawns it at ready. The
 * whole world serializes as the current level wrapped in its own
 * dictionary.
 */
class FWorld : public godot::Node2D, IMPLEMENTS(ISerializable) {
	GDCLASS(FWorld, godot::Node2D)

private:
	friend class FGameMode;	 // a freed game mode drops its world's link

	FLevel* current_level{ nullptr };

	/**
	 * @brief Game mode owned by this world; @c nullptr when none is set.
	 *
	 * A child node of this world, created by set_game_mode() or spawned
	 * from game_mode_prefab at ready. Freed together with the world.
	 */
	FGameMode* game_mode{ nullptr };

	/**
	 * @brief Scene whose root derives from FGameMode, spawned at ready.
	 *
	 * Assigned through the @c game_mode_prefab property and validated by
	 * set_game_mode_prefab().
	 */
	godot::PackedScene* game_mode_prefab{ nullptr };

protected:
	static void _bind_methods();

	static constexpr const char* s_serialization_level_name = "level";
	static constexpr const char* s_serialization_type_name = "type";

	void _notification(int p_what) override;
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

	/**
	 * @brief Makes @p game_mode this world's game mode and child node.
	 *
	 * The world owns its game mode: @p game_mode is added below this world
	 * in the scene tree and linked back through FGameMode::set_world(), and
	 * a previously set game mode is removed from the tree and freed.
	 * Passing @c nullptr clears the game mode the same way.
	 *
	 * The game mode must not have a parent yet - the world adopts it as
	 * its child and never steals it from an existing parent. A call with
	 * an already-parented game mode is refused with an error and changes
	 * nothing.
	 *
	 * @param game_mode Parentless game mode to own, or @c nullptr to clear.
	 */
	void set_game_mode(FGameMode* game_mode);

	/**
	 * @brief Returns this world's game mode.
	 *
	 * @return The owned FGameMode (also a child node of this world), or
	 *         @c nullptr when none is set.
	 */
	[[nodiscard]] FGameMode* get_game_mode() const;

	/**
	 * @brief Sets the scene used to spawn the game mode at ready.
	 *
	 * The scene's root node must derive from FGameMode (checked with
	 * fragile::utils::is_scene_root_derived_from); anything else is
	 * refused with an error and the previous prefab is kept. Passing
	 * @c nullptr clears the prefab.
	 *
	 * @param prefab Scene whose root is an FGameMode subclass, or @c nullptr.
	 */
	void set_game_mode_prefab(godot::PackedScene* prefab);

	/**
	 * @brief Returns the scene used to spawn the game mode at ready.
	 *
	 * @return The assigned PackedScene, or @c nullptr when none is set.
	 */
	[[nodiscard]] godot::PackedScene* get_game_mode_prefab() const;

	void _ready() override;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_FRAMEWORK_WORLD_H__
