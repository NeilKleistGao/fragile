#ifndef __FRAGILE_H__
#define __FRAGILE_H__

#define FRAGILE_MAJOR_VERSION 0
#define FRAGILE_MINOR_VERSION 1
#define FRAGILE_PATCH_VERSION 0

#if FRAGILE_MAJOR_VERSION == 0 && FRAGILE_MINOR_VERSION == 1
#define FRAGILE_EXPECTED_GODOT_MAJOR_VERSION 4
#define FRAGILE_EXPECTED_GODOT_MINOR_VERSION 7
#else
#define FRAGILE_EXPECTED_GODOT_MAJOR_VERSION 4
#define FRAGILE_EXPECTED_GODOT_MINOR_VERSION 7
#endif

#include "fragile/framework/actor.h"
#include "fragile/framework/character.h"
#include "fragile/framework/controller.h"
#include "fragile/framework/enhanced_input.h"
#include "fragile/framework/game_mode.h"
#include "fragile/framework/info.h"
#include "fragile/framework/input_component.h"
#include "fragile/framework/level.h"
#include "fragile/framework/pawn.h"
#include "fragile/framework/player_controller.h"
#include "fragile/framework/world.h"

#endif	// __FRAGILE_H__
