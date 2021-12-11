#include <algorithm>
#include <gal.hpp>
#include <utils/assert.hpp>
#include <utils/format.hpp>
#include <vm/common.hpp>
#include <vm/compiler.hpp>
#include <vm/debugger.hpp>
#include <vm/value.hpp>
#include <vm/vm.hpp>

namespace gal
{
	object_string gal_virtual_machine_state::validate_superclass(const object_string &name, magic_value superclass_value, gal_size_type num_fields)
	{
		object_string error{*this};

		// Make sure the superclass is a class.
		if (not superclass_value.is_class())
		{
			std_format::format_to(error.get_appender(), "Class '{}' cannot inherit from a non-class object.", name.str());
			return error;
		}

		// Make sure it doesn't inherit from a sealed built-in type. Primitive methods
		// on these classes assume the instance is one of the other object_xxx types and
		// will fail horribly if it's actually an object_instance.
		auto *superclass = superclass_value.as_class();
		if (
				superclass == class_class_ ||
				superclass == fiber_class_ ||
				superclass == function_class_ ||
				superclass == list_class_ ||
				superclass == map_class_ ||
				superclass == range_class_ ||
				superclass == string_class_ ||
				superclass == boolean_class_ ||
				superclass == null_class_ ||
				superclass == number_class_)
		{
			std_format::format_to(error.get_appender(), "Class '{}' cannot inherit from built-in class '{}'.", name.str(), superclass->get_class_name().str());
			return error;
		}

		if (superclass->is_outer_class())
		{
			std_format::format_to(error.get_appender(), "Class '{}' cannot inherit from outer class '{}'.", name.str(), superclass->get_class_name().str());
			return error;
		}

		if (superclass->get_remain_field_size() < num_fields)
		{
			std_format::format_to(
					error.get_appender(),
					"There are currently {} fields in class, and {} fields will be added, but there can only be {} fields at most, including inherited ones.",
					superclass->get_field_size(),
					num_fields,
					max_fields);
			return error;
		}

		return error;
	}

	void gal_virtual_machine_state::bind_outer_class(object_class &obj_class, object_module &module)
	{
		gal_configuration::gal_outer_class_method methods{};

		// Check the optional built-in module first so the host can override it.
		if (configuration_.bind_outer_class_function != nullptr)
		{
			methods = configuration_.bind_outer_class_function(*this, module.get_name().data(), obj_class.get_class_name().data());
		}

		// If the host didn't provide it, see if it's a built-in optional module.
		if (methods.allocate == nullptr && methods.finalize == nullptr)
		{
			// todo: built-in module
		}

		method m{};
		m.type		= method_type::outer_type;

		// Add the symbol even if there is no allocator, so we can ensure that the
		// symbol itself is always in the symbol table.
		auto symbol = method_names_.ensure(*this, gal_configuration::gal_outer_class_method::allocate_symbol_name, gal_configuration::gal_outer_class_method::allocate_symbol_name_length);
		if (methods.allocate != nullptr)
		{
			m.as.outer_method_function = methods.allocate;
			obj_class.set_method(symbol, m);
		}

		// Add the symbol even if there is no finalizer, so we can ensure that the
		// symbol itself is always in the symbol table.
		symbol = method_names_.ensure(*this, gal_configuration::gal_outer_class_method::finalize_symbol_name, gal_configuration::gal_outer_class_method::finalize_symbol_name_length);
		if (methods.finalize != nullptr)
		{
			m.as.outer_method_function = reinterpret_cast<gal_outer_method_function_type>(methods.finalize);
			obj_class.set_method(symbol, m);
		}
	}

	gal_outer_method_function_type gal_virtual_machine_state::find_outer_method(const char *module_name, const char *class_name, bool is_static, const char *signature)
	{
		gal_outer_method_function_type method = nullptr;

		if (configuration_.bind_outer_method_function != nullptr)
		{
			method = configuration_.bind_outer_method_function(*this, module_name, class_name, is_static, signature);
		}

		// If the host didn't provide it, see if it's an optional one.
		if (method == nullptr)
		{
			// todo: build-in method
		}

		return method;
	}

	void gal_virtual_machine_state::bind_method(opcodes_type method_type, gal_index_type symbol, object_module &module, object_class &obj_class, magic_value method_value)
	{
		object_class *real_obj = &obj_class;
		if (method_type == opcodes_type::CODE_METHOD_STATIC)
		{
			real_obj = obj_class.get_class();
		}

		method m{};
		if (method_value.is_string())
		{
			const auto *name  = method_value.as_string()->data();

			m.type			  = method_type::outer_type;
			auto outer_method = find_outer_method(
					module.get_name().data(),
					obj_class.get_class_name().data(),
					method_type == opcodes_type::CODE_METHOD_STATIC,
					name);
			if (outer_method)
			{
				m.as.outer_method_function = outer_method;
			}
			else
			{
				auto error = object::ctor<object_string>(*this);
				std_format::format_to(error->get_appender(), "Could not find outer method '{}' for class '{}' in module '{}'.", name, real_obj->get_class_name().str(), module.get_name().str());
				fiber_->set_error(error->operator magic_value());
				return;
			}
		}
		else
		{
			m.type		 = method_type::block_type;
			m.as.closure = method_value.as_closure();

			// Patch up the bytecode now that we know the superclass.
			set_class_method(*real_obj, m.as.closure->get_function());
		}

		real_obj->set_method(symbol, m);
	}

	void gal_virtual_machine_state::call_outer(object_fiber &fiber, gal_outer_method_function_type outer, gal_size_type num_args)
	{
		gal_assert(api_stack_ == nullptr, "Cannot already be in outer call.");

		api_stack_ = fiber.get_stack_point(num_args);

		outer(this);

		// Discard the stack slots for the arguments and temporaries but leave one
		// for the result.
		fiber.set_stack_top(api_stack_ + 1);

		api_stack_ = nullptr;
	}

	void gal_virtual_machine_state::runtime_error()
	{
		auto *ret = fiber_->raise_error();
		if (ret)
		{
			fiber_ = ret;
			return;
		}

		// If we got here, nothing caught the error, so show the stack trace.
		debugger::print_stack_trace(*this);
		fiber_	   = nullptr;
		api_stack_ = nullptr;
	}

