#ifndef __FRAGILE_ENHANCED_INPUT_H__
#define __FRAGILE_ENHANCED_INPUT_H__

#include "info.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_map.hpp>

namespace fragile::framework {

class FEnhancedInput : public FInfo {
	GDCLASS(FEnhancedInput, FInfo)

private:
	InputMap* input_map{ nullptr };

protected:
	static void _bind_methods();

public:
	void _ready() override;

	void rebind(const StringName& action, const Ref<InputEvent>& event) const;

	[[nodiscard]] bool is_bound(const Ref<InputEvent>& event) const;
};

}	 // namespace fragile::framework

#endif	// __FRAGILE_ENHANCED_INPUT_H__
