
#ifndef __FRAGILE_TYPE_CHECK_H__
#define __FRAGILE_TYPE_CHECK_H__

#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/ref.hpp"

namespace fragile::utils {

/**
 * @brief Checks whether a scene's root node derives from @p class_name.
 *
 * Compares the scene's root node type against @p class_name; for an
 * instanced scene (a root with an empty type) the check recurses into
 * the instanced scene. Used to validate FWorld::game_mode_prefab, whose
 * root must be an FGameMode subclass.
 *
 * @param scene Scene to inspect; @c nullptr yields @c false.
 * @param class_name Class the root node must be or derive from.
 * @return @c true when the scene's root derives from @p class_name.
 */
bool is_scene_root_derived_from(const godot::Ref<godot::PackedScene>& scene, const godot::StringName& class_name);

}	 // namespace fragile::utils

#endif	// __FRAGILE_TYPE_CHECK_H__
