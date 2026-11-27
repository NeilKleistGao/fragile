#include "fragile/framework/level.h"

#include "fragile/framework/actor.h"
#include "fragile/framework/character.h"
#include "fragile/framework/controller.h"
#include "fragile/framework/game_mode.h"
#include "fragile/framework/world.h"
#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"
#include "fragile/utils/type_check.h"

namespace fragile::framework {

void FLevel::_bind_methods() {
	ADD_METHOD(FLevel, add_actor, "actor");
	ADD_PREFAB_PROPERTY(FLevel, override_game_mode);
}

void FLevel::spawn_game_mode() {
	auto game_mode_prefab = override_game_mode;
	if (game_mode_prefab.is_null()) {
		const auto* world = FWorld::get_current_world();
		if (world == nullptr) {
			print_error(String{ "Cannot load default game mode: current world not found." });
			return;
		}
		game_mode_prefab = world->get_game_mode_prefab();
	}

	if (auto* game_mode = utils::instantiate<FGameMode>(game_mode_prefab); game_mode != nullptr) {
		this->game_mode = game_mode;
		add_child(game_mode);
	}
	else {
		print_error(String{ "No game mode specified." });
	}
}

void FLevel::spawn_player_with_controller() {
	const auto& character_prefab = game_mode->get_character_prefab();
	if (auto* character = utils::instantiate<FCharacter>(character_prefab); character != nullptr) {
		player = character;
		add_child(player);
	}

	const auto& controller_prefab = game_mode->get_controller_prefab();
	if (auto* ctrl = utils::instantiate<FController>(controller_prefab); ctrl != nullptr) {
		controller = ctrl;
		add_child(controller);
	}

	if (player != nullptr && controller != nullptr) {
		controller->control(player);
	}
}

void FLevel::add_actor(FActor* actor) {
	if (actor == nullptr) {
		return;
	}

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

	print_error("cannot set override_game_mode: the scene root does not derive from FGameMode.");
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

void FLevel::_notification(int p_what) {
	if (p_what != NOTIFICATION_PREDELETE) {
		return;
	}

	// The scene tree frees the children; only drop the pointers so nothing can
	// reach a node that is already gone.
	game_mode = nullptr;
	player = nullptr;
	controller = nullptr;
}

FCharacter* FLevel::get_player_character() const {
	return player;
}

FController* FLevel::get_player_controller() const {
	return controller;
}

}	 // namespace fragile::framework
