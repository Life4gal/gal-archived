#pragma once

#ifndef GAL_LANG_VM_TAGGED_METHOD_HPP
#define AL_LANG_VM_TAGGED_METHOD_HPP

#include <string_view>
#include <gal.hpp>

namespace gal::vm
{
	enum class tagged_method_type
	{
		index = 0,
		new_index,
		mode,
		named_call,

		equal,

		plus,
		minus,
		multiply,
		divide,
		pow,

		unary,

		less_than,
		less_equal,

		call,

		type,

		tagged_method_count
	};

	constexpr std::string_view gal_event_name[] =
	{
			{"$index"},
			{"$new_index"},
			{"$mode"},
			{"$named_call"},

			{"$equal"},

			{"$plus"},
			{"$minus"},
			{"$multiply"},
			{"$divide"},
			{"$pow"},

			{"$unary"},

			{"$less_than"},
			{"$less_equal"},

			{"$call"},

			{"$type"}
	};

	static_assert(std::size(gal_event_name) == static_cast<size_t>(tagged_method_type::tagged_method_count));

	// see gal.hpp -> object_type
	constexpr std::string_view gal_typename[] =
	{
			{"null"},
			{"boolean"},
			{"user_data"},
			{"number"},
			{"vector"},
			{"string"},
			{"table"},
			{"function"},
			{"user_data"},
			{"thread"}
	};

	static_assert(std::size(gal_typename) == static_cast<size_t>(object_type::tagged_value_count));
}

#endif // AL_LANG_VM_TAGGED_METHOD_HPP
