#pragma once

#ifndef GAL_LANG_COMPILER_OPERAND_CODES_HPP
#define GAL_LANG_COMPILER_OPERAND_CODES_HPP

#include<cstdint>
#include <string_view>
#include <utils/assert.hpp>
#include <utils/enum_utils.hpp>
#include <utils/macro.hpp>

namespace gal::compiler
{
	/**
	 * @note about byte-code:
	 *
	 *	byte-code definitions:
	 *		byte-code definitions arg using "word code" - each instruction is one or many 32-bit words.
	 *
	 *		The first word in the instruction is always the instruction header, and *must* contain the *operand* (enum below) in the least significant byte.
	 *
	 *		Instruction word can be encoded using one of the following encodings:
	 *			ABC - least-significant byte for the operand, followed by three *bytes*, A, B and C; each byte declares a register index, small index into some other table or an *unsigned integral* value
	 *			AD - least-significant byte for the operand, followed by A byte, followed by D half-word (*16-bit* integer). D is a *signed integer* that commonly specifies constant table index or jump offset
	 *			E - least-significant byte for the operand, followed by E (*24-bit* integer). E is a *signed integer* that commonly specifies a jump offset
	 *			AUX - one extra word, this is just a *32-bit* word and is decoded according to the specification for each operand
	 *
	 *			For each operand the encoding is *static* - that is, based on the operand you know a-priory how large the instruction is, with the exception of operand::new_closure
	 *
	 * byte-code indices:
	 *		byte-code instructions commonly refer to integer values that define offsets or indices for various entities. For each type, there's a maximum encode-able value.
	 *
	 *		Note that in some cases, the compiler will set a lower limit than the maximum encode-able value is to prevent fragile code into bumping against the limits whenever we change the compilation details.
	 *		Additionally, in some specific instructions such as operand::logical_and_key, the limit on the encoded value is smaller; this means that if a value is larger, a different instruction must be selected.
	 *
	 * Registers: 0-254. Registers refer to the values on the function's stack frame, including arguments.
	 * Upvalues: 0-254. Upvalues refer to the values stored in the closure object.
	 * Constants: 0-2^23-1. Constants are stored in a table allocated with each proto; to allow for future byte-code tweaks the encode-able value is limited to 23 bits.
	 * Closures: 0-2^15-1. Closures are created from child protos via a child index; the limit is for the number of closures immediately referenced in each function.
	 * Jumps: -2^23~2^23. Jump offsets are specified in word increments, so jumping over an instruction may sometimes require an offset of 2 or more.
	 */

	using operand_underlying_type = std::uint32_t;
	using operand_abc_underlying_type = std::uint8_t;
	using operand_d_underlying_type = std::int16_t;
	using operand_e_underlying_type = std::int32_t;

	enum class operands : operand_underlying_type
	{
		operand_sentinel_begin = 0,

		nop = 0,

		debugger_break,

		// set register to null
		// A: target register
		// B: none
		// C: none
		load_null,

		// sets register to boolean and jumps to a given short offset (used to compile comparison results into a boolean)
		// A: target register
		// B: value (false-0 / true-any other)
		// C: jump offset
		load_boolean,

		// sets register to a number literal
		// A: target register
		// D: value (-32768~32767: signed 16 bits)
		load_number,

		// sets register to an entry from the constant table from the proto (number/string)
		// A: target register
		// D: constant table index (0~32767: signed 16 bits, positive only)
		load_key,

		// copy value from one register to another one
		// A: target register
		// B: source register
		// C: none
		move,

		// load value from global table using constant string as a key
		// A: target register
		// B: none
		// C: predicted slot index (based on hash)
		// AUX: constant table index
		load_global,

		// set value in global table using constant string as a key
		// A: source register
		// B: none
		// C: predicted slot index (based on hash)
		// AUX: constant table index
		set_global,

		// load upvalue from the upvalue table for the current function
		// A: target register
		// B: upvalue index (0~255: unsigned 8 bits)
		// C: none
		load_upvalue,

		// store value into the upvalue table for the current function
		// A: target register
		// B: upvalue index (0~255: unsigned 8 bits)
		// C: none
		set_upvalue,

		// close (migrate to heap) all upvalues that were captured for registers >= target
		// A: target register
		// B: none
		// C: none
		close_upvalues,

