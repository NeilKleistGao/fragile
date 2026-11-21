
#include "game_mode.h"

#include "fragile/utils/type_check.h"
#include "world.h"

namespace fragile::framework {

void FGameMode::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("set_character_prefab", "prefab"), &FGameMode::set_character_prefab);
	godot::ClassDB::bind_method(godot::D_METHOD("get_character_prefab"), &FGameMode::get_character_prefab);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "character_prefab", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_character_prefab", "get_character_prefab");

	godot::ClassDB::bind_method(godot::D_METHOD("set_controller_prefab", "prefab"), &FGameMode::set_controller_prefab);
	godot::ClassDB::bind_method(godot::D_METHOD("get_controller_prefab"), &FGameMode::get_controller_prefab);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "controller_prefab", godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_controller_prefab", "get_controller_prefab");
}

void FGameMode::set_character_prefab(godot::PackedScene* prefab) {
	if (prefab == nullptr) {
		character_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, godot::StringName{ "FCharacter" })) {
		character_prefab = prefab;
		return;
	}

	godot::print_error("cannot set character_prefab: the scene root does not derive from FCharacter.");
}

godot::PackedScene* FGameMode::get_character_prefab() const {
	return character_prefab;
}

void FGameMode::set_controller_prefab(godot::PackedScene* prefab) {
	if (prefab == nullptr) {
		controller_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, godot::StringName{ "FController" })) {
		controller_prefab = prefab;
		return;
	}

	godot::print_error("cannot set controller_prefab: the scene root does not derive from FController.");
}

godot::PackedScene* FGameMode::get_controller_prefab() const {
	return controller_prefab;
}

}	 // namespace fragile::framework