	void gal_virtual_machine_state::method_not_found(object_class &obj_class, gal_index_type symbol)
	{
		auto error = object::ctor<object_string>(*this);
		std_format::format_to(error->get_appender(), "{} does not implement '{}'.", obj_class.get_class_name().str(), method_names_[symbol].str());
		fiber_->set_error(error->operator magic_value());
	}

	object_module *gal_virtual_machine_state::get_module(magic_value name) const
	{
		auto module = modules_.get(name);
		return module == magic_value_undefined ? nullptr : module.as_module();
	}

	object_closure *gal_virtual_machine_state::compile_in_module(magic_value name, const char *source, bool is_expression, bool print_errors)
	{
		// See if the module has already been loaded.
		auto *module = get_module(name);
		if (not module)
		{
			module = object::ctor<object_module>(*name.as_string());

			modules_.set(name, module->operator magic_value());

			// Implicitly import the core module.
			module->copy_variables(*get_module(magic_value_null));
		}

		auto *function = compile(*this, *module, source, is_expression, print_errors);
		if (not function)
		{
			// TODO: Should we still store the module even if it didn't compile?
			return nullptr;
		}

		return object::ctor<object_closure>(*this, *function);
	}

	void gal_virtual_machine_state::create_outer(magic_value *stack)
	{
		auto *obj_class = stack[0].as_class();
		gal_assert(obj_class->is_outer_class(), "Class must be a outer class.");

		// TODO: Don't look up every time.
		const auto symbol = method_names_.find(gal_configuration::gal_outer_class_method::allocate_symbol_name, gal_configuration::gal_outer_class_method::allocate_symbol_name_length);
		gal_assert(symbol != gal_index_not_exist, "Should have defined <allocate> symbol.");

		gal_assert(std::cmp_greater(obj_class->get_methods_size(), symbol), "Class should have allocator.");
		auto &method = obj_class->get_method(symbol);
		gal_assert(method.type == method_type::outer_type, "Allocator should be outer.");

		// Pass the constructor arguments to the allocator as well.
		gal_assert(api_stack_ == nullptr, "Cannot already be in outer call.");
		set_stack_bottom(stack);

		method.as.outer_method_function(this);

		shutdown_stack();
	}

	object_string gal_virtual_machine_state::resolve_module(const object_string &name)
	{
		// If the host doesn't care to resolve, leave the name alone.
		if (configuration_.resolve_module_function == nullptr)
		{
			return name;
		}

		auto &function = fiber_->get_recent_frame().closure->get_function();
		auto &importer = function.get_module().get_name();

		auto *resolved = configuration_.resolve_module_function(*this, importer.data(), name.data());

		if (not resolved)
		{
			auto error = object::ctor<object_string>(*this);
			std_format::format_to(error->get_appender(), "Could not resolve module '{}' imported from '{}'.", name.str(), importer.str());
			fiber_->set_error(error->operator magic_value());
		}

		// If they resolved to the exact same string, we don't need to copy it.
		if (resolved == name)
		{
			return name;
		}

		// Copy the string into a GAL String object.
		// todo: `resolved` sequence is dynamic allocated, we maybe need to delete it here.
		return {*this, resolved, std::strlen(resolved)};
	}

	magic_value gal_virtual_machine_state::import_module(const object_string &name)
	{
		auto read_name = resolve_module(name);

		// If the module is already loaded, we don't need to do anything.
		auto existing  = modules_.get(read_name.operator magic_value());
		if (existing != magic_value_undefined)
		{
			return existing;
		}

		gal_configuration::gal_load_module_result result{};

		// Let the host try to provide the module.
		if (configuration_.load_module_function != nullptr)
		{
			result = configuration_.load_module_function(*this, read_name.data());
		}

		// If the host didn't provide it, see if it's a built-in optional module.
		if (result.source == nullptr)
		{
			result.on_complete = nullptr;
			// todo: built-in module here
		}

		if (result.source == nullptr)
		{
			auto error = object::ctor<object_string>(*this);
			std_format::format_to(error->get_appender(), "Could not load module '{}'.", read_name.str());
			fiber_->set_error(error->operator magic_value());
			return magic_value_null;
		}

		auto *module_closure = compile_in_module(read_name.operator magic_value(), result.source, false, true);

		// Now that we're done, give the result back in case there's cleanup to do.
		if (result.on_complete)
		{
			result.on_complete(*this, read_name.data(), result);
		}

		if (not module_closure)
		{
			auto error = object::ctor<object_string>(*this);
			std_format::format_to(error->get_appender(), "Could not compile module '{}'.", read_name.str());
			fiber_->set_error(error->operator magic_value());
			return magic_value_null;
		}

		// Return the closure that executes the module.
		return module_closure->operator magic_value();
	}

	magic_value gal_virtual_machine_state::get_module_variable(object_module &module, const object_string &variable_name)
	{
		auto variable = module.get_variable(variable_name);

		// It's a runtime error if the imported variable does not exist.
		if (variable != magic_value_undefined)
		{
			return variable;
		}

		auto error = object::ctor<object_string>(*this);
		std_format::format_to(
				error->get_appender(),
				"Could not find a variable named '{}' in module '{}'.",
				variable_name.str(),
				module.get_name().str());
		fiber_->set_error(error->operator magic_value());
		return magic_value_null;
	}

	bool gal_virtual_machine_state::check_arity(magic_value value, gal_size_type num_args)
	{
		gal_assert(value.is_closure(), "Receiver must be a closure.");
		auto &function = value.as_closure()->get_function();

		// We only care about missing arguments, not extras. The "- 1" is because
		// num_args includes the receiver, the function itself, which we don't want to
		// count.
		if (function.check_parameters_arity(num_args - 1))
		{
			return true;
		}

		auto error = object::ctor<object_string>(*this);
		std_format::format_to(
				error->get_appender(),
				"Function {} expects {} argument(s), but only given {} argument(s).",
				function.get_name(),
				function.get_parameters_arity(),
				num_args - 1);
		fiber_->set_error(error->operator magic_value());
		return false;
	}

