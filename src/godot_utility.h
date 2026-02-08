#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#define PRINT_ERROR(m_msg) \
	print_error(godot::String("[") + get_class() + "::" + __func__ + "] " + m_msg)
