#pragma once

#ifndef GAL_LANG_LANGUAGE_AST_NODE_HPP
#define GAL_LANG_LANGUAGE_AST_NODE_HPP

#include <unordered_set>
#include <utils/hash.hpp>
#include <utils/point.hpp>
#include <gal/foundation/dispatcher.hpp>

namespace gal::lang
{
	namespace lang
	{
		struct name_validator
		{
			[[nodiscard]] constexpr static auto hash_name(const foundation::string_view_type name) noexcept { return utils::hash_fnv1a<false>(name); }

			[[nodiscard]] static bool is_reserved_name(const foundation::string_view_type name) noexcept
			{
				const static std::unordered_set names{
						hash_name(keyword_define_name::value),
						hash_name(keyword_class_name::value),
						hash_name(keyword_variable_declare_name::value),
						hash_name(keyword_true_name::value),
						hash_name(keyword_false_name::value),
						hash_name(keyword_global_name::value),
						hash_name(keyword_and_name::value),
						hash_name(keyword_or_name::value),
						hash_name(keyword_if_name::value),
						hash_name(keyword_else_name::value),
						hash_name(keyword_for_in_name::subtype<0>::value),
						hash_name(keyword_for_in_name::subtype<1>::value),
						hash_name(keyword_while_name::value),
						hash_name(keyword_continue_break_name::subtype<0>::value),
						hash_name(keyword_continue_break_name::subtype<1>::value),
						hash_name(keyword_continue_break_name::subtype<0>::value),
						hash_name(keyword_match_case_default_name::subtype<1>::value),
						hash_name(keyword_match_case_default_name::subtype<2>::value),
						hash_name(keyword_function_argument_placeholder_name::value),
						hash_name(keyword_try_catch_finally_name::subtype<0>::value),
						hash_name(keyword_try_catch_finally_name::subtype<1>::value),
						hash_name(keyword_try_catch_finally_name::subtype<2>::value),
						hash_name(keyword_function_guard_name::value),
						hash_name(keyword_inline_range_gen_name::value),
						hash_name(keyword_operator_declare_name::value),
						hash_name(keyword_number_inf_nan_name::subtype<0>::value),
						hash_name(keyword_number_inf_nan_name::subtype<1>::value)};

				return names.contains(hash_name(name));
			}
		};


	}
}

#endif // GAL_LANG_LANGUAGE_AST_NODE_HPP
