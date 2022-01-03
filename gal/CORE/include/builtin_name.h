#pragma once

#ifndef GAL_LANG_BUILTIN_NAME_HPP
#define GAL_LANG_BUILTIN_NAME_HPP

/**
 * @brief see compiler/operand_codes.hpp -> builtin_function
 */

#ifdef __cplusplus
extern "C"
{
#endif

inline const char* builtin_get_function_environment = "get_function_environment";
inline const char* builtin_set_function_environment = "set_function_environment";

inline const char* builtin_assert = "assert";

inline const char* builtin_type = "type";

inline const char* builtin_typeof = "typeof";

inline const char* builtin_raw_set = "raw_set";
inline const char* builtin_raw_get = "raw_get";
inline const char* builtin_raw_equal = "raw_equal";

inline const char* builtin_unpack = "unpack";

inline const char* builtin_math_prefix = "math";
inline const char* builtin_math_abs = "m_abs";
inline const char* builtin_math_acos = "m_acos";
inline const char* builtin_math_asin = "m_asin";
inline const char* builtin_math_atan2 = "m_atan2";
inline const char* builtin_math_atan = "m_atan";
inline const char* builtin_math_ceil = "m_ceil";
inline const char* builtin_math_cosh = "m_cosh";
inline const char* builtin_math_cos = "m_cos";
inline const char* builtin_math_clamp = "m_clamp";
inline const char* builtin_math_deg = "m_deg";
inline const char* builtin_math_exp = "m_exp";
inline const char* builtin_math_floor = "m_floor";
inline const char* builtin_math_fmod = "m_fmod";
inline const char* builtin_math_fexp = "m_fexp";
inline const char* builtin_math_ldexp = "m_ldexp";
inline const char* builtin_math_log10 = "m_log10";
inline const char* builtin_math_log = "m_log";
inline const char* builtin_math_max = "m_max";
inline const char* builtin_math_min = "m_min";
inline const char* builtin_math_modf = "m_modf";
inline const char* builtin_math_pow = "m_pow";
inline const char* builtin_math_rad = "m_rad";
inline const char* builtin_math_sign = "m_sign";
inline const char* builtin_math_sinh = "m_sinh";
inline const char* builtin_math_sin = "m_sin";
inline const char* builtin_math_sqrt = "m_sqrt";
inline const char* builtin_math_tanh = "m_tanh";
inline const char* builtin_math_tan = "m_tan";
inline const char* builtin_math_round = "m_round";

inline const char* builtin_bits_prefix = "bits";
inline const char* builtin_bits_arshift = "b_arshift";
inline const char* builtin_bits_and = "b_and";
inline const char* builtin_bits_or = "b_or";
inline const char* builtin_bits_xor = "b_xor";
inline const char* builtin_bits_not = "b_not";
inline const char* builtin_bits_test = "b_test";
inline const char* builtin_bits_extract = "b_extract";
inline const char* builtin_bits_lrotate = "b_lrotate";
inline const char* builtin_bits_lshift = "b_lshift";
inline const char* builtin_bits_replace = "b_replace";
inline const char* builtin_bits_rrotate = "b_rrotate";
inline const char* builtin_bits_rshift = "b_rshift";
inline const char* builtin_bits_countlz = "b_countlz";
inline const char* builtin_bits_countrz = "b_countrz";

inline const char* builtin_string_prefix = "string";
inline const char* builtin_string_byte = "s_byte";
inline const char* builtin_string_char = "s_char";
inline const char* builtin_string_len = "s_len";
inline const char* builtin_string_sub = "s_sub";

inline const char* builtin_table_prefix = "table";
inline const char* builtin_table_insert = "t_insert";
inline const char* builtin_table_unpack = "t_unpack";

inline const char* builtin_vector_prefix = "vector";
inline const char* builtin_vector_ctor = "v_new";

#ifdef __cplusplus
}
#endif

#endif // GAL_LANG_BUILTIN_NAME_HPP