		// load imported global table global from the constant table
		// A: target register
		// D: constant table index (0~32767: signed 16 bits, positive only); we assume that imports are loaded into the constant table
		// AUX: (2 + 3 * 10) bits, top 2 bits is the length of the path(1,2,3), three 10-bit indices of constant strings that, combined, constitute an import path
		load_import,

		// load value from table into target register using key from register
		// A: target register
		// B: table register
		// C: index register
		load_table,

		// store source register into table using key from register
		// A: source register
		// B: table register
		// C: index register
		set_table,

		// load value from table into target register using constant string as a key
		// A: target register
		// B: table register
		// C: predicted slot index (based on hash)
		// AUX: constant table index
		load_table_string_key,

		// store source register into table using constant string as a key
		// A: source register
		// B: table register
		// C: predicted slot index (based on hash)
		// AUX: constant table index
		set_table_string_key,

		// load value from table into target register using small integer index as a key
		// A: target register
		// B: table register
		// C: index-1 (index is 1~256: unsigned 8 bits)
		load_table_number_key,

		// store source register into table using small integer index as a key
		// A: source register
		// B: table register
		// C: index-1 (index is 1~256: unsigned 8 bits)
		set_table_number_key,

		// create closure from a child proto; followed by a *CAPTURE* instruction for each upvalue
		// A: target register
		// D: child proto index (0~32767: signed 16 bits, positive only)
		new_closure,

		// prepare to call specified method by name by loading function from source register using constant index into target register and copying source register into target register + 1
		// A: target register
		// B: source register
		// C: predicted slot index (based on hash)
		// AUX: constant table index
		// Note that this instruction must be followed directly by *CALL*; it prepares the arguments
		// This instruction is roughly equivalent to (load_table_string_key + move) pair, but we need a special instruction to support custom __named_call meta method
		named_call,

		// call specified function
		// A: register where the function object lives, followed by arguments; results are placed starting from the same register
		// B: argument count + 1, or 0 to preserve all arguments up to top (multiple return)
		// C: result count + 1, or 0 to preserve all values and adjust top (multiple return)
		call,

		// returns specified values from the function
		// A: register where the returned values start
		// B: number of returned values + 1, or 0 to return all values up to top (multiple return)
		// C: none
		call_return,

		// jumps to target offset
		// A: none
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		jump,

		// jumps to target offset; this is equivalent to *JUMP* but is used as a safe point to be able to interrupt while/repeat loops
		// A: none
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		jump_back,

		// jumps to target offset if register is not null/false
		// A: none
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		jump_if,

		// jumps to target offset if register is null/false
		// A: none
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		jump_if_not,

		// jumps to target offset if the comparison is true (or false, for *NOT* variants)
		// A: source register 1
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		// AUX: source register 2
		jump_if_equal,
		jump_if_less_equal,
		jump_if_less_than,
		jump_if_not_equal,
		jump_if_not_less_equal,
		jump_if_not_less_than,

		// compute arithmetic operation between two source registers and put the result into target register
		// A: target register
		// B: source register 1
		// C: source register 2
		plus,
		minus,
		multiply,
		divide,
		modulus,
		pow,
		// todo: builtin bit operands
		bitwise_and,
		bitwise_or,
		bitwise_xor,
		bitwise_left_shift,
		bitwise_right_shift,

		// compute arithmetic operation between the source register and a constant and put the result into target register
		// A: target register
		// B: source register
		// C: constant table index (0~255: unsigned 8 bits)
		plus_key,
		minus_key,
		multiply_key,
		divide_key,
		modulus_key,
		pow_key,
		// todo: builtin bit operands
		bitwise_and_key,
		bitwise_or_key,
		bitwise_xor_key,
		bitwise_left_shift_key,
		bitwise_right_shift_key,

		// perform `and` or `or` operation (selecting first or second register based on whether the first one is truth) and put the result into target register
		// A: target register
		// B: source register 1
		// C: source register 2
		logical_and,
		logical_or,

		// perform `and` or `or` operation (selecting source register or constant based on whether the source register is truth) and put the result into target register
		// A: target register
		// B: source register
		// C: constant table index (0~255: unsigned 8 bits)
		logical_and_key,
		logical_or_key,

