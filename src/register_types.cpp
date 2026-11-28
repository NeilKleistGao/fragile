#include "register_types.h"

#include "fragile/fragile.h"
#include "godot_cpp/classes/engine.hpp"

#include <gdextension_interface.h>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_fragile_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#if GODOT_VERSION_MAJOR != FRAGILE_EXPECTED_GODOT_MAJOR_VERSION || GODOT_VERSION_MINOR != FRAGILE_EXPECTED_GODOT_MINOR_VERSION
	WARN_PRINT_ONCE_ED("This Godot version may not be fully supported.")
#endif

	ClassDB::register_class<fragile::framework::FInfo>();
	ClassDB::register_class<fragile::framework::FInputComponent>();
	ClassDB::register_class<fragile::framework::FGameMode>();
	ClassDB::register_class<fragile::framework::FActor>();
	ClassDB::register_class<fragile::framework::FLevel>();
	ClassDB::register_class<fragile::framework::FWorld>();
	ClassDB::register_class<fragile::framework::FPawn>();
	ClassDB::register_class<fragile::framework::FCharacter>();
	ClassDB::register_abstract_class<fragile::framework::FController>();
	ClassDB::register_class<fragile::framework::FPlayerController>();
	ClassDB::register_abstract_class<fragile::framework::FEnhancedInput>();
	ClassDB::register_class<fragile::framework::FSpawnPoint>();

	auto* enhanced_input = memnew(fragile::framework::FEnhancedInput);
	Engine::get_singleton()->register_singleton("EnhancedInput", enhanced_input);
	// Engine singletons have no scene placement, but a Node needs to be inside
	// the tree to receive input callbacks. The SceneTree does not exist yet at
	// this initialization level, so join it from the first deferred message
	// (attach_to_tree() is idempotent and skips the editor).
	enhanced_input->call_deferred("attach_to_tree");
}

void uninitialize_fragile_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine* engine = Engine::get_singleton();
	if (engine != nullptr && engine->has_singleton("EnhancedInput")) {
		auto* enhanced_input = Object::cast_to<fragile::framework::FEnhancedInput>(engine->get_singleton("EnhancedInput"));
		engine->unregister_singleton("EnhancedInput");
		// The engine does not own singletons: free it, otherwise ObjectDB
		// reports a leaked instance at exit. At runtime the scene tree has
		// already freed it and dropped the registration, so this is a no-op.
		if (enhanced_input != nullptr) {
			memdelete(enhanced_input);
		}
	}
}

extern "C" {

// Initialization.
GDExtensionBool GDE_EXPORT fragile_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_fragile_module);
	init_obj.register_terminator(uninitialize_fragile_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
