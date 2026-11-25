#include "level.h"

#include "fragile/utils/register_helper.h"
#include "fragile/utils/type_check.h"

#include <godot_cpp/core/class_db.hpp>

namespace fragile::framework {

void FLevel::_bind_methods() {
	ADD_METHOD(FLevel, add_actor, "actor");
	ADD_PREFAB_PROPERTY(FLevel, override_game_mode);
}

void FLevel::add_actor(FActor* actor) {
	for (const auto& v : actors) {
		if (const auto* act = cast_to<FActor>(v.get_validated_object()); act != nullptr && act->get_name() == actor->get_name()) {
			print_error(String("Duplicate actor name: ") + actor->get_name());
			return;
		}
	}

	actors.append(actor);
	add_child(actor);
}

void FLevel::set_override_game_mode(const Ref<PackedScene>& prefab) {
	if (prefab.is_null()) {
		override_game_mode = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, StringName{ "FGameMode" })) {
		override_game_mode = prefab;
		return;
	}

	print_error("cannot set game_mode_prefab: the scene root does not derive from FGameMode.");
}

Ref<PackedScene> FLevel::get_override_game_mode() const {
	return override_game_mode;
}

}	 // namespace fragile::framework