		// compute unary operation for source register and put the result into target register
		// A: target register
		// B: source register
		// C: none
		// todo: builtin bit operands
		unary_plus,
		unary_minus,
		unary_not,
		unary_bitwise_not,

		// create table in target register
		// A: target register
		// B: table size, stored as 0 for v = 0 and ceil(log2(v))+1 for v != 0
		// C: none
		// AUX: array size
		new_table,

		// copy table using the constant table template to target register
		// A: target register
		// D: constant table index (0~32767: signed 16 bits, positive only)
		copy_table,

		// set a list of values to table in target register
		// A: target register
		// B: source register start
		// C: value count + 1, or 0 to use all values up to top (multiple return)
		// AUX: table index to start from
		set_list,

		// prepare a numeric for loop, jump over the loop if first iteration doesn't need to run
		// A: target register; numeric for loops assume a register layout [limit, step, index, variable]
		// D: jump offset (-32768~32767: signed 16 bits)
		// Note that limit/step are immutable, index isn't visible to user code since it's copied into variable
		for_numeric_loop_prepare,

		// adjust loop variables for one iteration, jump back to the loop header if loop needs to continue
		// A: target register; see for_numeric_loop_prepare for register layout
		// D: jump offset (-32768~32767: signed 16 bits)
		for_numeric_loop,

		// A: target register; generic for loops assume a register layout [generator, state, index, variables...]
		// D: jump offset (-32768~32767: signed 16 bits)
		// AUX: variable count (1..255: positive 8 bits only)
		// Note that loop variables are adjusted by calling generator(state, index) and expecting it to return a tuple that's copied to the user variables
		for_generic_loop,

		// for_generic_loop with 2 output variables (no AUX encoding), assuming generator is bytecode_inext
		// for_generic_loop_prepare_inext prepares the index variable and jumps to for_generic_loop_inext
		// for_generic_loop_inext has identical encoding and semantics to for_generic_loop (except for AUX encoding)
		for_generic_loop_prepare_inext,
		for_generic_loop_inext,

		// for_generic_loop with 2 output variables (no AUX encoding), assuming generator is bytecode_next
		// for_generic_loop_prepare_next prepares the index variable and jumps to for_generic_loop_next
		// for_generic_loop_next has identical encoding and semantics to for_generic_loop (except for AUX encoding)
		for_generic_loop_prepare_next,
		for_generic_loop_next,

		// copy variables into the target register from vararg storage for current function
		// A: target register
		// B: variable count + 1, or 0 to copy all variables and adjust top (multiple return)
		// C: none
		load_varargs,

		// copy closure from a pre-created function object (reusing it unless environments diverge)
		// A: target register
		// D: constant table index (0~32767: signed 16 bits, positive only)
		copy_closure,

		// prepare stack for variadic functions so that load_varargs works correctly
		// A: number of fixed arguments
		// D: none
		prepare_varargs,

		// sets register to an entry from the constant table from the proto (number/string)
		// A: target register
		// D: none
		// AUX: constant table index
		load_key_extra,

		// jumps to the target offset; like *JUMP_BACK*, supports interruption
		// E: jump offset (-2^23~2^23: signed 24 bits, 0 means nop, aka do not jump)
		jump_extra,

		// perform a fast call of a built-in function
		// A: builtin function id (see builtin_function)
		// B: none
		// C: jump offset to get to following *CALL*
		// Note that *FASTCALL* is followed by one of (*LOAD_IMPORT*, *MOVE*, *LOAD_UPVALUE*) instructions and by *CALL* instruction
		// This is necessary so that if FASTCALL can't perform the call inline, it can continue normal execution
		// If FASTCALL *can* perform the call, it jumps over the instructions *and* over the next *CALL*
		// Note that *FASTCALL* will read the actual call arguments, such as argument/result registers and counts, from the *CALL* instruction
		fastcall,

		// update coverage information stored in the instruction
		// E: hit count for the instruction (0..2^23-1: signed 24 bits, positive only)
		// Not that the hit count is incremented by VM every time the instruction is executed, and saturates at 2^23-1
		coverage,

		// capture a local or an upvalue as an upvalue into a newly created closure; only valid after *NEW_CLOSURE*
		// A: capture type, see capture_type
		// B: source register (for value/reference) or upvalue index (for upvalue/upreference)
		// C: none
		capture,

