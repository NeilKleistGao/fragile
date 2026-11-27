#ifndef __FRAGILE_ENHANCED_INPUT_H__
#define __FRAGILE_ENHANCED_INPUT_H__

#include "info.h"
#include "input_component.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/list.hpp>

namespace fragile::framework {

/**
 * @brief Process-wide dispatcher that turns InputMap actions into callbacks.
 *
 * Registered as the "EnhancedInput" engine singleton when the module loads,
 * so it exists for the whole process and never has to be placed in a scene.
 * At runtime the module then parents it under the scene tree root (deferred,
 * because the tree does not exist yet during module initialization), which is
 * what lets it receive _unhandled_input() and _physics_process() callbacks.
 *
 * It listens to unhandled input events, remembers which actions are held, and
 * broadcasts started/ongoing/completed updates to the FInputComponent
 * listeners registered per action. It also maps input events back to the
 * project's actions for rebinding and "is this event already used?" checks.
 *
 * FEnhancedInput is registered as abstract: the only instance is the
 * singleton created by initialize_fragile_module().
 */
class FEnhancedInput : public FInfo {
	GDCLASS(FEnhancedInput, FInfo)

private:
	// Engine InputMap resolved in _ready().
	InputMap* input_map{ nullptr };

	/**
	 * @brief Listeners to notify per action, keyed by action name.
	 *
	 * Populated by FInputComponent::bind_action(); entries are removed by
	 * unbind_action() and when a component is torn down.
	 */
	HashMap<StringName, List<FInputComponent*>> listeners{};

	/**
	 * @brief Actions currently held, refreshed every physics frame.
	 *
	 * A held action produces no new input event, so it is re-broadcast as
	 * ongoing from here once per physics frame.
	 */
	HashSet<StringName> activated_actions{};

	/**
	 * @brief Sends one state update for @p action to every listener.
	 *
	 * @param action Action name to broadcast.
	 * @param started Whether the action was just pressed.
	 * @param ongoing Whether the action is being held this frame.
	 * @param completed Whether the action was just released.
	 */
	void broadcast_action(const StringName& action, bool started, bool ongoing, bool completed);

	/**
	 * @brief Returns the InputMap to use, resolving it lazily when needed.
	 *
	 * The cached pointer is only set by _ready(); editor tooling can call
	 * rebind()/is_bound() before that, so fall back to the engine singleton.
	 *
	 * @return The InputMap, or @c nullptr when the engine is not ready.
	 */
	[[nodiscard]] InputMap* get_input_map() const;

protected:
	static void _bind_methods();

public:
	/**
	 * @brief Parents the singleton under the scene tree root.
	 *
	 * Called deferred by initialize_fragile_module(): the SceneTree is created
	 * after MODULE_INITIALIZATION_LEVEL_SCENE, so it cannot be joined during
	 * module initialization. Runtime only - in the editor the dispatcher has
	 * nothing to do and would just consume editor input. Idempotent, so calling
	 * it again once attached is harmless.
	 */
	void attach_to_tree();

	void _notification(int p_what);

	void _ready() override;

	void _physics_process(double p_delta) override;

	void _unhandled_input(const Ref<InputEvent>& p_event) override;

	/**
	 * @brief Rebinds @p action to @p event.
	 *
	 * Does nothing when @p action already matches @p event; otherwise the
	 * action's previous events are replaced by @p event.
	 *
	 * @param action Action to rebind.
	 * @param event Input event that should trigger the action.
	 */
	void rebind(const StringName& action, const Ref<InputEvent>& event) const;

	/**
	 * @brief Checks whether @p event already triggers a project action.
	 *
	 * Built-in "ui_*" actions are ignored so UI navigation does not count as
	 * a gameplay binding.
	 *
	 * @param event Input event to look up.
	 * @return @c true when any project action matches @p event.
	 */
	[[nodiscard]] bool is_bound(const Ref<InputEvent>& event) const;

	/**
	 * @brief Registers @p input_component as a listener of @p action.
	 *
	 * Registering the same component for the same action twice is ignored.
	 *
	 * @param action Action to listen to.
	 * @param input_component Component to notify on updates; never owned here.
	 */
	void register_listener(const StringName& action, FInputComponent* input_component);

	/**
	 * @brief Removes @p input_component from the listeners of @p action.
	 *
	 * A no-op when the component is not registered for that action.
	 *
	 * @param action Action to stop listening to.
	 * @param input_component Component to remove; only its instance ID is used.
	 */
	void unregister_listener(const StringName& action, const FInputComponent* input_component);

	/**
	 * @brief Returns the "EnhancedInput" singleton.
	 *
	 * @return The singleton, or @c nullptr when the module is not initialized
	 *         or the singleton has already been torn down.
	 */
	static FEnhancedInput* get_singleton();
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_ENHANCED_INPUT_H__
