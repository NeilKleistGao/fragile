
#include "game_mode.h"

#include "fragile/utils/register_helper.h"
#include "fragile/utils/type_check.h"

namespace fragile::framework {

void FGameMode::_bind_methods() {
	ADD_PREFAB_PROPERTY(FGameMode, character_prefab);
	ADD_PREFAB_PROPERTY(FGameMode, controller_prefab);
}

void FGameMode::set_character_prefab(const Ref<PackedScene>& prefab) {
	if (prefab.is_null()) {
		character_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, StringName{ "FCharacter" })) {
		character_prefab = prefab;
		return;
	}

	print_error("cannot set character_prefab: the scene root does not derive from FCharacter.");
}

Ref<PackedScene> FGameMode::get_character_prefab() const {
	return character_prefab;
}

void FGameMode::set_controller_prefab(const Ref<PackedScene>& prefab) {
	if (prefab.is_null()) {
		controller_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, StringName{ "FController" })) {
		controller_prefab = prefab;
		return;
	}

	print_error("cannot set controller_prefab: the scene root does not derive from FController.");
}

Ref<PackedScene> FGameMode::get_controller_prefab() const {
	return controller_prefab;
}

}	 // namespace fragile::framework