		// jumps to target offset if the comparison with constant is true (or false, for NOT variants)
		// A: source register 1
		// D: jump offset (-32768~32767: signed 16 bits, 0 means nop, aka do not jump)
		// AUX: constant table index
		jump_if_equal_key,
		jump_if_not_equal_key,

		// perform a fast call of a built-in function using 1 register argument
		// A: builtin function id (see builtin_function)
		// B: source argument register
		// C: jump offset to get to following *CALL*
		fastcall_1,

		// perform a fast call of a built-in function using 2 register arguments
		// A: builtin function id (see builtin_function)
		// B: source argument register
		// C: jump offset to get to following *CALL*
		// AUX: source register 2 in least-significant byte
		fastcall_2,

		// perform a fast call of a built-in function using 1 register argument and 1 constant argument
		// A: builtin function id (see builtin_function)
		// B: source argument register
		// C: jump offset to get to following *CALL*
		// AUX: constant index
		fastcall_2_key,

		// let us know how many operands there are
		operand_sentinel_size,
		operand_sentinel_end = operand_sentinel_size
	};

	constexpr static operand_underlying_type max_operands_size = 0xff;
	static_assert(static_cast<operand_underlying_type>(operands::operand_sentinel_size) < max_operands_size);

	/**
	 * @brief byte-code tags, used internally for byte-code encoded as a string
	 */
	enum class bytecode_tag : operand_abc_underlying_type
	{
		// byte-code version
		version = 0,
		// types of constant table entries
		null,
		boolean,
		number,
		string,
		import,
		table,
		closure
	};

	/**
	 * @brief Builtin function ids, used in operand::fastcall
	 */
	enum class builtin_function : operand_abc_underlying_type
	{
		none = 0,

		// assert
		assert,

		// math
		math_abs,
		math_acos,
		math_asin,
		math_atan2,
		math_atan,
		math_ceil,
		math_cosh,
		math_cos,
		math_clamp,
		math_deg,
		math_exp,
		math_floor,
		math_fmod,
		math_fexp,
		math_ldexp,
		math_log10,
		math_log,
		math_max,
		math_min,
		math_modf,
		math_pow,
		math_rad,
		math_sign,
		math_sinh,
		math_sin,
		math_sqrt,
		math_tanh,
		math_tan,
		math_round,

		// bits
		bits_arshift,
		bits_and,
		bits_not,
		bits_or,
		bits_xor,
		bits_test,
		bits_extract,
		bits_lrotate,
		bits_lshift,
		bits_replace,
		bits_rrotate,
		bits_rshift,
		bits_countlz,
		bits_countrz,

		// typeof()
		typeof,

		// string.
		string_sub,

		// raw*
		raw_set,
		raw_get,
		raw_equal,

		// table
		table_insert,
		table_unpack,

		// vector ctor
		vector
	};

	enum class capture_type : operand_abc_underlying_type
	{
		value = 0,
		reference,
		upvalue
	};

	/**
	 * @brief byte-code instruction header: it's always a 32-bit integer, with low byte (first byte in little-endian) containing the operand
	 *
	 * @note Some instruction types require more data and have more 32-bit integers following the header
	 */
	template<bool Underlying = true>
	constexpr auto instruction_to_operand(const operand_underlying_type instruction) noexcept
	{
		if constexpr (Underlying) { return instruction & max_operands_size; }
		else { return static_cast<operands>(instruction & max_operands_size); }
	}

	/**
	 * @brief ABC encoding: three 8-bit values, containing registers or small numbers
	 */
	constexpr auto instruction_to_a(const operand_underlying_type instruction) noexcept { return static_cast<operand_abc_underlying_type>((instruction >> 8) & max_operands_size); }

	/**
	 * @brief ABC encoding: three 8-bit values, containing registers or small numbers
	 */
	constexpr auto instruction_to_b(const operand_underlying_type instruction) noexcept { return static_cast<operand_abc_underlying_type>((instruction >> 16) & max_operands_size); }

	/**
	 * @brief ABC encoding: three 8-bit values, containing registers or small numbers
	 */
	constexpr auto instruction_to_c(const operand_underlying_type instruction) noexcept { return static_cast<operand_abc_underlying_type>((instruction >> 24) & max_operands_size); }

