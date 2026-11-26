#include "type_check.h"

#include <godot_cpp/classes/scene_state.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace fragile::utils {

bool is_scene_root_derived_from(const godot::Ref<godot::PackedScene>& scene, const godot::StringName& class_name) {
	if (scene.is_null()) {
		return false;
	}

	const auto state = scene->get_state();
	if (state.is_null() || state->get_node_count() == 0) {
		return false;
	}

	if (const godot::StringName root_type = state->get_node_type(0); !root_type.is_empty()) {
		return root_type == class_name || godot::ClassDB::is_parent_class(root_type, class_name);
	}

	return is_scene_root_derived_from(state->get_node_instance(0), class_name);
}

}	 // namespace fragile::utils
