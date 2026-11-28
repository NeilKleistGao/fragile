#include "fragile/framework/input_component.h"

#include "fragile/framework/enhanced_input.h"
#include "fragile/utils/register_helper.h"

namespace fragile::framework {

void FInputComponent::_bind_methods() {
	ADD_METHOD(FInputComponent, bind_action, "action", "handler");
	ADD_METHOD(FInputComponent, unbind_action, "action");
	ADD_METHOD(FInputComponent, enable_input);
	ADD_METHOD(FInputComponent, disable_input);
	ADD_METHOD(FInputComponent, is_input_enabled);
}

void FInputComponent::_notification(int p_what) {
	if (p_what != NOTIFICATION_PREDELETE) {
		return;
	}

	// The singleton normally outlives scene nodes, but it is unregistered
	// before the final module teardown, so do not assume it is still there.
	FEnhancedInput* enhanced_input = FEnhancedInput::get_singleton();
	if (enhanced_input == nullptr) {
		return;
	}

	for (const KeyValue<StringName, Callable>& binding : bindings) {
		enhanced_input->unregister_listener(binding.key, this);
	}
	bindings.clear();
}

void FInputComponent::receive_action_data(const StringName& action, bool started, bool ongoing, bool completed) {
	if (!enabled || bindings.find(action) == bindings.end()) {
		return;
	}

	// Fire and forget: the handler has no return value in the contract.
	auto _ = bindings[action].call(started, ongoing, completed);
}

void FInputComponent::bind_action(const StringName& action, const Callable& handler) {
	bindings[action] = handler;

	if (FEnhancedInput* enhanced_input = FEnhancedInput::get_singleton(); enhanced_input != nullptr) {
		enhanced_input->register_listener(action, this);
	}
}

void FInputComponent::unbind_action(const StringName& action) {
	bindings.erase(action);

	if (FEnhancedInput* enhanced_input = FEnhancedInput::get_singleton(); enhanced_input != nullptr) {
		enhanced_input->unregister_listener(action, this);
	}
}

}	 // namespace fragile::framework