	/**
	 * @brief AD encoding: one 8-bit value, one signed 16-bit value
	 */
	constexpr auto instruction_to_d(const operand_underlying_type instruction) noexcept { return static_cast<operand_d_underlying_type>(instruction >> 16); }

	/**
	 * @brief E encoding: one signed 24-bit value
	 */
	constexpr auto instruction_to_e(const std::uint32_t instruction) noexcept { return static_cast<operand_e_underlying_type>(instruction >> 8); }

	GAL_ASSERT_CONSTEXPR capture_type instruction_to_capture_type(const operand_abc_underlying_type operand) noexcept
	{
		gal_assert(
				operand == static_cast<operand_abc_underlying_type>(capture_type::value) ||
				operand == static_cast<operand_abc_underlying_type>(capture_type::reference) ||
				operand == static_cast<operand_abc_underlying_type>(capture_type::upvalue));
		return static_cast<capture_type>(operand);
	}

	/**
	 * @see operands::capture
	 */
	GAL_ASSERT_CONSTEXPR bool is_valid_capture_operand(const operand_underlying_type operand) noexcept
	{
		gal_assert(instruction_to_operand<false>(operand) == operands::capture);
		if (const auto type = instruction_to_a(operand);
			type == static_cast<operand_abc_underlying_type>(capture_type::value) ||
			type == static_cast<operand_abc_underlying_type>(capture_type::upvalue)) { return true; }
		return false;
	}

	template<typename T>
		requires(std::is_convertible_v<T, operand_underlying_type> && not std::is_convertible_v<T, operands>)
	constexpr std::size_t get_operand_length(const T operand) noexcept { return get_operand_length(static_cast<operands>(static_cast<operand_underlying_type>(operand))); }

	template<typename T>
		requires std::is_convertible_v<T, operands>
	constexpr std::size_t get_operand_length(const T operand) noexcept
	{
		switch (static_cast<operands>(operand))// NOLINT(clang-diagnostic-switch-enum)
		{
				using enum operands;
			case load_global:
			case set_global:

			case load_import:

			case load_table_string_key:
			case set_table_string_key:

			case named_call:

			case jump_if_equal:
			case jump_if_less_than:
			case jump_if_less_equal:
			case jump_if_not_equal:
			case jump_if_not_less_than:
			case jump_if_not_less_equal:

			case copy_table:

			case set_list:

			case for_generic_loop:

			case load_key_extra:

			case jump_if_equal_key:
			case jump_if_not_equal_key:

			case fastcall_2:
			case fastcall_2_key: { return 2; }
			default: { return 1; }
		}
	}

	using operand_name_type = std::string_view;

