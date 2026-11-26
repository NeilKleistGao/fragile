#include "world.h"

#include "character.h"
#include "controller.h"
#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"
#include "fragile/utils/type_check.h"
#include "level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>

namespace fragile::framework {

FWorld* FWorld::instance = nullptr;

void FWorld::_bind_methods() {
	ADD_METHOD(FWorld, open_level, "level_name");
	ADD_PREFAB_PROPERTY(FWorld, game_mode_prefab);
	ADD_PREFAB_PROPERTY(FWorld, default_level_prefab);
	ADD_STATIC_METHOD(FWorld, get_current_world);
	ADD_METHOD(FWorld, get_player_character);
	ADD_METHOD(FWorld, get_player_controller);
}

void FWorld::open_level(const StringName& level_name) {
	if (ClassDB::class_exists(level_name) && ClassDB::can_instantiate(level_name)) {
		Object* object = ClassDB::instantiate(level_name);
		if (auto* level = cast_to<FLevel>(object); level != nullptr) {
			if (current_level != nullptr) {
				remove_child(current_level);
				current_level->queue_free();
			}

			current_level = level;
			add_child(current_level);
			return;
		}

		if (object != nullptr && cast_to<RefCounted>(object) == nullptr) {
			memdelete(object);
		}
	}

	print_error(String("Cannot create ") + level_name);
}

void FWorld::set_game_mode_prefab(const Ref<PackedScene>& prefab) {
	if (prefab.is_null()) {
		game_mode_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, StringName{ "FGameMode" })) {
		game_mode_prefab = prefab;
		return;
	}

	print_error("cannot set game_mode_prefab: the scene root does not derive from FGameMode.");
}

Ref<PackedScene> FWorld::get_game_mode_prefab() const {
	return game_mode_prefab;
}

void FWorld::set_default_level_prefab(const Ref<PackedScene>& prefab) {
	if (prefab.is_null()) {
		default_level_prefab = nullptr;
		return;
	}

	if (utils::is_scene_root_derived_from(prefab, StringName{ "FLevel" })) {
		default_level_prefab = prefab;
		return;
	}

	print_error("cannot set default_level_prefab: the scene root does not derive from FLevel.");
}

Ref<PackedScene> FWorld::get_default_level_prefab() const {
	return default_level_prefab;
}

void FWorld::_ready() {
	Node2D::_ready();

	FRAGILE_RUNTIME_API;

	if (auto* level = utils::instantiate<FLevel>(default_level_prefab); level != nullptr) {
		current_level = level;
		add_child(current_level);
	}
	else {
		print_error("No default level specified.");
	}
}

void FWorld::_enter_tree() {
	Node2D::_enter_tree();

	FRAGILE_RUNTIME_API;

	ERR_FAIL_COND_MSG(instance != nullptr, "Multiple world definitions.");
	instance = this;
}

void FWorld::_exit_tree() {
	Node2D::_exit_tree();

	FRAGILE_RUNTIME_API;

	instance = nullptr;
}

FWorld* FWorld::get_current_world() {
	return instance;
}

FCharacter* FWorld::get_player_character() const {
	return (current_level == nullptr) ? nullptr : current_level->get_player_character();
}

FController* FWorld::get_player_controller() const {
	return (current_level == nullptr) ? nullptr : current_level->get_player_controller();
}

}	 // namespace fragile::framework
