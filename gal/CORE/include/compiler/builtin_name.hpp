#pragma once

#ifndef GAL_LANG_BUILTIN_NAME_HPP
#define GAL_LANG_BUILTIN_NAME_HPP

/**
 * @brief see compiler/operand_codes.hpp -> builtin_function
 */

#include <string_view>

namespace gal
{
	using builtin_name_type = std::string_view;

	constexpr builtin_name_type builtin_global_table_declaration{"global"};

	constexpr builtin_name_type builtin_get_function_environment{"get_function_environment"};
	constexpr builtin_name_type builtin_set_function_environment{"set_function_environment"};

	constexpr builtin_name_type builtin_assert{"assert"};

	constexpr builtin_name_type builtin_type{"type"};

	constexpr builtin_name_type builtin_typeof{"typeof"};

	constexpr builtin_name_type builtin_raw_set{"raw_set"};
	constexpr builtin_name_type builtin_raw_get{"raw_get"};
	constexpr builtin_name_type builtin_raw_equal{"raw_equal"};

	constexpr builtin_name_type builtin_unpack{"unpack"};

	constexpr builtin_name_type builtin_generator_ipairs{"l_ipairs"};
	constexpr builtin_name_type builtin_generator_pairs{"l_pairs"};
	constexpr builtin_name_type builtin_generator_next{"l_next"};

	constexpr builtin_name_type builtin_math_prefix{"math"};
	constexpr builtin_name_type builtin_math_abs{"m_abs"};
	constexpr builtin_name_type builtin_math_acos{"m_acos"};
	constexpr builtin_name_type builtin_math_asin{"m_asin"};
	constexpr builtin_name_type builtin_math_atan2{"m_atan2"};
	constexpr builtin_name_type builtin_math_atan{"m_atan"};
	constexpr builtin_name_type builtin_math_ceil{"m_ceil"};
	constexpr builtin_name_type builtin_math_cosh{"m_cosh"};
	constexpr builtin_name_type builtin_math_cos{"m_cos"};
	constexpr builtin_name_type builtin_math_clamp{"m_clamp"};
	constexpr builtin_name_type builtin_math_deg{"m_deg"};
	constexpr builtin_name_type builtin_math_exp{"m_exp"};
	constexpr builtin_name_type builtin_math_floor{"m_floor"};
	constexpr builtin_name_type builtin_math_fmod{"m_fmod"};
	constexpr builtin_name_type builtin_math_fexp{"m_fexp"};
	constexpr builtin_name_type builtin_math_ldexp{"m_ldexp"};
	constexpr builtin_name_type builtin_math_log10{"m_log10"};
	constexpr builtin_name_type builtin_math_log{"m_log"};
	constexpr builtin_name_type builtin_math_max{"m_max"};
	constexpr builtin_name_type builtin_math_min{"m_min"};
	constexpr builtin_name_type builtin_math_modf{"m_modf"};
	constexpr builtin_name_type builtin_math_pow{"m_pow"};
	constexpr builtin_name_type builtin_math_rad{"m_rad"};
	constexpr builtin_name_type builtin_math_sign{"m_sign"};
	constexpr builtin_name_type builtin_math_sinh{"m_sinh"};
	constexpr builtin_name_type builtin_math_sin{"m_sin"};
	constexpr builtin_name_type builtin_math_sqrt{"m_sqrt"};
	constexpr builtin_name_type builtin_math_tanh{"m_tanh"};
	constexpr builtin_name_type builtin_math_tan{"m_tan"};
	constexpr builtin_name_type builtin_math_round{"m_round"};

	constexpr builtin_name_type builtin_bits_prefix{"bits"};
	constexpr builtin_name_type builtin_bits_arshift{"b_arshift"};
	constexpr builtin_name_type builtin_bits_and{"b_and"};
	constexpr builtin_name_type builtin_bits_or{"b_or"};
	constexpr builtin_name_type builtin_bits_xor{"b_xor"};
	constexpr builtin_name_type builtin_bits_not{"b_not"};
	constexpr builtin_name_type builtin_bits_test{"b_test"};
	constexpr builtin_name_type builtin_bits_extract{"b_extract"};
	constexpr builtin_name_type builtin_bits_lrotate{"b_lrotate"};
	constexpr builtin_name_type builtin_bits_lshift{"b_lshift"};
	constexpr builtin_name_type builtin_bits_replace{"b_replace"};
	constexpr builtin_name_type builtin_bits_rrotate{"b_rrotate"};
	constexpr builtin_name_type builtin_bits_rshift{"b_rshift"};
	constexpr builtin_name_type builtin_bits_countlz{"b_countlz"};
	constexpr builtin_name_type builtin_bits_countrz{"b_countrz"};

	constexpr builtin_name_type builtin_string_prefix{"string"};
	constexpr builtin_name_type builtin_string_byte{"s_byte"};
	constexpr builtin_name_type builtin_string_char{"s_char"};
	constexpr builtin_name_type builtin_string_len{"s_len"};
	constexpr builtin_name_type builtin_string_sub{"s_sub"};

	constexpr builtin_name_type builtin_table_prefix{"table"};
	constexpr builtin_name_type builtin_table_insert{"t_insert"};
	constexpr builtin_name_type builtin_table_unpack{"t_unpack"};

	constexpr builtin_name_type builtin_vector_prefix{"vector"};
	constexpr builtin_name_type builtin_vector_ctor{"v_new"};
}

#endif // GAL_LANG_BUILTIN_NAME_HPP
