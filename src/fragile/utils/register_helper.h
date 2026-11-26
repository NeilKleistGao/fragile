
#ifndef __FRAGILE_REGISTER_HELPER_H__
#define __FRAGILE_REGISTER_HELPER_H__

#include <godot_cpp/core/class_db.hpp>

#define CONCAT(__X__, __Y__) __X__##__Y__
#define STR(__X__) #__X__

#define ADD_PREFAB_PROPERTY(__CLASS__, __NAME__)                                                                                                                        \
	godot::ClassDB::bind_method(godot::D_METHOD(STR(set_##__NAME__), "prefab"), &__CLASS__::CONCAT(set_, __NAME__));                                                      \
	godot::ClassDB::bind_method(godot::D_METHOD(STR(get_##__NAME__)), &__CLASS__::CONCAT(get_, __NAME__));                                                                \
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, STR(__NAME__), godot::PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), STR(set_##__NAME__), STR(get_##__NAME__))

// ##__VA_ARGS__ (a GNU extension honored by GCC, Clang and MSVC) drops the
// comma when no argument names are passed, so a zero-argument method such as
// ADD_METHOD(FPawn, get_controller) expands to D_METHOD("get_controller")
// instead of the ill-formed D_METHOD("get_controller", ) outside MSVC's
// traditional preprocessor.
#define ADD_METHOD(__CLASS__, __NAME__, ...)                                                       \
	godot::ClassDB::bind_method(godot::D_METHOD(STR(__NAME__), ##__VA_ARGS__), &__CLASS__::__NAME__)

#define ADD_STATIC_METHOD(__CLASS__, __NAME__, ...)                                                                       \
	godot::ClassDB::bind_static_method(STR(__CLASS__), godot::D_METHOD(STR(__NAME__), ##__VA_ARGS__), &__CLASS__::__NAME__)

#define ADD_SETTER_GETTER(__CLASS__, __NAME__, ...)                                                                     \
	godot::ClassDB::bind_method(godot::D_METHOD(STR(set_##__NAME__), ##__VA_ARGS__), &__CLASS__::CONCAT(set_, __NAME__)); \
	godot::ClassDB::bind_method(godot::D_METHOD(STR(get_##__NAME__)), &__CLASS__::CONCAT(get_, __NAME__))

#endif	// __FRAGILE_REGISTER_HELPER_H__
