
#ifndef __FRAGILE_TYPE_CHECK_H__
#define __FRAGILE_TYPE_CHECK_H__

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <type_traits>

namespace fragile::utils {

/**
 * @brief Checks whether a scene's root node derives from @p class_name.
 *
 * Compares the scene's root node type against @p class_name; for an
 * instanced scene (a root with an empty type) the check recurses into
 * the instanced scene. Used to validate prefab properties such as
 * FWorld::game_mode_prefab, FLevel::override_game_mode and
 * FGameMode::character_prefab.
 *
 * @param scene Scene to inspect; @c nullptr yields @c false.
 * @param class_name Class the root node must be or derive from.
 * @return @c true when the scene's root derives from @p class_name.
 */
bool is_scene_root_derived_from(const godot::Ref<godot::PackedScene>& scene, const godot::StringName& class_name);

/**
 * @brief Instantiates @p scene and returns its root as @p T.
 *
 * Returns @c nullptr when the scene is null, cannot be instantiated, or its
 * root is not an @p T; a wrongly typed root is queue_freed before returning.
 *
 * @tparam T Expected type of the scene's root node.
 * @param scene Scene to instantiate.
 * @return The instantiated root cast to @p T, or @c nullptr.
 */
template <typename T>
T* instantiate(const godot::Ref<godot::PackedScene>& scene) {
	if (scene.is_null()) {
		return nullptr;
	}

	auto* obj = scene->instantiate();
	if (obj == nullptr) {
		return nullptr;
	}

	if (auto* ins = godot::Object::cast_to<std::remove_const_t<T>>(obj); ins != nullptr) {
		return ins;
	}

	obj->queue_free();
	return nullptr;
}

}	 // namespace fragile::utils

#endif	// __FRAGILE_TYPE_CHECK_H__
