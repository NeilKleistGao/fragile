#include "fragile/framework/enhanced_input.h"

#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/window.hpp"

#include <algorithm>

namespace fragile::framework {

void FEnhancedInput::_bind_methods() {
	ADD_METHOD(FEnhancedInput, rebind, "action", "event");
	ADD_METHOD(FEnhancedInput, is_bound, "event");
	ADD_METHOD(FEnhancedInput, attach_to_tree);
}

void FEnhancedInput::attach_to_tree() {
	if (is_inside_tree()) {
		return;
	}

	// Runtime only: in the editor the dispatcher skips _ready() (see
	// FRAGILE_RUNTIME_API) and would only consume editor input.
	if (utils::is_in_editor()) {
		return;
	}

	SceneTree* scene_tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (scene_tree == nullptr) {
		// The main loop is created after module initialization; the deferred
		// call normally runs once it exists. Doctool-style runs have none.
		return;
	}

	set_name("EnhancedInput");
	scene_tree->get_root()->add_child(this);
}

void FEnhancedInput::_notification(int p_what) {
	if (p_what != NOTIFICATION_PREDELETE) {
		return;
	}

	// The scene tree owns this node at runtime and frees it before the module
	// terminator runs. Remove the engine singleton entry now: leaving a raw
	// pointer there would make get_singleton() hand out a dangling object for
	// the rest of the teardown.
	Engine* engine = Engine::get_singleton();
	if (engine == nullptr || !engine->has_singleton("EnhancedInput")) {
		return;
	}
	if (engine->get_singleton("EnhancedInput") == this) {
		engine->unregister_singleton("EnhancedInput");
	}
}

void FEnhancedInput::broadcast_action(const StringName& action, bool started, bool ongoing, bool completed) {
	if (listeners.find(action) == listeners.end()) {
		return;
	}

	for (auto* listener : listeners[action]) {
		listener->receive_action_data(action, started, ongoing, completed);
	}
}

InputMap* FEnhancedInput::get_input_map() const {
	if (input_map != nullptr) {
		return input_map;
	}

	return InputMap::get_singleton();
}

void FEnhancedInput::_ready() {
	FInfo::_ready();

	FRAGILE_RUNTIME_API;

	input_map = InputMap::get_singleton();
}

void FEnhancedInput::_physics_process(double p_delta) {
	FInfo::_physics_process(p_delta);

	std::for_each(activated_actions.begin(), activated_actions.end(), [this](const StringName& action) {
		broadcast_action(action, false, true, false);
	});
}

void FEnhancedInput::_unhandled_input(const Ref<InputEvent>& p_event) {
	FInfo::_unhandled_input(p_event);

	InputMap* map = get_input_map();
	if (map == nullptr) {
		return;
	}

	const auto& all_actions = map->get_actions();
	for (const auto& action : all_actions) {
		if (p_event->is_action_pressed(action)) {
			broadcast_action(action, true, false, false);
			activated_actions.insert(action);
		}
		else if (p_event->is_action_released(action)) {
			broadcast_action(action, false, false, true);
			activated_actions.erase(action);
		}
	}
}

void FEnhancedInput::rebind(const StringName& action, const Ref<InputEvent>& event) const {
	InputMap* map = get_input_map();
	if (map == nullptr) {
		return;
	}

	if (map->action_has_event(action, event)) {
		return;
	}

	map->action_erase_events(action);
	map->action_add_event(action, event);
}

bool FEnhancedInput::is_bound(const Ref<InputEvent>& event) const {
	InputMap* map = get_input_map();
	if (map == nullptr) {
		return false;
	}

	const auto& all_actions = map->get_actions();
	return std::any_of(all_actions.begin(), all_actions.end(), [map, &event](const StringName& action) {
		if (action.begins_with("ui_")) {
			return false;
		}	 // ignore built-in actions

		const auto& events = map->action_get_events(action);
		return std::any_of(events.begin(), events.end(), [&event](const Ref<InputEvent>& e) { return e->is_match(event); });
	});
}

void FEnhancedInput::register_listener(const StringName& action, FInputComponent* input_component) {
	if (listeners.find(action) == listeners.end()) {
		listeners[action] = List<FInputComponent*>();
	}

	auto& list = listeners[action];
	if (std::any_of(list.begin(), list.end(), [&input_component](const FInputComponent* ic) {
				return input_component->get_instance_id() == ic->get_instance_id();
			})) {
		return;
	}

	list.push_back(input_component);
}

void FEnhancedInput::unregister_listener(const StringName& action, const FInputComponent* input_component) {
	if (listeners.find(action) == listeners.end()) {
		return;
	}

	auto& list = listeners[action];
	for (auto it = list.begin(); it != list.end(); ++it) {
		if ((*it)->get_instance_id() == input_component->get_instance_id()) {
			list.erase(*it);
			break;
		}
	}
}

FEnhancedInput* FEnhancedInput::get_singleton() {
	const Engine* engine = Engine::get_singleton();
	// Registered by initialize_fragile_module() and unregistered when the node
	// is torn down (the scene tree owns it at runtime). Check first: asking for
	// a missing singleton name logs an engine error.
	if (engine == nullptr || !engine->has_singleton("EnhancedInput")) {
		return nullptr;
	}

	return cast_to<FEnhancedInput>(engine->get_singleton("EnhancedInput"));
}

}	 // namespace fragile::framework