	gal_interpret_result gal_virtual_machine_state::run_interpreter(object_fiber *fiber)
	{
		// Remember the current fiber, so we can find it if a GC happens.
		fiber_ = fiber;
		fiber->set_state(fiber_state::root_state);

		call_frame		   *frame;
		magic_value		*stack_start;
		const std::uint8_t *ip;
		object_function	*function;

		auto				read_byte = [&]
		{ return static_cast<std::uint8_t>(*ip++); };
		auto read_short = [&]
		{
			ip += 2;
			return static_cast<std::uint16_t>((ip[-2] << 8) | ip[-1]);
		};

		/**
		 * @brief Use this before a call_frame is pushed to store the local variables back
		 * into the current one.
		 */
		auto store_frame = [&]
		{ frame->ip = ip; };

		/**
		 * @brief Use this after a call_frame has been pushed or popped to refresh the local
		 * variables.
		 */
		auto load_frame = [&]
		{
			frame		= &fiber->get_recent_frame();
			stack_start = frame->stack_start;
			ip			= frame->ip;
			function	= &frame->closure->get_function();
		};

		auto runtime_error = [&]
		{
			bool finished = false;

			store_frame();
			this->runtime_error();
			if (fiber_ == nullptr) { finished = true; }
			fiber = fiber_;
			load_frame();

			return finished;
		};

		auto trace_instructions = [&]
		{
			if constexpr (debug_trace_instruction)
			{
				// Prints the stack and instruction before each instruction is executed.
				debugger::dump(*fiber);
				debugger::dump(*this, *function, static_cast<int>(ip - function->get_code_data()));
			}
		};

		load_frame();

		opcodes_type instruction;

	loop_back:
		trace_instructions();
		switch (instruction = static_cast<opcodes_type>(read_byte()))
		{
			case opcodes_type::CODE_LOAD_LOCAL_0:
			case opcodes_type::CODE_LOAD_LOCAL_1:
			case opcodes_type::CODE_LOAD_LOCAL_2:
			case opcodes_type::CODE_LOAD_LOCAL_3:
			case opcodes_type::CODE_LOAD_LOCAL_4:
			case opcodes_type::CODE_LOAD_LOCAL_5:
			case opcodes_type::CODE_LOAD_LOCAL_6:
			case opcodes_type::CODE_LOAD_LOCAL_7:
			case opcodes_type::CODE_LOAD_LOCAL_8:
			{
				fiber->stack_push(stack_start[code_to_scalar(instruction) - code_to_scalar(opcodes_type::CODE_LOAD_LOCAL_0)]);
				goto loop_back;
			}
			case opcodes_type::CODE_LOAD_LOCAL:
			{
				fiber->stack_push(stack_start[read_byte()]);
				goto loop_back;
			}
			case opcodes_type::CODE_LOAD_FIELD_THIS:
			{
				auto filed	  = read_byte();
				auto receiver = stack_start[0];
				gal_assert(receiver.is_instance(), "Receiver should be instance.");
				auto *instance = receiver.as_instance();
				gal_assert(filed < instance->get_field_size(), "Out of bounds field.");
				fiber->stack_push(instance->get_field(filed));
				goto loop_back;
			}
			case opcodes_type::CODE_POP:
			{
				fiber->stack_drop();
				goto loop_back;
			}
			case opcodes_type::CODE_NULL:
			{
				fiber->stack_push({magic_value::null_val});
				goto loop_back;
			}
			case opcodes_type::CODE_FALSE:
			{
				fiber->stack_push({magic_value::false_val});
				goto loop_back;
			}
			case opcodes_type::CODE_TRUE:
			{
				fiber->stack_push({magic_value::true_val});
				goto loop_back;
			}
			case opcodes_type::CODE_STORE_LOCAL:
			{
				stack_start[read_byte()] = *fiber->stack_peek();
				goto loop_back;
			}
			case opcodes_type::CODE_CONSTANT:
			{
				fiber->stack_push(function->get_constant(read_short()));
				goto loop_back;
			}
				{
					// The opcodes for doing method and superclass calls share a lot of code.
					// However, doing an if() test in the middle of the instruction sequence
					// to handle the bit that is special to super calls makes the non-super
					// call path noticeably slower.
					//
					// Instead, we do this old school using an explicit goto to share code for
					// everything at the tail end of the call-handling code that is the same
					// between normal and superclass calls.
					gal_size_type  num_args;
					gal_index_type symbol;

					magic_value	*args;
					object_class	 *obj_class;

					method		   *method;

					case opcodes_type::CODE_CALL_0:
					case opcodes_type::CODE_CALL_1:
					case opcodes_type::CODE_CALL_2:
					case opcodes_type::CODE_CALL_3:
					case opcodes_type::CODE_CALL_4:
					case opcodes_type::CODE_CALL_5:
					case opcodes_type::CODE_CALL_6:
					case opcodes_type::CODE_CALL_7:
					case opcodes_type::CODE_CALL_8:
					case opcodes_type::CODE_CALL_9:
					case opcodes_type::CODE_CALL_10:
					case opcodes_type::CODE_CALL_11:
					case opcodes_type::CODE_CALL_12:
					case opcodes_type::CODE_CALL_13:
					case opcodes_type::CODE_CALL_14:
					case opcodes_type::CODE_CALL_15:
					case opcodes_type::CODE_CALL_16:
					{
						// Add one for the implicit receiver argument.
						num_args  = code_to_scalar(instruction) - code_to_scalar(opcodes_type::CODE_CALL_0) + 1;
						symbol	  = read_short();

						// The receiver is the first argument.
						args	  = fiber->get_stack_point(num_args);
						obj_class = get_class(args[0]);
						goto complete_call;
					}
					case opcodes_type::CODE_SUPER_0:
					case opcodes_type::CODE_SUPER_1:
					case opcodes_type::CODE_SUPER_2:
					case opcodes_type::CODE_SUPER_3:
					case opcodes_type::CODE_SUPER_4:
					case opcodes_type::CODE_SUPER_5:
					case opcodes_type::CODE_SUPER_6:
					case opcodes_type::CODE_SUPER_7:
					case opcodes_type::CODE_SUPER_8:
					case opcodes_type::CODE_SUPER_9:
					case opcodes_type::CODE_SUPER_10:
					case opcodes_type::CODE_SUPER_11:
					case opcodes_type::CODE_SUPER_12:
					case opcodes_type::CODE_SUPER_13:
					case opcodes_type::CODE_SUPER_14:
					case opcodes_type::CODE_SUPER_15:
					case opcodes_type::CODE_SUPER_16:
					{
						// Add one for the implicit receiver argument.
						num_args  = code_to_scalar(instruction) - code_to_scalar(opcodes_type::CODE_SUPER_0) + 1;
						symbol	  = read_short();

						// The receiver is the first argument.
						args	  = fiber->get_stack_point(num_args);

						// The superclass is stored in a constant.
						obj_class = function->get_constant(read_short()).as_class();
						goto complete_call;
					}
					complete_call:
					{
						// If the class's method table doesn't include the symbol, bail.
						if (std::cmp_greater_equal(symbol, obj_class->get_methods_size()) ||
							(method = &obj_class->get_method(symbol))->type == method_type::none_type)
						{
							method_not_found(*obj_class, symbol);
							if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
						}
						switch (method->type)
						{
							case method_type::primitive_type:
							{
								if (method->as.primitive_function(*this, args))
								{
									// The result is now in the first arg slot. Discard the other
									// stack slots.
									fiber->pop_stack(num_args - 1);
								}
								else
								{
									// An error, fiber switch, or call frame change occurred.
									store_frame();

									// If we don't have a fiber to switch to, stop interpreting.
									fiber = fiber_;
									if (fiber == nullptr) { return gal_interpret_result::RESULT_SUCCESS; }
									if (fiber->has_error())
									{
										if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
									}
									load_frame();
								}
								break;
							}
							case method_type::function_call_type:
							{
								if (not check_arity(args[0], num_args))
								{
									if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
									break;
								}

								store_frame();
								method->as.primitive_function(*this, args);
								load_frame();
								break;
							}
							case method_type::outer_type:
							{
								call_outer(*fiber, method->as.outer_method_function, num_args);
								if (fiber->has_error())
								{
									if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
								}
								break;
							}
							case method_type::block_type:
							{
								store_frame();
								fiber->call_function(*this, *method->as.closure, num_args);
								load_frame();
								break;
							}
							case method_type::none_type:
							{
								UNREACHABLE();
								break;
							}
						}
						goto loop_back;
					}
				}
			case opcodes_type::CODE_LOAD_UPVALUE:
			{
				fiber->stack_push(*frame->closure->get_upvalue(read_byte())->get_value());
				goto loop_back;
			}
			case opcodes_type::CODE_STORE_UPVALUE:
			{
				frame->closure->get_upvalue(read_byte())->reset_value(fiber->stack_peek());
				goto loop_back;
			}
			case opcodes_type::CODE_LOAD_MODULE_VAR:
			{
				fiber->stack_push(function->get_module().get_variable(read_short()));
				goto loop_back;
			}
			case opcodes_type::CODE_STORE_MODULE_VAR:
			{
				function->get_module().set_variable(read_short(), *fiber->stack_peek());
				goto loop_back;
			}
			case opcodes_type::CODE_STORE_FIELD_THIS:
			{
				auto field	  = read_byte();
				auto receiver = stack_start[0];
				gal_assert(receiver.is_instance(), "Receiver should be instance.");
				auto *instance = receiver.as_instance();
				gal_assert(field < instance->get_field_size(), "Out of bounds field.");
				instance->get_field(field) = *fiber->stack_peek();
				goto loop_back;
			}
			case opcodes_type::CODE_LOAD_FIELD:
			{
				auto field	  = read_byte();
				auto receiver = fiber->stack_pop();
				gal_assert(receiver->is_instance(), "Receiver should be instance.");
				auto *instance = receiver->as_instance();
				gal_assert(field < instance->get_field_size(), "Out of bounds field.");
				fiber->stack_push(instance->get_field(field));
				goto loop_back;
			}
			case opcodes_type::CODE_STORE_FIELD:
			{
				auto field	  = read_byte();
				auto receiver = fiber->stack_pop();
				gal_assert(receiver->is_instance(), "Receiver should be instance.");
				auto *instance = receiver->as_instance();
				gal_assert(field < instance->get_field_size(), "Out of bounds field.");
				instance->get_field(field) = *fiber->stack_peek();
				goto loop_back;
			}
			case opcodes_type::CODE_JUMP:
			{
				ip += read_short();
				goto loop_back;
			}
			case opcodes_type::CODE_LOOP:
			{
				// Jump back to the top of the loop.
				ip -= read_short();
				goto loop_back;
			}
			case opcodes_type::CODE_JUMP_IF:
			{
				auto offset	   = read_short();
				auto condition = fiber->stack_pop();

				if (condition->is_falsy())
				{
					ip += offset;
				}
				goto loop_back;
			}
			case opcodes_type::CODE_AND:
			{
				auto offset	   = read_short();
				auto condition = fiber->stack_peek();

				if (condition->is_falsy())
				{
					// Short-circuit the right-hand side.
					ip += offset;
				}
				else
				{
					// Discard the condition and evaluate the right-hand side.
					fiber->stack_drop();
				}
				goto loop_back;
			}
			case opcodes_type::CODE_OR:
			{
				auto offset	   = read_short();
				auto condition = fiber->stack_peek();

				if (condition->is_falsy())
				{
					// Discard the condition and evaluate the right-hand side.
					fiber->stack_drop();
				}
				else
				{
					// Short-circuit the right-hand side.
					ip += offset;
				}
				goto loop_back;
			}
			case opcodes_type::CODE_CLOSE_UPVALUE:
			{
				// Close the upvalue for the local if we have one.
				fiber->close_upvalue(*fiber->get_stack_point(1));
				fiber->stack_drop();
				goto loop_back;
			}
			case opcodes_type::CODE_RETURN:
			{
				auto result = fiber->stack_pop();
				fiber->pop_recent_frame();

				// Close any upvalues still in scope.
				fiber->close_upvalue(stack_start[0]);

				// If the fiber is complete, end it.
				if (not fiber->has_frame())
				{
					// See if there's another fiber to return to. If not, we're done.
					if (not fiber->has_caller())
					{
						// Store the final result value at the beginning of the stack so the
						// C++ API can get it.
						fiber->set_stack_point(0, *result);
						fiber->stack_top_rebase(1);
						return gal_interpret_result::RESULT_SUCCESS;
					}

					auto *resuming_fiber = fiber->get_caller();
					fiber->set_caller(nullptr);
					fiber  = resuming_fiber;
					fiber_ = resuming_fiber;

					// Store the result in the resuming fiber.
					fiber->set_stack_point(1, *result);
				}
				else
				{
					// Store the result of the block in the first slot, which is where the
					// caller expects it.
					stack_start[0] = *result;

					// Discard the stack slots for the call frame (leaving one slot for the
					// result).
					fiber->set_stack_top(frame->stack_start + 1);
				}

				load_frame();
				goto loop_back;
			}
			case opcodes_type::CODE_CONSTRUCT:
			{
				gal_assert(stack_start[0].is_class(), "'this' should be a class.");
				stack_start[0] = object::ctor<object_instance>(stack_start[0].as_class())->operator magic_value();
				goto loop_back;
			}
			case opcodes_type::CODE_OUTER_CONSTRUCT:
			{
				gal_assert(stack_start[0].is_class(), "'this' should be a class.");
				create_outer(stack_start);
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}
				goto loop_back;
			}
			case opcodes_type::CODE_CLOSURE:
			{
				// Create the closure and push it on the stack before creating upvalues
				// so that it doesn't get collected.
				auto *func	  = function->get_constant(read_short()).as_function();
				auto *closure = object::ctor<object_closure>(*this, *func);
				fiber->stack_push(closure->operator magic_value());

				// Capture upvalues, if any.
				for (decltype(func->get_upvalues_size()) i = 0; i < func->get_upvalues_size(); ++i)
				{
					const auto is_local = read_byte();
					const auto index	= read_byte();
					if (is_local)
					{
						// Make a new upvalue to close over the parent's local variable.
						closure->push_upvalue(&fiber->capature_upvalue(frame->stack_start[index]));
					}
					else
					{
						// Use the same upvalue as the current call frame.
						closure->push_upvalue(frame->closure->get_upvalue(index));
					}
				}
				goto loop_back;
			}
			case opcodes_type::CODE_END_CLASS:
			{
				fiber->end_class();
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}
				goto loop_back;
			}
			case opcodes_type::CODE_CLASS:
			{
				fiber_->create_class(*this, read_byte(), nullptr);
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}
				goto loop_back;
			}
			case opcodes_type::CODE_OUTER_CLASS:
			{
				fiber_->create_class(*this, object_class::outer_class_fields_size, &function->get_module());
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}
				goto loop_back;
			}
			case opcodes_type::CODE_METHOD_INSTANCE:
			case opcodes_type::CODE_METHOD_STATIC:
			{
				auto  symbol	= read_short();
				auto *obj_class = fiber->stack_peek()->as_class();
				auto  method	= fiber->stack_peek2();
				bind_method(instruction, symbol, function->get_module(), *obj_class, *method);
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}
				fiber->stack_drop();
				fiber->stack_drop();
				goto loop_back;
			}
			case opcodes_type::CODE_END_MODULE:
			{
				last_module_ = &function->get_module();
				fiber->stack_push(magic_value_null);
				goto loop_back;
			}
			case opcodes_type::CODE_IMPORT_MODULE:
			{
				// Make a slot on the stack for the module's fiber to place the return
				// value. It will be popped after this fiber is resumed. Store the
				// imported module's closure in the slot in case a GC happens when
				// invoking the closure.
				fiber->stack_push(import_module(*function->get_constant(read_short()).as_string()));
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}

				// If we get a closure, call it to execute the module body.
				if (auto v = fiber->stack_peek(); v->is_closure())
				{
					store_frame();
					auto *closure = v->as_closure();
					fiber->call_function(*this, *closure, 1);
					load_frame();
				}
				else
				{
					// The module has already been loaded. Remember it, so we can import
					// variables from it if needed.
					last_module_ = v->as_module();
				}
				goto loop_back;
			}
			case opcodes_type::CODE_IMPORT_VARIABLE:
			{
				auto variable = function->get_constant(read_short());
				gal_assert(last_module_, "Should have already imported module.");
				auto result = last_module_->get_variable(*variable.as_string());
				if (fiber->has_error())
				{
					if (runtime_error()) { return gal_interpret_result::RESULT_RUNTIME_ERROR; }
				}

				fiber->stack_push(result);
				goto loop_back;
			}
			case opcodes_type::CODE_END:
			{
				// A CODE_END should always be preceded by a CODE_RETURN. If we get here,
				// the compiler generated wrong code.
				UNREACHABLE();
			}
		}

		// We should only exit this function from an explicit return from CODE_RETURN
		// or a runtime error.
		UNREACHABLE();
		return gal_interpret_result::RESULT_RUNTIME_ERROR;
	}

	bool gal_virtual_machine_state::validate_helper(const char *arg_name, const char *requires_type)
	{
		auto error = object::ctor<object_string>(*this);
		std_format::format_to(error->get_appender(), "{} must be a {}.", arg_name, requires_type);
		fiber_->set_error(error->operator magic_value());
		return false;
	}

	bool gal_virtual_machine_state::validate_function(magic_value arg, const char *arg_name)
	{
		if (arg.is_closure())
		{
			return true;
		}

		return validate_helper(arg_name, "closure");
	}

	bool gal_virtual_machine_state::validate_number(magic_value arg, const char *arg_name)
	{
		if (arg.is_number())
		{
			return true;
		}

		return validate_helper(arg_name, "number");
	}

	bool gal_virtual_machine_state::validate_int_value(double value, const char *arg_name)
	{
		if (std::trunc(value) == value)
		{
			return true;
		}

		return validate_helper(arg_name, "integer");
	}

	bool gal_virtual_machine_state::validate_int(magic_value arg, const char *arg_name)
	{
		// Make sure it's a number first.
		if (not validate_number(arg, arg_name))
		{
			return false;
		}

		return validate_int_value(arg.as_number(), arg_name);
	}

	bool gal_virtual_machine_state::validate_key(magic_value arg)
	{
		if (object_map::is_valid_key(arg))
		{
			return true;
		}

		return validate_helper("Map's key", "value type");
	}

	gal_index_type gal_virtual_machine_state::validate_index_value(double value, gal_size_type count, const char *arg_name)
	{
		if (not validate_int_value(value, arg_name))
		{
			return gal_index_not_exist;
		}

		// Negative indices count from the end.
		if (value < 0)
		{
			value = static_cast<double>(count) + value;
		}

		// Check bounds.
		if (value >= 0 && value < static_cast<double>(count))
		{
			return static_cast<gal_index_type>(value);
		}

		auto error = object::ctor<object_string>(*this);
		std_format::format_to(error->get_appender(), "{} out of bound {}.", arg_name, count);
		fiber_->set_error(error->operator magic_value());
		return gal_index_not_exist;
	}

	gal_index_type gal_virtual_machine_state::validate_index(magic_value arg, gal_size_type count, const char *arg_name)
	{
		if (not validate_number(arg, arg_name))
		{
			return gal_index_not_exist;
		}

		return validate_index_value(arg.as_number(), count, arg_name);
	}

	bool gal_virtual_machine_state::validate_string(magic_value arg, const char *arg_name)
	{
		if (arg.is_string())
		{
			return true;
		}

		return validate_helper(arg_name, "string");
	}

	void gal_virtual_machine_state::validate_slot(gal_slot_type slot) const
	{
		// gal_assert(slot >= 0, "Slot cannot be negative.");
		gal_assert(slot < get_slot_count(), "Not that many slots.");
	}

	void gal_virtual_machine_state::ensure_slot(gal_slot_type slots)
	{
		// If we don't have a fiber accessible, create one for the API to use.
		if (not api_stack_)
		{
			fiber_	   = make_object<object_fiber>(*this, nullptr);
			api_stack_ = fiber_->get_stack_bottom();
		}

		const auto current = fiber_->get_current_stack_size();
		if (current >= slots)
		{
			return;
		}

		// Grow the stack if needed.
		const auto needed = api_stack_ - fiber_->get_stack_bottom() + slots;
		fiber_->ensure_stack(*this, needed);

		fiber_->set_stack_top(api_stack_ + slots);
	}

	gal_object_type gal_virtual_machine_state::get_slot_type(gal_slot_type slot) const
	{
		validate_slot(slot);

		auto target = get_slot_value(slot);
		if (target.is_boolean())
		{
			return gal_object_type::BOOLEAN_TYPE;
		}
		if (target.is_number())
		{
			return gal_object_type::NUMBER_TYPE;
		}
		if (target.is_outer())
		{
			return gal_object_type::OUTER_TYPE;
		}
		if (target.is_list())
		{
			return gal_object_type::LIST_TYPE;
		}
		if (target.is_map())
		{
			return gal_object_type::MAP_TYPE;
		}
		if (target.is_null())
		{
			return gal_object_type::NULL_TYPE;
		}
		if (target.is_string())
		{
			return gal_object_type::STRING_TYPE;
		}

		return gal_object_type::UNKNOWN_TYPE;
	}

	void gal_virtual_machine_state::finalize_outer(object_outer &outer) const
	{
		// TODO: Don't look up every time.
		auto symbol = method_names_.find(gal_configuration::gal_outer_class_method::finalize_symbol_name, gal_configuration::gal_outer_class_method::finalize_symbol_name_length);
		gal_assert(symbol != gal_index_not_exist, "Should have defined <finalize> symbol.");

		// If there are no finalizers, don't finalize it.
		if (symbol == gal_index_not_exist)
		{
			return;
		}

		// If the class doesn't have a finalizer, bail out.
		auto *obj = outer.get_class();
		if (std::cmp_greater_equal(symbol, obj->get_methods_size()))
		{
			return;
		}

		auto &method = obj->get_method(symbol);
		if (method.type == method_type::none_type)
		{
			return;
		}

		gal_assert(method.type == method_type::outer_type, "Finalizer should be outer.");

		auto finalizer = reinterpret_cast<gal_finalize_function_type>(method.as.outer_method_function);
		finalizer(outer.get_data());
	}

	gal_handle *gal_virtual_machine_state::make_handle(magic_value value)
	{
		return &handles_.emplace_front(value);
	}

	object_closure *gal_virtual_machine_state::compile_source(const char *module, const char *source, bool is_expression, bool print_errors)
	{
		magic_value name = module ? object_string{*this, module, std::strlen(module)}.operator magic_value() : magic_value_null;

		return compile_in_module(name, source, is_expression, print_errors);
	}

	magic_value gal_virtual_machine_state::get_module_variable(magic_value module_name, const object_string &variable_name)
	{
		auto *module = get_module(module_name);
		if (not module)
		{
			auto error = object::ctor<object_string>(*this);
			std_format::format_to(error->get_appender(), "Module '{}' is not loaded.", module_name.as_string()->str());
			fiber_->set_error(error->operator magic_value());
			return magic_value_null;
		}
		return module->get_variable(variable_name);
	}

	magic_value gal_virtual_machine_state::get_module_variable(magic_value module_name, object_string::const_pointer variable_name)
	{
		auto *module = get_module(module_name);
		if (not module)
		{
			auto error = object::ctor<object_string>(*this);
			std_format::format_to(error->get_appender(), "Module '{}' is not loaded.", module_name.as_string()->str());
			fiber_->set_error(error->operator magic_value());
			return magic_value_null;
		}
		return module->get_variable(variable_name);
	}

	gal_virtual_machine::gal_virtual_machine(gal_configuration configuration)
		: state{*new gal_virtual_machine_state{configuration}}
	{
	}

	gal_virtual_machine::~gal_virtual_machine() noexcept
	{
		// todo: free them all

		delete &state;
	}

	void gal_virtual_machine::gc()
	{
		// todo
	}

	gal_interpret_result gal_virtual_machine::interpret(const char *module, const char *source)
	{
		auto *closure = state.compile_source(module, source, false, true);
		if (not closure)
		{
			return gal_interpret_result::RESULT_COMPILE_ERROR;
		}

		return state.run_interpreter(state.make_object<object_fiber>(state, closure));
	}

	gal_handle *gal_virtual_machine::make_callable_handle(const char *signature)
	{
		gal_assert(signature, "Signature cannot be nullptr.");

		auto signature_length = std::strlen(signature);
		gal_assert(signature_length > 0, "Signature cannot be empty.");

		// todo: check global module
		auto		 *global_module = state.get_module(magic_value_null);

		// todo: pattern

		// Count the number parameters the method expects.
		gal_size_type num_params	= 0;
		if (signature[signature_length - 1] == ')')
		{
			for (auto i = signature_length - 1; i > 0 && signature[i] != '('; --i)
			{
				if (signature[i] == '_')
				{
					++num_params;
				}
			}
		}

		// Count subscript arguments.
		if (signature[0] == '[')
		{
			for (decltype(signature_length) i = 0; i < signature_length && signature[i] != ']'; ++i)
			{
				if (signature[i] == '_')
				{
					++num_params;
				}
			}
		}

		// Add the signature to the method table.
		auto  method   = state.method_names_.ensure(state, signature, signature_length);

		// Create a little stub function that assumes the arguments are on the stack
		// and calls the method.
		auto *function = object::ctor<object_function>(state, *global_module, num_params + 1);

		// Wrap the function in a closure and then in a handle. Do this here, so it
		// doesn't get collected as we fill it in.
		auto *handle   = state.make_handle(object::ctor<object_closure>(state, *function)->operator magic_value());

		function->append_code(code_to_scalar(opcodes_type::CODE_CALL_0) + num_params).append_code((method >> 8) & 0xff).append_code(method & 0xff).append_code(code_to_scalar(opcodes_type::CODE_RETURN)).append_code(code_to_scalar(opcodes_type::CODE_END));

		function->set_name(signature);

		return handle;
	}

	gal_interpret_result gal_virtual_machine::call_handle(gal_handle &method)
	{
		gal_assert(method.value.is_closure(), "Method must be a method handle.");
		gal_assert(state.fiber_, "Must set up arguments for call first.");
		gal_assert(state.api_stack_, "Must set up arguments for call first.");
		gal_assert(not state.fiber_->has_frame(), "Can not call from a outer method.");

		auto *closure = method.value.as_closure();
		gal_assert(
				std::cmp_greater_equal(state.fiber_->get_current_stack_size(), closure->get_function().get_parameters_arity()),
				"Stack must have enough arguments for method.");

		// Clear the API stack. Now that call_handle() has control, we no longer need
		// it. We use this being non-null to tell if re-entrant calls to outer
		// methods are happening, so it's important to clear it out now so that you
		// can call outer methods from within calls to call_handle().
		// todo: release existing api_stack
		state.shutdown_stack();

		// Discard any extra temporary slots. We take for granted that the stub
		// function has exactly one slot for each argument.
		state.fiber_->stack_top_rebase(closure->get_function().get_slots_size());

		state.fiber_->call_function(state, *closure, 0);
		auto result = state.run_interpreter(state.fiber_);

		// If the call didn't abort, then set up the API stack to point to the
		// beginning of the stack so the host can access the call's return value.
		if (state.fiber_)
		{
			state.api_stack_ = state.fiber_->get_stack_bottom();
		}

		return result;
	}

	void gal_virtual_machine::release_handle(gal_handle &handle)
	{
		gal_assert(not state.handles_.empty(), "Cannot release handles not created by gal_virtual_machine::make_callable_handle");

		auto prev	 = state.handles_.before_begin();
		auto current = state.handles_.begin();
		for (
				;
				current != state.handles_.end() && current->value != handle.value;
				++current, ++prev)
		{
		}

		if (current->value.is_object())
		{
			object::dtor(current->value.as_object());
		}
		state.handles_.erase_after(prev);
	}

	gal_size_type gal_virtual_machine::get_slot_count() const noexcept
	{
		return state.get_slot_count();
	}

	void gal_virtual_machine::ensure_slots(gal_slot_type slots)
	{
		state.ensure_slot(slots);
	}

	gal_object_type gal_virtual_machine::get_slot_type(gal_slot_type slot) const
	{
		return state.get_slot_type(slot);
	}

	bool gal_virtual_machine::get_slot_boolean(gal_slot_type slot)
	{
		auto target = state.get_slot_value(slot);
		gal_assert(target.is_boolean(), "Slot must hold a bool.");
		return target.as_boolean();
	}

	const char *gal_virtual_machine::get_slot_bytes(gal_slot_type slot, gal_size_type &length)
	{
		auto target = state.get_slot_value(slot);
		gal_assert(target.is_string(), "Slot must hold a string.");

		auto *string = target.as_string();
		length		 = string->size();
		return string->data();
	}

	double gal_virtual_machine::get_slot_number(gal_slot_type slot)
	{
		auto target = state.get_slot_value(slot);
		gal_assert(target.is_number(), "Slot must hold a number.");
		return target.as_number();
	}

	void *gal_virtual_machine::get_slot_outer(gal_slot_type slot)
	{
		auto target = state.get_slot_value(slot);
		gal_assert(target.is_outer(), "Slot must hold a outer instance.");
		return target.as_outer()->get_data();
	}

	const char *gal_virtual_machine::get_slot_string(gal_slot_type slot)
	{
		auto target = state.get_slot_value(slot);
		gal_assert(target.is_string(), "Slot must hold a string.");
		return target.as_string()->data();
	}

	gal_handle *gal_virtual_machine::get_slot_handle(gal_slot_type slot)
	{
		auto target = state.get_slot_value(slot);
		return state.make_handle(target);
	}

	void gal_virtual_machine::set_slot_boolean(gal_slot_type slot, bool value)
	{
		state.set_slot_value(slot, to_magic_value(value));
	}

	void gal_virtual_machine::set_slot_bytes(gal_slot_type slot, const char *bytes, gal_size_type length)
	{
		gal_assert(bytes, "Byte array cannot be nullptr.");
		state.set_slot_value(slot, object::ctor<object_string>(state, bytes, length)->operator magic_value());
	}

	void gal_virtual_machine::set_slot_number(gal_slot_type slot, double value)
	{
		state.set_slot_value(slot, to_magic_value(value));
	}

	gal_row_pointer_type gal_virtual_machine::set_slot_outer(gal_slot_type slot, gal_slot_type class_slot, gal_size_type size)
	{
		auto target = state.get_slot_value(class_slot);
		gal_assert(target.is_class(), "Slot must hold a class.");

		auto *obj_class = target.as_class();
		gal_assert(obj_class->is_outer_class(), "Class must be a outer class.");

		auto *outer = object::ctor<object_outer>(obj_class, size);
		state.set_slot_value(slot, outer->operator magic_value());

		return outer->get_data();
	}

	void gal_virtual_machine::set_slot_list(gal_slot_type slot)
	{
		state.set_slot_value(slot, object::ctor<object_list>(state)->operator magic_value());
	}

	void gal_virtual_machine::set_slot_map(gal_slot_type slot)
	{
		state.set_slot_value(slot, object::ctor<object_map>(state)->operator magic_value());
	}

	void gal_virtual_machine::set_slot_null(gal_slot_type slot)
	{
		state.set_slot_value(slot, magic_value_null);
	}

	void gal_virtual_machine::set_slot_string(gal_slot_type slot, const char *text, gal_size_type length)
	{
		gal_assert(text, "String cannot be nullptr.");
		state.set_slot_value(slot, object::ctor<object_string>(state, text, length)->operator magic_value());
	}

	void gal_virtual_machine::set_slot_handle(gal_slot_type slot, gal_handle &handle)
	{
		state.set_slot_value(slot, handle.value);
	}

	gal_size_type gal_virtual_machine::get_list_size(gal_slot_type slot)
	{
		const auto target = state.get_slot_value(slot);
		gal_assert(target.is_list(), "Slot must hold a list.");
		return target.as_list()->size();
	}

	void gal_virtual_machine::get_list_element(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot)
	{
		const auto target = state.get_slot_value(list_slot);
		gal_assert(target.is_list(), "Slot must hold a list.");

		const auto *list	   = target.as_list();

		const auto	real_index = index_to_size(list->size(), index);
		state.set_slot_value(element_slot, list->get(real_index));
	}

	void gal_virtual_machine::set_list_element(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot)
	{
		auto target = state.get_slot_value(list_slot);
		gal_assert(target.is_list(), "Slot must hold a list.");

		auto		 *list		  = target.as_list();

		const auto real_index = index_to_size(list->size(), index);
		list->set(real_index, state.get_slot_value(element_slot));
	}

	void gal_virtual_machine::insert_to_list(gal_slot_type list_slot, gal_index_type index, gal_slot_type element_slot)
	{
		auto target = state.get_slot_value(list_slot);
		gal_assert(target.is_list(), "Slot must hold a list.");

		auto		 *list		  = target.as_list();

		const auto real_index = index_to_size<false>(list->size(), index);
		gal_assert(std::cmp_less_equal(real_index, list->size()), "Index out bounds.");

		list->insert(real_index, state.get_slot_value(element_slot));
	}

	gal_size_type gal_virtual_machine::get_map_size(gal_slot_type slot)
	{
		const auto target = state.get_slot_value(slot);
		gal_assert(target.is_map(), "Slot must hold a map.");

		return target.as_map()->size();
	}

	bool gal_virtual_machine::get_map_contains_key(gal_slot_type map_slot, gal_slot_type key_slot)
	{
		const auto target = state.get_slot_value(map_slot);
		gal_assert(target.is_map(), "Slot must hold a map.");

		const auto key = state.get_slot_value(key_slot);
		if (not state.validate_key(key))
		{
			return false;
		}

		return target.as_map()->get(key) != magic_value_undefined;
	}

	void gal_virtual_machine::get_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot)
	{
		const auto target = state.get_slot_value(map_slot);
		gal_assert(target.is_map(), "Slot must hold a map.");

		const auto key	 = state.get_slot_value(key_slot);
		const auto value = target.as_map()->get(key);
		state.set_slot_value(value_slot, value == magic_value_undefined ? magic_value_null : value);
	}

	void gal_virtual_machine::set_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot)
	{
		const auto target = state.get_slot_value(map_slot);
		gal_assert(target.is_map(), "Slot must hold a map.");

		const auto key = state.get_slot_value(key_slot);
		gal_assert(object_map::is_valid_key(key), "Map key must be a value type.");

		if (not state.validate_key(key))
		{
			return;
		}

		const auto value = state.get_slot_value(value_slot);
		target.as_map()->set(key, value);
	}

	void gal_virtual_machine::erase_map_value(gal_slot_type map_slot, gal_slot_type key_slot, gal_slot_type value_slot)
	{
		const auto target = state.get_slot_value(map_slot);
		gal_assert(target.is_map(), "Slot must hold a map.");

		const auto key = state.get_slot_value(key_slot);
		gal_assert(object_map::is_valid_key(key), "Map key must be a value type.");

		if (not state.validate_key(key))
		{
			return;
		}

		const auto removed = target.as_map()->remove(key);
		state.set_slot_value(value_slot, removed);
	}

	void gal_virtual_machine::get_variable(const char *module_name, const char *variable_name, gal_slot_type slot)
	{
		gal_assert(module_name, "Module name cannot be nullptr.");
		gal_assert(variable_name, "Variable name cannot be nullptr");

		const auto module_name_str																		= object_string{state, module_name, std::strlen(module_name)}.operator magic_value();

		const auto																			   *module = state.get_module(module_name_str);
		gal_assert(module, "Could not find module.");

		const auto variable = module->get_variable(variable_name);
		gal_assert(variable != magic_value_undefined, "Count not find variable.");

		state.set_slot_value(slot, variable);
	}

	bool gal_virtual_machine::has_variable(const char *module_name, const char *variable_name)
	{
		gal_assert(module_name, "Module name cannot be nullptr.");
		gal_assert(variable_name, "Variable name cannot be nullptr");

		const auto module_name_str																		= object_string{state, module_name, std::strlen(module_name)}.operator magic_value();

		const auto																			   *module = state.get_module(module_name_str);
		gal_assert(module, "Could not find module.");

		const auto variable = module->get_variable(variable_name);
		return variable != magic_value_undefined;
	}

	bool gal_virtual_machine::has_module(const char *module_name)
	{
		gal_assert(module_name, "Module name cannot be nullptr.");

		const auto module_name_str																		= object_string{state, module_name, std::strlen(module_name)}.operator magic_value();

		const auto																			   *module = state.get_module(module_name_str);

		return module != nullptr;
	}

	void gal_virtual_machine::abort_fiber(gal_slot_type slot)
	{
		state.fiber_->set_error(state.get_slot_value(slot));
	}

	gal_row_pointer_type gal_virtual_machine::get_user_data()
	{
		return state.configuration_.user_data;
	}

	void gal_virtual_machine::set_user_data(gal_row_pointer_type user_data)
	{
		state.configuration_.user_data = user_data;
	}
}// namespace gal
