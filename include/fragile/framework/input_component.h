#ifndef __FRAGILE_INPUT_COMPONENT_H__
#define __FRAGILE_INPUT_COMPONENT_H__

#include "info.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace fragile::framework {

/**
 * @brief Input listener component attached to a possessed pawn.
 *
 * Mirrors Unreal Engine's UInputComponent: a lightweight node that turns
 * FEnhancedInput actions into callables. It does not read input itself; it
 * registers with the FEnhancedInput singleton per bound action and is called
 * back by it when the action starts, is held, or completes.
 *
 * Lifetime belongs to the controller: FPlayerController creates one and
 * parents it to the pawn it possesses. The component unregisters itself from
 * FEnhancedInput when it is torn down, so it can be freed while actions are
 * still bound.
 */
class FInputComponent : public FInfo {
	GDCLASS(FInputComponent, FInfo)

private:
	/**
	 * @brief Handlers bound to actions, keyed by action name.
	 *
	 * The keys are mirrored in FEnhancedInput's listener table; the table
	 * entries are removed by unbind_action() and by the NOTIFICATION_PREDELETE
	 * handler.
	 */
	HashMap<StringName, Callable> bindings{};

	bool enabled{ true };

protected:
	static void _bind_methods();

public:
	void _notification(int p_what);

	/**
	 * @brief Delivers one action state update to the handler of @p action.
	 *
	 * Called by FEnhancedInput for every registered component; not meant to
	 * be called by user code. Updates for actions without a bound handler are
	 * ignored.
	 *
	 * @param action Action name.
	 * @param started Whether the action was just pressed.
	 * @param ongoing Whether the action is being held this frame.
	 * @param completed Whether the action was just released.
	 */
	void receive_action_data(const StringName& action, bool started, bool ongoing, bool completed);

	/**
	 * @brief Binds @p handler to @p action and registers this component.
	 *
	 * FEnhancedInput registers the component once per action, so rebinding an
	 * action only replaces its handler. Binding is a no-op registration when
	 * FEnhancedInput does not exist (for example outside a running project),
	 * but the handler is always stored.
	 *
	 * @param action Action whose state updates should reach @p handler.
	 * @param handler Callable receiving (started, ongoing, completed).
	 */
	void bind_action(const StringName& action, const Callable& handler);

	/**
	 * @brief Removes the handler bound to @p action, if any.
	 *
	 * Also removes this component from FEnhancedInput's listener table for
	 * that action. Unbinding an action that has no handler is a no-op.
	 *
	 * @param action Action to unbind.
	 */
	void unbind_action(const StringName& action);

	void enable_input() { enabled = true; }
	void disable_input() { enabled = false; }
	[[nodiscard]] bool is_input_enabled() const { return enabled; }
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_INPUT_COMPONENT_H__
