#include <compiler/bytecode_builder.hpp>
#include <utils/macro.hpp>
#include <utils/format.hpp>

namespace gal::compiler
{
	constexpr bytecode_encoder::~bytecode_encoder() = default;

	void bytecode_builder::validate() const
	{
		#define CHECK_REG(value) gal_assert((value) < function.max_stack_size)
		#define CHECK_REG_RANGE(value, count) gal_assert((value) + (count) <= function.max_stack_size)
		#define CHECK_UPVALUE(value) gal_assert((value) < function.num_upvalues)
		#define CHECK_CONSTANT_TYPE(index, type) gal_assert(static_cast<decltype(constants_.size())>((index)) < constants_.size() && std::holds_alternative<type>(constants_[static_cast<decltype(constants_.size())>((index))].data))
		#define CHECK_CONSTANT(index) gal_assert(static_cast<decltype(constants_.size())>((index)) < constants_.size())
		#define CHECK_JUMP(value) gal_assert((i + 1 + (value)) < instructions_.size() && is_instruction_valid[i + 1 + (value)] )

		gal_assert(current_function_ != ~decltype(current_function_){0});

		const auto& function = functions_[current_function_];

		// first pass: tag instruction offsets so that we can validate jumps
		std::vector is_instruction_valid(instructions_.size(), false);

		for (decltype(instructions_.size()) i = 0; i < instructions_.size();)
		{
			const auto operand = instruction_to_operand(instructions_[i]);

			is_instruction_valid[i] = true;

			i += get_operand_length(operand);
			gal_assert(i < instructions_.size());
		}

		// second pass: validate the rest of the bytecode
		for (decltype(instructions_.size()) i = 0; i < instructions_.size();)
		{
			const auto instruction = instructions_[i];

			const auto operand = instruction_to_operand<false>(instruction);
			switch (operand)// NOLINT(clang-diagnostic-switch-enum)
			{
					using enum operands;
				case load_null:
				{
					CHECK_REG(instruction_to_a(instruction));
					break;
				}
				case load_boolean:
				{
					CHECK_REG(instruction_to_a(instruction));
					// For boolean values, we do not require that it must be 0/1
					// if it is 0, set it to false, otherwise it is set to true, and there is no need to check B
					CHECK_JUMP(instruction_to_c(instruction));
					break;
				}
				case load_number:
				{
					CHECK_REG(instruction_to_a(instruction));
					break;
				}
				case load_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT(instruction_to_d(instruction));
					break;
				}
				case move:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
				}
				case load_global:
				case set_global:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT_TYPE(instructions_[i + 1], constant::string_type);
					break;
				}
				case load_upvalue:
				case set_upvalue:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_UPVALUE(instruction_to_b(instruction));
					break;
				}
				case close_upvalues:
				{
					CHECK_REG(instruction_to_a(instruction));
					break;
				}
				case load_import:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT_TYPE(instruction_to_d(instruction), constant::import_type);
					break;
				}
				case load_table:
				case set_table:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_REG(instruction_to_c(instruction));
					break;
				}
				case load_table_string_key:
				case set_table_string_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_CONSTANT_TYPE(instructions_[i + 1], constant::string_type);
					break;
				}
				case load_table_number_key:
				case set_table_number_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					break;
				}
				case new_closure:
				{
					CHECK_REG(instruction_to_a(instruction));
					gal_assert(static_cast<decltype(protos_.size())>(instruction_to_d(instruction)) < protos_.size());
					gal_assert(protos_[static_cast<decltype(protos_.size())>(instruction_to_d(instruction))] < functions_.size());

					const auto num_upvalues = functions_[protos_[instruction_to_d(instruction)]].num_upvalues;

					for (decltype(function::num_upvalues) j = 0; j < num_upvalues; ++j)
					{
						gal_assert(i + 1 + j < instructions_.size());
						gal_assert(instruction_to_operand<false>(instructions_[i + 1 + j]) == capture);
					}
					break;
				}
				case named_call:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_CONSTANT_TYPE(instructions_[i + 1], constant::string_type);
					gal_assert(instruction_to_operand<false>(instructions_[i + 2]) == call);
					break;
				}
				case call:
				{
					CHECK_REG(instruction_to_a(instruction));
					const auto n_params = instruction_to_b(instruction);
					const auto n_results = instruction_to_c(instruction);
					CHECK_REG_RANGE(instruction_to_a(instruction) + 1, n_params == 0 ? 0 : n_params - 1);// 1 ~ n parameters
					CHECK_REG_RANGE(instruction_to_a(instruction), n_results == 0 ? 0 : n_results - 1);  // 1 ~ n results
					break;
				}
				case call_return:
				{
					const auto n_results = instruction_to_b(instruction);
					CHECK_REG_RANGE(instruction_to_a(instruction), n_results == 0 ? 0 : n_results - 1);// 0 ~ n results - 1
					break;
				}
				case jump:
				{
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case jump_if:
				case jump_if_not:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case jump_if_equal:
				case jump_if_less_equal:
				case jump_if_less_than:
				case jump_if_not_equal:
				case jump_if_not_less_equal:
				case jump_if_not_less_than:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instructions_[i + 1]);
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case jump_if_equal_key:
				case jump_if_not_equal_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT(instructions_[i + 1]);
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case plus:
				case minus:
				case multiply:
				case divide:
				case modulus:
				case pow:
				case bitwise_and:
				case bitwise_or:
				case bitwise_xor:
				case bitwise_left_shift:
				case bitwise_right_shift:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_REG(instruction_to_c(instruction));
					break;
				}
				case plus_key:
				case minus_key:
				case multiply_key:
				case divide_key:
				case modulus_key:
				case pow_key:
				case bitwise_and_key:
				case bitwise_or_key:
				case bitwise_xor_key:
				case bitwise_left_shift_key:
				case bitwise_right_shift_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_CONSTANT_TYPE(instruction_to_c(instruction), constant::number_type);
					break;
				}
				case logical_and:
				case logical_or:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_REG(instruction_to_c(instruction));
					break;
				}
				case logical_and_key:
				case logical_or_key:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					CHECK_CONSTANT(instruction_to_c(instruction));
					break;
				}
				case unary_plus:
				case unary_minus:
				case unary_not:
				case unary_bitwise_not:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_REG(instruction_to_b(instruction));
					break;
				}
				case new_table:
				{
					CHECK_REG(instruction_to_a(instruction));
					break;
				}
				case copy_table:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT_TYPE(instruction_to_d(instruction), constant::table_type);
					break;
				}
				case set_list:
				{
					CHECK_REG(instruction_to_a(instruction));
					const auto count = instruction_to_c(instruction);
					CHECK_REG_RANGE(instruction_to_b(instruction), count == 0 ? 0 : count - 1);
					break;
				}
				case for_numeric_loop_prepare:
				case for_numeric_loop:
				{
					// for numeric loop protocol: A, A+1, A+2 are used for iteration
					CHECK_REG(instruction_to_a(instruction) + 2);
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case for_generic_loop:
				{
					// for generic loop protocol: A, A+1, A+2 are used for iteration protocol
					// A+3, ... are loop variables
					CHECK_REG(instruction_to_a(instruction) + 2 + instructions_[i + 1]);
					CHECK_JUMP(instruction_to_d(instruction));
					gal_assert(instructions_[i + 1] >= 1);
					break;
				}
				case for_generic_loop_prepare_inext:
				case for_generic_loop_inext:
				case for_generic_loop_prepare_next:
				case for_generic_loop_next:
				{
					// for generic loop protocol: A, A+1, A+2 are used for iteration protocol
					// A+3, A+4 are loop variables
					CHECK_REG(instruction_to_a(instruction) + 4);
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case load_varargs:
				{
					const auto n_results = instruction_to_b(instruction);
					CHECK_REG_RANGE(instruction_to_a(instruction), n_results == 0 ? 0 : n_results - 1);// 0 ~ n results - 1
					break;
				}
				case copy_closure:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT_TYPE(instruction_to_d(instruction), constant::closure_type);
					const auto proto = constants_[instruction_to_d(instruction)].get<constant::closure_index>();
					gal_assert(proto < functions_.size());
					const auto num_upvalues = functions_[proto].num_upvalues;

					for (decltype(function::num_upvalues) j = 0; j < num_upvalues; ++j)
					{
						gal_assert(i + 1 + j < instructions_.size());
						gal_assert(instruction_to_operand<false>(instructions_[i + 1 + j]) == capture);
						gal_assert(is_valid_capture_operand(instruction));
					}
					break;
				}
				case prepare_varargs:
				{
					gal_assert(instruction_to_a(instruction) == function.num_params);
					gal_assert(function.is_vararg);
					break;
				}
				case debugger_break: { break; }
				case jump_back:
				{
					CHECK_JUMP(instruction_to_d(instruction));
					break;
				}
				case load_key_extra:
				{
					CHECK_REG(instruction_to_a(instruction));
					CHECK_CONSTANT(instructions_[i + 1]);
					break;
				}
				case jump_extra:
				{
					CHECK_JUMP(instruction_to_e(instruction));
					break;
				}
				case fastcall:
				{
					CHECK_JUMP(instruction_to_c(instruction));
					gal_assert(instruction_to_operand<false>(instructions_[i + 1 + instruction_to_c(instruction)]) == call);
					break;
				}
				case fastcall_1:
				{
					CHECK_REG(instruction_to_b(instruction));
					CHECK_JUMP(instruction_to_c(instruction));
					gal_assert(instruction_to_operand<false>(instructions_[i + 1 + instruction_to_c(instruction)]) == call);
					break;
				}
				case fastcall_2:
				{
					CHECK_REG(instruction_to_b(instruction));
					CHECK_JUMP(instruction_to_c(instruction));
					gal_assert(instruction_to_operand<false>(instructions_[i + 1 + instruction_to_c(instruction)]) == call);
					CHECK_REG(instructions_[i + 1]);
					break;
				}
				case fastcall_2_key:
				{
					CHECK_REG(instruction_to_b(instruction));
					CHECK_JUMP(instruction_to_c(instruction));
					gal_assert(instruction_to_operand<false>(instructions_[i + 1 + instruction_to_c(instruction)]) == call);
					CHECK_CONSTANT(instructions_[i + 1]);
					break;
				}
				case coverage: { break; }
				case capture:
				{
					switch (instruction_to_capture_type(instruction_to_a(instruction)))
					{
						case capture_type::value:
						case capture_type::reference:
						{
							CHECK_REG(instruction_to_b(instruction));
							break;
						}
						case capture_type::upvalue:
						{
							CHECK_UPVALUE(instruction_to_b(instruction));
							break;
						}
					}
				}
				default:
				{
					UNREACHABLE();
					gal_assert(false, "Unsupported operand!");
				}
			}

			i += get_operand_length(operand);
			gal_assert(i <= instructions_.size());
		}

		#undef CHECK_REG
		#undef CHECK_REG_RANGE
		#undef CHECK_UPVALUE
		#undef CHECK_CONSTANT_TYPE
		#undef CHECK_CONSTANT
		#undef CHECK_JUMP
	}

	std::string bytecode_builder::dump_current_function() const
	{
		if (not utils::is_enum_flag_contains(dump_flags_, dump_flags::code)) { return {}; }

		const auto* code_begin = instructions_.data();
		const auto* code = instructions_.data();
		const auto* code_end = instructions_.data() + instructions_.size();

		int last_line = -1;

		std::string result;

		if (utils::is_enum_flag_contains(dump_flags_, dump_flags::locals))
		{
			for (decltype(debug_locals_.size()) i = 0; i < debug_locals_.size(); ++i)
			{
				// todo:  it would be nice to emit name as well but it requires reverse lookup through string_tables_
				const auto& [_, reg, begin_pc, end_pc] = debug_locals_[i];

				gal_assert(begin_pc < end_pc);
				gal_assert(begin_pc < lines_.size());
				// end_pc is exclusive in the debug info, but it's more intuitive to print inclusive data
				gal_assert(end_pc <= lines_.size());

				std_format::format_to(
						std::back_inserter(result),
						"local {}: reg {}, begin_pc {} line {}, end_pc {} line {}\n",
						i,
						reg,
						begin_pc,
						lines_[begin_pc],
						end_pc - 1,
						lines_[end_pc - 1]
						);
			}
		}

		while (code != code_end)
		{
			if (const auto operand = instruction_to_operand<false>(*code);
				operand == operands::prepare_varargs)
			{
				// Don't emit function header in bytecode - it's used for call dispatching and doesn't contain "interesting" information
				++code;
				continue;
			}

			if (utils::is_enum_flag_contains(dump_flags_, dump_flags::source))
			{
				const auto line = lines_[code - code_begin];

				if (line > 0 && line != last_line)
				{
					gal_assert(static_cast<decltype(dump_source_.size())>(line - 1) < dump_source_.size());

					std_format::format_to(
							std::back_inserter(result),
							"line: {:5<} source: {}",
							line,
							dump_source_[line - 1]);
					last_line = line;
				}
			}

			if (utils::is_enum_flag_contains(dump_flags_, dump_flags::line))
			{
				std_format::format_to(
						std::back_inserter(result),
						"line: {:5<}",
						lines_[code - code_begin]);
			}

			code = dump_instruction(code, result);
		}

		return result;
	}

	const operand_underlying_type* bytecode_builder::dump_instruction(const operand_underlying_type* code, std::string& output) const
	{
		constexpr auto* operand_abc_format = "operand: {} -> ABC: '{}'-'{}'-'{}'\n";
		constexpr auto* operand_abc_aux_format = "operand: {} -> ABC: '{}'-'{}'-'{}' -> AUX: '{}'\n";
		constexpr auto* operand_ad_format = "operand: '{}' -> AD: '{}'-'{}'\n";
		constexpr auto* operand_ad_aux_format = "operand: '{}' -> AD: '{}'-'{}' -> AUX: '{}'\n";
		constexpr auto* operand_e_format = "operand: {} -> E: {}\n";

		const auto instruction = *code++;
		const auto operand = instruction_to_operand<false>(instruction);

		auto append_abc = [&output, operand](
				operand_abc_underlying_type a,
				operand_abc_underlying_type b,
				operand_abc_underlying_type c)
		{
			std_format::format_to(
					std::back_inserter(output),
					operand_abc_format,
					get_operands_name(operand),
					a,
					b,
					c
					);
		};

		auto append_abc_default = [&append_abc, instruction]
		{
			return append_abc(
					instruction_to_a(instruction),
					instruction_to_b(instruction),
					instruction_to_c(instruction)
					);
		};

		auto append_abc_aux = [&output, operand](
				operand_underlying_type aux,
				operand_abc_underlying_type a,
				operand_abc_underlying_type b,
				operand_abc_underlying_type c)
		{
			std_format::format_to(
					std::back_inserter(output),
					operand_abc_aux_format,
					get_operands_name(operand),
					a,
					b,
					c,
					aux
					);
		};

		auto append_abc_aux_default = [&append_abc_aux, instruction](const operand_underlying_type aux)
		{
			return append_abc_aux(
					aux,
					instruction_to_a(instruction),
					instruction_to_b(instruction),
					instruction_to_c(instruction)
					);
		};

		auto append_ad_default = [&output, instruction, operand]
		{
			std_format::format_to(
					std::back_inserter(output),
					operand_ad_format,
					get_operands_name(operand),
					instruction_to_a(instruction),
					instruction_to_d(instruction));
		};

		auto append_ad_aux_default = [&output, instruction, operand](operand_underlying_type aux)
		{
			std_format::format_to(
					std::back_inserter(output),
					operand_ad_aux_format,
					get_operands_name(operand),
					instruction_to_a(instruction),
					instruction_to_d(instruction),
					aux);
		};

		auto append_e_default = [&output, instruction, operand]
		{
			std_format::format_to(
					std::back_inserter(output),
					operand_e_format,
					get_operands_name(operand),
					instruction_to_e(instruction));
		};

		switch (operand)// NOLINT(clang-diagnostic-switch-enum)
		{
				using enum operands;
			case debugger_break:
			case coverage: { output.append(get_operands_name(operand)); }
			case capture:
			{
				std_format::format_to(std::back_inserter(output), "TYPE: {} -> ", get_capture_type_name(static_cast<capture_type>(instruction_to_a(instruction))));
				[[fallthrough]];
			}
			case load_null:
			case load_boolean:
			case move:
			case load_upvalue:
			case set_upvalue:
			case close_upvalues:
			case load_table:
			case set_table:
			case plus:
			case minus:
			case multiply:
			case divide:
			case modulus:
			case pow:
			case bitwise_and:
			case bitwise_or:
			case bitwise_xor:
			case bitwise_left_shift:
			case bitwise_right_shift:
			case plus_key:
			case minus_key:
			case multiply_key:
			case divide_key:
			case modulus_key:
			case pow_key:
			case bitwise_and_key:
			case bitwise_or_key:
			case bitwise_xor_key:
			case bitwise_left_shift_key:
			case bitwise_right_shift_key:
			case logical_and:
			case logical_or:
			case logical_and_key:
			case logical_or_key:
			case unary_plus:
			case unary_minus:
			case unary_not:
			case unary_bitwise_not:
			case fastcall:
			case fastcall_1:
			{
				append_abc_default();
				break;
			}
			case load_table_number_key:
			case set_table_number_key:
			{
				append_abc(instruction_to_a(instruction), instruction_to_b(instruction), instruction_to_c(instruction) + 1);
				break;
			}
			case call:
			{
				append_abc(instruction_to_a(instruction), instruction_to_b(instruction) - 1, instruction_to_c(instruction) - 1);
				break;
			}
			case call_return:
			{
				append_abc(instruction_to_a(instruction), instruction_to_b(instruction) - 1, instruction_to_c(instruction));
				break;
			}
			case load_varargs: { append_abc(instruction_to_a(instruction), instruction_to_b(instruction) - 1, instruction_to_c(instruction)); }
			case load_global:
			case set_global:
			case load_table_string_key:
			case set_table_string_key:
			case named_call:
			{
				append_abc_aux_default(*code++);
				break;
			}
			case new_table:
			{
				append_abc_aux(*code++, instruction_to_a(instruction), instruction_to_b(instruction) == 0 ? 0 : static_cast<operand_abc_underlying_type>(1 << (instruction_to_b(instruction) - 1)), instruction_to_c(instruction));
				break;
			}
			case set_list:
			{
				append_abc_aux(*code++, instruction_to_a(instruction), instruction_to_b(instruction), instruction_to_c(instruction) - 1);
				break;
			}
			case load_import:
			{
				++code;// aux
				[[fallthrough]];
			}
			case load_number:
			case load_key:
			case new_closure:
			case jump:
			case jump_back:
			case jump_if:
			case jump_if_not:
			case copy_table:
			case for_numeric_loop_prepare:
			case for_numeric_loop:
			case for_generic_loop_prepare_inext:
			case for_generic_loop_inext:
			case for_generic_loop_prepare_next:
			case for_generic_loop_next:
			case copy_closure:
			{
				append_ad_default();
				break;
			}
			case jump_if_equal:
			case jump_if_less_equal:
			case jump_if_less_than:
			case jump_if_not_equal:
			case jump_if_not_less_equal:
			case jump_if_not_less_than:
			case for_generic_loop:
			case load_key_extra:
			case jump_if_equal_key:
			case jump_if_not_equal_key:
			case fastcall_2:
			case fastcall_2_key:
			{
				append_ad_aux_default(*code++);
				break;
			}
			case jump_extra:
			{
				append_e_default();
				break;
			}
			default:
			{
				UNREACHABLE();
				gal_assert(false, "Unsupported operand!");
			}
		}

		return code;
	}

	namespace
	{
		template<typename String, typename T>
			requires(sizeof(T) == sizeof(std::byte)) && (sizeof(typename String::value_type) == sizeof(std::byte))
		constexpr void write_byte(String& data, const T value) { data.push_back(std::bit_cast<typename String::value_type>(value)); }

		template<typename String, typename T>
			requires(sizeof(T) == 4 * sizeof(std::byte)) && (sizeof(typename String::value_type) == sizeof(std::byte))
		constexpr void write_int(String& data, const T value) noexcept { data.append(std::bit_cast<const typename String::value_type*>(&value), sizeof(value)); }

		template<typename String>
		constexpr void write_double(String& data, const ast::gal_number_type value) noexcept { data.append(std::bit_cast<const typename String::value_type*>(&value), sizeof(value)); }

		template<typename String>
		constexpr void write_var_int(String& data, bytecode_builder::index_type value) noexcept
		{
			do
			{
				write_byte(data, static_cast<operand_abc_underlying_type>((value & 127) | ((value > 127) << 7)));
				value >>= 7;
			} while (value);
		}
	}

	void bytecode_builder::write_function(std::string& str, const function_id_type id) const
	{
		gal_assert(static_cast<decltype(functions_.size())>(id) < functions_.size());

		const auto& function = functions_[id];

		// header
		write_byte(str, function.max_stack_size);
		write_byte(str, function.num_params);
		write_byte(str, function.num_upvalues);
		write_byte(str, function.is_vararg);

		// instructions
		// todo
		write_int(str, static_cast<operand_underlying_type>(instructions_.size()));

		for (decltype(instructions_.size()) i = 0; i < instructions_.size();)
		{
			const auto operand = instruction_to_operand<false>(instructions_[i]);
			gal_assert(utils::is_enum_between_of<false, false>(operand, operands::operand_sentinel_begin, operands::operand_sentinel_end));

			const auto length = get_operand_length(operand);
			const auto encode = encoder_ ? encoder_->encode_operand(static_cast<bytecode_encoder::encoder_require_type>(operand)) : static_cast<bytecode_encoder::encoder_require_type>(operand);

			write_int(str, encode | (instructions_[i] & ~max_operands_size));

			for (decltype(instructions_.size()) j = 1; j < length; ++j) { write_int(str, instructions_[i + j]); }

			i += length;
		}

		// constants
		// todo
		write_int(str, static_cast<operand_underlying_type>(constants_.size()));

		for (const auto& c: constants_)
		{
			c.visit(
					[this, &str]<typename T>(T&& value)
					{
						using type = T;
						if constexpr (std::is_same_v<type, constant::null_type>) { write_byte(str, bytecode_tag::null); }
						else if constexpr (std::is_same_v<type, constant::boolean_type>)
						{
							write_byte(str, bytecode_tag::boolean);
							write_byte(str, value);
						}
						else if constexpr (std::is_same_v<type, constant::number_type>)
						{
							write_byte(str, bytecode_tag::number);
							write_double(str, value);
						}
						else if constexpr (std::is_same_v<type, constant::string_type>)
						{
							write_byte(str, bytecode_tag::string);
							write_var_int(str, value);
						}
						else if constexpr (std::is_same_v<type, constant::import_type>)
						{
							write_byte(str, bytecode_tag::import);
							write_int(str, value);
						}
						else if constexpr (std::is_same_v<type, constant::table_type>)
						{
							const auto& shape = table_shapes_[value];
							write_byte(str, bytecode_tag::table);
							write_var_int(str, shape.length);
							for (decltype(shape.length) i = 0; i < shape.length; ++i) { write_var_int(str, shape.key[i]); }
						}
						else if constexpr (std::is_same_v<type, constant::closure_type>)
						{
							write_byte(str, bytecode_tag::closure);
							write_var_int(str, value);
						}
						else
						{
							UNREACHABLE();
							gal_assert(false, "non-exhaustive visitor!");
						}
					});
		}

		// child protos
		// todo
		write_var_int(str, static_cast<index_type>(protos_.size()));

		for (const auto child: protos_) { write_var_int(str, child); }

		// debug info
		write_var_int(str, function.debug_name_index);

		if (const auto has_line = std::ranges::find(lines_, 0) == lines_.end(); has_line)
		{
			write_byte(str, true);
			write_line_info(str);
		}
		else { write_byte(str, false); }

		if (const auto has_debug = (not debug_locals_.empty() || not debug_upvalues_.empty()); has_debug)
		{
			write_byte(str, true);

			// todo
			write_var_int(str, static_cast<index_type>(debug_locals_.size()));
			for (const auto& [name, reg, begin_pc, end_pc]: debug_locals_)
			{
				write_var_int(str, name);
				write_var_int(str, begin_pc);
				write_var_int(str, end_pc);
				write_byte(str, reg);
			}

			// todo
			write_var_int(str, static_cast<index_type>(debug_upvalues_.size()));
			for (const auto& [name]: debug_upvalues_) { write_var_int(str, name); }
		}
		else { write_byte(str, false); }
	}

	void bytecode_builder::write_line_info(std::string& str) const
	{
		auto log2 = []<std::integral T>(T value) constexpr noexcept
		{
			gal_assert(value != 0);

			decltype(value) ret = 0;
			while (std::cmp_greater_equal(value, 2 << ret)) { ++ret; }

			return ret;
		};

		// this function encodes lines inside each span as a 8-bit delta to span baseline
		// span is always a power of two; depending on the line info input, it may need to be as low as 1
		int span = 1 << 24;

		// first pass: determine span length
		for (decltype(lines_.size()) offset = 0; offset < lines_.size(); offset += span)
		{
			auto next = offset;

			auto min = lines_[offset];
			auto max = lines_[offset];

			for (; next < lines_.size() && next < offset + span; ++next)
			{
				min = std::ranges::min(min, lines_[next]);
				max = std::ranges::max(max, lines_[next]);

				if (max - min > 255) { break; }
			}

			if (next < lines_.size() && std::cmp_less(next - offset, span))
			{
				// since not all lines in the range fit in 8b delta, we need to shrink the span
				// next iteration will need to reprocess some lines again since span changed
				span = 1 << log2(next - offset);
			}
		}

		// second pass: compute span base
		std::vector baseline((lines_.size() - 1) / span + 1, 0);

		for (decltype(lines_.size()) offset = 0; offset < lines_.size(); offset += span)
		{
			auto next = offset;

			auto min = lines_[offset];

			for (; next < lines_.size() && std::cmp_less(next - offset, span); ++next) { min = std::ranges::min(min, lines_[next]); }

			baseline[offset / span] = min;
		}

		// third pass: write resulting data
		const auto log_span = log2(span);

		write_byte(str, static_cast<std::byte>(log_span));

		std::uint8_t last_offset = 0;
		for (decltype(lines_.size()) i = 0; i < lines_.size(); ++i)
		{
			const auto delta = lines_[i] - baseline[i >> log_span];
			gal_assert(delta >= 0 && delta <= 255);

			write_byte(str, static_cast<std::byte>(delta - last_offset));
			last_offset = static_cast<decltype(last_offset)>(delta);
		}

		int last_line = 0;
		for (const auto i: baseline)
		{
			write_int(str, i - last_line);
			last_line = i;
		}
	}

	void bytecode_builder::write_string_table(std::string& str) const
	{
		std::vector strings(string_tables_.size(), string_ref_type{});

		for (auto& [ref, index]: string_tables_)
		{
			gal_assert(index > 0 && index <= strings.size());
			strings[index - 1] = ref;
		}

		write_var_int(str, static_cast<index_type>(strings.size()));

		for (auto& ref: strings)
		{
			write_var_int(str, static_cast<index_type>(ref.size()));
			str.append(ref);
		}
	}

	std::int32_t bytecode_builder::add_constant(const constant& key, const constant& value)
	{
		if (const auto it = constant_map_.find(key); it != constant_map_.end()) { return it->second; }

		const auto id = static_cast<std::int32_t>(constants_.size());

		if (std::cmp_greater_equal(id, max_constant_size)) { return -1; }

		constant_map_[key] = id;
		constants_.push_back(value);

		return id;
	}

	bytecode_builder::index_type bytecode_builder::add_string_table_entry(const string_ref_type value)
	{
		auto& index = string_tables_[value];

		// note: bytecode serialization format uses 1-based table indices, 0 is reserved to mean null
		if (index == 0) { index = static_cast<index_type>(string_tables_.size()); }

		return index;
	}

	bytecode_builder::function_id_type bytecode_builder::begin_function(operand_abc_underlying_type num_params, bool is_vararg)
	{
		gal_assert(current_function_ == ~decltype(current_function_){0});

		functions_.emplace_back(function{.num_params = num_params, .is_vararg = is_vararg});

		const auto id = static_cast<function_id_type>(functions_.size() - 1);

		current_function_ = id;

		has_long_jump_ = false;
		debug_line_ = 0;

		return id;
	}

	void bytecode_builder::end_function(const operand_abc_underlying_type max_stack_size, const operand_abc_underlying_type num_upvalues)
	{
		gal_assert(current_function_ != ~decltype(current_function_){0});

		auto& function = functions_[current_function_];

		function.max_stack_size = max_stack_size;
		function.num_upvalues = num_upvalues;

		gal_assert((validate(), true));  // NOLINT(clang-diagnostic-comma)

		// very approximate: 4 bytes per instruction for code, 1 byte for debug line, and 1-2 bytes for aux data like constants
		function.data.reserve(instructions_.size() * 7);

		write_function(function.data, current_function_);

		current_function_ = ~decltype(current_function_){0};

		// this call is indirect to make sure we only gain link time dependency on dump_current_function when needed
		if (dump_handler_) { function.dump = (this->*dump_handler_)(); }

		instructions_.clear();
		lines_.clear();
		constants_.clear();
		protos_.clear();
		jumps_.clear();

		table_shapes_.clear();

		constant_map_.clear();
		table_shape_map_.clear();

		debug_locals_.clear();
		debug_upvalues_.clear();
	}

	void bytecode_builder::set_main_function(const function_id_type function_id) { main_function_ = function_id; }

	bytecode_builder::signed_index_type bytecode_builder::add_constant_null()
	{
		const constant key{constant::null_type{}};
		const constant value{constant::null_type{}};

		return add_constant(key, value);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_constant_boolean(const constant::boolean_type value)
	{
		const constant key{value};

		return add_constant(key, key);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_constant_number(const constant::number_type value)
	{
		const constant key{value};

		return add_constant(key, key);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_constant_string(const string_ref_type value)
	{
		const auto index = add_string_table_entry(value);

		const constant key{constant::constant_type{std::in_place_index<constant::sting_index>, index}};

		return add_constant(key, key);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_import(const constant::import_type import_id)
	{
		const constant key{constant::constant_type{std::in_place_index<constant::import_index>, import_id}};

		return add_constant(key, key);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_constant_table(const table_shape& shape)
	{
		if (const auto it = table_shape_map_.find(shape); it != table_shape_map_.end()) { return it->second; }

		const auto id = static_cast<signed_index_type>(constants_.size());

		if (std::cmp_greater_equal(id, max_constant_size)) { return -1; }

		const constant key{constant::constant_type{std::in_place_index<constant::table_index>, static_cast<constant::table_type>(table_shapes_.size())}};

		table_shape_map_[shape] = id;
		table_shapes_.push_back(shape);
		constants_.push_back(key);

		return id;
	}

	bytecode_builder::signed_index_type bytecode_builder::add_constant_closure(constant::closure_type function_id)
	{
		const constant key{constant::constant_type{std::in_place_index<constant::closure_index>, function_id}};

		return add_constant(key, key);
	}

	bytecode_builder::signed_index_type bytecode_builder::add_child_function(function_id_type function_id)
	{
		const auto id = static_cast<signed_index_type>(protos_.size());

		if (std::cmp_greater_equal(id, max_closure_size)) { return -1; }

		protos_.push_back(function_id);

		return id;
	}
}
