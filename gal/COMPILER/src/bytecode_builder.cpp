#include <compiler/bytecode_builder.hpp>
#include <utils/macro.hpp>

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

		gal_assert(current_function != ~decltype(current_function){0});

		const auto& function = functions_[current_function];

		// first pass: tag instruction offsets so that we can validate jumps
		std::vector<bool> is_instruction_valid(instructions_.size(), false);

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
			switch (operand)
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
					const auto proto = constants_[instruction_to_d(instruction)].get<constant::closure_type>();
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
}
