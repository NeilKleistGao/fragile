#include "fragile/framework/enhanced_input.h"

#include "fragile/utils/editor.h"
#include "fragile/utils/register_helper.h"

#include <algorithm>

namespace fragile::framework {

void FEnhancedInput::_bind_methods() {
	ADD_METHOD(FEnhancedInput, rebind, "action", "event");
	ADD_METHOD(FEnhancedInput, is_bound, "event");
}

void FEnhancedInput::_ready() {
	FInfo::_ready();

	FRAGILE_RUNTIME_API;

	input_map = InputMap::get_singleton();
}

void FEnhancedInput::rebind(const StringName& action, const Ref<InputEvent>& event) const {
	if (input_map->action_has_event(action, event)) {
		return;
	}

	input_map->action_erase_events(action);
	input_map->action_add_event(action, event);
}

bool FEnhancedInput::is_bound(const Ref<InputEvent>& event) const {
	const auto& all_actions = input_map->get_actions();
	return std::any_of(all_actions.begin(), all_actions.end(), [&](const StringName& action) {
		if (action.begins_with("ui_")) {
			return false;
		}	 // ignore built-in actions

		const auto& events = input_map->action_get_events(action);
		return std::any_of(events.begin(), events.end(), [&](const Ref<InputEvent>& e) { return e->is_match(event); });
	});
}

}	 // namespace fragile::framework
