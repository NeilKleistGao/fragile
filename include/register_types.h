#pragma once

#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_fragile_module(ModuleInitializationLevel p_level);
void uninitialize_fragile_module(ModuleInitializationLevel p_level);