	constexpr operand_name_type get_operands_name(const operands operand) noexcept
	{
		switch (operand)// NOLINT(clang-diagnostic-switch-enum)
		{
				using enum operands;
			case debugger_break: { return "debugger_break"; }
			case load_null: { return "load_null"; }
			case load_boolean: { return "load_boolean"; }
			case load_number: { return "load_number"; }
			case load_key: { return "load_key"; }
			case move: { return "move"; }
			case load_global: { return "load_global"; }
			case set_global: { return "set_global"; }
			case load_upvalue: { return "load_upvalue"; }
			case set_upvalue: { return "set_upvalue"; }
			case close_upvalues: { return "close_upvalues"; }
			case load_import: { return "load_import"; }
			case load_table: { return "load_table"; }
			case set_table: { return "set_table"; }
			case load_table_string_key: { return "load_table_string_key"; }
			case set_table_string_key: { return "set_table_string_key"; }
			case load_table_number_key: { return "load_table_number_key"; }
			case set_table_number_key: { return "set_table_number_key"; }
			case new_closure: { return "new_closure"; }
			case named_call: { return "named_call"; }
			case call: { return "call"; }
			case call_return: { return "call_return"; }
			case jump: { return "jump"; }
			case jump_back: { return "jump_back"; }
			case jump_if: { return "jump_if"; }
			case jump_if_not: { return "jump_if_not"; }
			case jump_if_equal: { return "jump_if_equal"; }
			case jump_if_less_equal: { return "jump_if_less_equal"; }
			case jump_if_less_than: { return "jump_if_less_than"; }
			case jump_if_not_equal: { return "jump_if_not_equal"; }
			case jump_if_not_less_equal: { return "jump_if_not_less_equal"; }
			case jump_if_not_less_than: { return "jump_if_not_less_than"; }
			case plus: { return "plus"; }
			case minus: { return "minus"; }
			case multiply: { return "multiply"; }
			case divide: { return "divide"; }
			case modulus: { return "modulus"; }
			case pow: { return "pow"; }
			case bitwise_and: { return "bitwise_and"; }
			case bitwise_or: { return "bitwise_or"; }
			case bitwise_xor: { return "bitwise_xor"; }
			case bitwise_left_shift: { return "bitwise_left_shift"; }
			case bitwise_right_shift: { return "bitwise_right_shift"; }
			case plus_key: { return "plus_key"; }
			case minus_key: { return "minus_key"; }
			case multiply_key: { return "multiply_key"; }
			case divide_key: { return "divide_key"; }
			case modulus_key: { return "modulus_key"; }
			case pow_key: { return "pow_key"; }
			case bitwise_and_key: { return "bitwise_and_key"; }
			case bitwise_or_key: { return "bitwise_or_key"; }
			case bitwise_xor_key: { return "bitwise_xor_key"; }
			case bitwise_left_shift_key: { return "bitwise_left_shift_key"; }
			case bitwise_right_shift_key: { return "bitwise_right_shift_key"; }
			case logical_and: { return "logical_and"; }
			case logical_or: { return "logical_or"; }
			case logical_and_key: { return "logical_and_key"; }
			case logical_or_key: { return "logical_or_key"; }
			case unary_plus: { return "unary_plus"; }
			case unary_minus: { return "unary_minus"; }
			case unary_not: { return "unary_not"; }
			case unary_bitwise_not: { return "unary_bitwise_not"; }
			case new_table: { return "new_table"; }
			case copy_table: { return "copy_table"; }
			case set_list: { return "set_list"; }
			case for_numeric_loop_prepare: { return "for_numeric_loop_prepare"; }
			case for_numeric_loop: { return "for_numeric_loop"; }
			case for_generic_loop: { return "for_generic_loop"; }
			case for_generic_loop_prepare_inext: { return "for_generic_loop_prepare_inext"; }
			case for_generic_loop_inext: { return "for_generic_loop_inext"; }
			case for_generic_loop_prepare_next: { return "for_generic_loop_prepare_next"; }
			case for_generic_loop_next: { return "for_generic_loop_next"; }
			case load_varargs: { return "load_varargs"; }
			case copy_closure: { return "copy_closure"; }
			case prepare_varargs: { return "prepare_varargs"; }
			case load_key_extra: { return "load_key_extra"; }
			case jump_extra: { return "jump_extra"; }
			case fastcall: { return "fastcall"; }
			case coverage: { return "coverage"; }
			case capture: { return "capture"; }
			case jump_if_equal_key: { return "jump_if_equal_key"; }
			case jump_if_not_equal_key: { return "jump_if_not_key"; }
			case fastcall_1: { return "fastcall_1"; }
			case fastcall_2: { return "fastcall_2"; }
			case fastcall_2_key: { return "fastcall_2_key"; }
			default:
			{
				UNREACHABLE();
				gal_assert(false, "Unsupported operand!");
			}
		}
	}

	constexpr operand_name_type get_capture_type_name(const capture_type type) noexcept
	{
		switch (type)
		{
				using enum capture_type;
			case value: { return "value"; }
			case reference: { return "reference"; }
			case upvalue: { return "upvalue"; }
		}
		UNREACHABLE();
		gal_assert(false, "Unsupported capture type!");
	}

	template<typename T>
		requires(std::is_convertible_v<T, operand_underlying_type> && not std::is_convertible_v<T, operands>)
	constexpr bool is_any_operand(const T operand) noexcept { return is_any_operand(static_cast<operands>(static_cast<operand_underlying_type>(operand))); }

	template<typename T>
		requires std::is_convertible_v<T, operands>
	constexpr bool is_any_operand(const T operand) noexcept { return utils::is_enum_between_of<true, false>(static_cast<operands>(operand), operands::operand_sentinel_begin, operands::operand_sentinel_end); }
}

#endif
