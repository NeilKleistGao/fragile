#include "level.h"

#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"
#include "fragile/utils/type_check.h"
#include "world.h"

#include <godot_cpp/core/class_db.hpp>

namespace fragile::framework {

void FLevel::_bind_methods() {
	ADD_METHOD(FLevel, add_actor, "actor");
	ADD_PREFAB_PROPERTY(FLevel, override_game_mode);
}

void FLevel::spawn_game_mode() {
	auto& game_mode_prefab = override_game_mode;
	if (game_mode_prefab.is_null()) {
		const auto* world = FWorld::get_current_world();
		if (world == nullptr) {
			print_error(String{ "Cannot load default game mode: current world not found." });
			return;
		}
		game_mode_prefab = world->get_game_mode_prefab();
	}

	if (game_mode_prefab.is_null()) {
		print_error(String{ "No game mode specified." });
		return;
	}

	if (auto* game_mode = cast_to<FGameMode>(game_mode_prefab->instantiate()); game_mode != nullptr) {
		this->game_mode = game_mode;
		add_child(game_mode);
	}
}

void FLevel::spawn_player_with_controller() {
	// TODO:
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

void FLevel::_ready() {
	Node2D::_ready();

	FRAGILE_RUNTIME_API;

	spawn_game_mode();
	if (game_mode != nullptr) {
		spawn_player_with_controller();
	}
}

}	 // namespace fragile::framework
