#include <charconv>
#include <vm/meta.hpp>
#include <vm/vm.hpp>

namespace gal
{
	meta_object& meta_object::instance(gal_virtual_machine_state& state)
	{
		static meta_object o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_object::operator_not(magic_value* values)
	{
		values[0] = magic_value_false;
	}

	void meta_object::operator_eq(magic_value* values)
	{
		values[0] = (values[0].equal(values[1]) ? magic_value_true : magic_value_false);
	}

	void meta_object::operator_not_eq(magic_value* values)
	{
		values[0] = (values[0].equal(values[1]) ? magic_value_false : magic_value_true);
	}

	void meta_object::operator_instance_of(gal_virtual_machine_state& state, magic_value* values)
	{
		if (not values[0].is_class())
		{
			auto error = object::ctor<object_string>(state);
			std_format::format_to(error->get_appender(), "Right operand must be a class.");
			state.fiber_->set_error(error->operator magic_value());
			return;
		}

		auto* obj_class	 = &get_meta_class(values[0]);
		auto* base_class = values[1].as_class();

		// Walk the superclass chain looking for the class.
		do {
			if (obj_class == base_class)
			{
				values[0] = magic_value_true;
			}
			obj_class = obj_class->get_class();
		} while (obj_class);

		values[0] = magic_value_false;
	}

	void meta_object::operator_to_string(magic_value* values)
	{
		values[0] = values[0].as_object()->get_class()->get_class_name().operator magic_value();
	}

	void meta_object::operator_typeof(magic_value* values)
	{
		values[0] = get_meta_class(values[0]).operator magic_value();
	}

	meta_class& meta_class::instance(gal_virtual_machine_state& state)
	{
		static meta_class o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_class::operator_nameof(magic_value* values)
	{
		values[0] = values[0].as_class()->get_class_name().operator magic_value();
	}

	void meta_class::operator_super_type(magic_value* values)
	{
		const auto* obj_class = values[0].as_class();
		// Object has no superclass.
		if (const auto* super = obj_class->get_super_class(); super)
		{
			values[0] = super->operator magic_value();
		}
		else
		{
			values[0] = magic_value_null;
		}
	}

	void meta_class::operator_to_string(magic_value* values)
	{
		values[0] = values[0].as_class()->get_class_name().operator magic_value();
	}

	void meta_class::operator_attributes(magic_value* values)
	{
		values[0] = values[0].as_class()->get_attributes();
	}

	meta_object_metaclass& meta_object_metaclass::instance(gal_virtual_machine_state& state)
	{
		static meta_object_metaclass o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_object_metaclass::operator_is_same(magic_value* values)
	{
		values[0] = values[1].equal(values[2]) ? magic_value_true : magic_value_false;
	}

	meta_boolean& meta_boolean::instance(gal_virtual_machine_state& state)
	{
		static meta_boolean o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_boolean::operator_not(magic_value* values)
	{
		values[0] = values[0].as_boolean() ? magic_value_false : magic_value_true;
	}

	void meta_boolean::operator_to_string(gal_virtual_machine_state& state, magic_value* values)
	{
		if (values[0].as_boolean())
		{
			static object_string true_str{state, true_name, true_name_length};
			values[0] = true_str.operator magic_value();
		}
		else
		{
			static object_string  false_str{state, false_name, false_name_length};
			values[0] = false_str.operator magic_value();
		}
	}

	meta_fiber& meta_fiber::instance(gal_virtual_machine_state& state)
	{
		static meta_fiber o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_fiber::operator_new(gal_virtual_machine_state& state, magic_value* values)
	{
		if (not state.validate_function(values[1], "Argument"))
		{
			return;
		}

		auto* closure = values[0].as_closure();
		if (closure->get_function().get_parameters_arity() > 1)
		{
			auto error = object::ctor<object_string>(state);
			std_format::format_to(error->get_appender(), "Function cannot take more than one parameter.");
			state.fiber_->set_error(error->operator magic_value());
			return;
		}

		values[0] = state.make_object<object_fiber>(state, closure)->operator magic_value();
	}

	void meta_fiber::operator_abort(gal_virtual_machine_state& state, magic_value* values)
	{
		state.fiber_->set_error(values[1]);
	}

	void meta_fiber::operator_current(gal_virtual_machine_state& state, magic_value* values)
	{
		values[0] = state.fiber_->operator magic_value();
	}

	void meta_fiber::operator_suspend(gal_virtual_machine_state& state)
	{
		// Switching to a null fiber tells the interpreter to stop and exit.
		state.fiber_ = nullptr;
		state.shutdown_stack();
	}

	void meta_fiber::operator_yield_no_args(gal_virtual_machine_state& state)
	{
		auto* current = state.fiber_;
		state.fiber_  = current->get_caller();

		// Unhook this fiber from the one that called it.
		current->set_caller(nullptr);
		current->set_state(fiber_state::other_state);

		if (state.fiber_)
		{
			// Make the caller's run method return null.
			state.fiber_->set_stack_point(1, magic_value_null);
		}
	}

	void meta_fiber::operator_yield_has_args(gal_virtual_machine_state& state, magic_value* values)
	{
		auto* current = state.fiber_;
		state.fiber_  = current->get_caller();

		// Unhook this fiber from the one that called it.
		current->set_caller(nullptr);
		current->set_state(fiber_state::other_state);

		if (state.fiber_)
		{
			// Make the caller's run method return the argument passed to yield.
			state.fiber_->set_stack_point(1, values[1]);

			// When the yielding fiber resumes, we'll store the result of the yield
			// call in its stack. Since Fiber.yield(value) has two arguments (the Fiber
			// class and the value) and we only need one slot for the result, discard
			// the other slot now.
			current->stack_drop();
		}
	}

	/**
	 * @brief Transfer execution to [fiber(values[0])] coming from the current fiber whose stack has [values(values[1:...])].
	 *
	 * [is_call] is true if [fiber] is being called and not transferred.
	 * [has_value] is true if a value in [args] is being passed to the new fiber.
	 * Otherwise, `null` is implicitly being passed.
	 */
	namespace
	{
		template<bool IsCall, bool HasValue>
		void fiber_run(gal_virtual_machine_state& state, magic_value* values, const char* verb)
		{
			auto& fiber = *values[0].as_fiber();

			if (fiber.has_error())
			{
				auto error = object::ctor<object_string>(state);
				std_format::format_to(error->get_appender(), "Cannot {} an aborted fiber.", verb);
				state.fiber_->set_error(error->operator magic_value());
				return;
			}

			if constexpr (IsCall)
			{
				// You can't call a called fiber, but you can transfer directly to it,
				// which is why this check is gated on `is_call`. This way, after resuming a
				// suspended fiber, it will run and then return to the fiber that called it
				// and so on.
				if (fiber.has_caller())
				{
					auto error = object::ctor<object_string>(state);
					std_format::format_to(error->get_appender(), "Fiber has already been called.");
					state.fiber_->set_error(error->operator magic_value());
					return;
				}

				if (fiber.get_state() == fiber_state::root_state)
				{
					auto error = object::ctor<object_string>(state);
					std_format::format_to(error->get_appender(), "Cannot call root fiber.");
					state.fiber_->set_error(error->operator magic_value());
					return;
				}

				// Remember who ran it.
				fiber.set_caller(state.fiber_);
			}

			if (not fiber.has_frame())
			{
				auto error = object::ctor<object_string>(state);
				std_format::format_to(error->get_appender(), "Cannot {} an finished fiber.", verb);
				state.fiber_->set_error(error->operator magic_value());
				return;
			}

			// When the calling fiber resumes, we'll store the result of the call in its
			// stack. If the call has two arguments (the fiber and the value), we only
			// need one slot for the result, so discard the other slot now.
			if constexpr (HasValue)
			{
				state.fiber_->stack_drop();
			}

			if (fiber.get_frames_size() == 1 && fiber.get_recent_frame().closure->get_function().has_code())
			{
				// The fiber is being started for the first time. If its function takes a
				// parameter, bind an argument to it.
				if (fiber.get_recent_frame().closure->get_function().get_parameters_arity() == 1)
				{
					if constexpr (HasValue)
					{
						fiber.stack_push(values[1]);
					}
					else
					{
						fiber.stack_push(magic_value_null);
					}
				}
			}
			else
			{
				// The fiber is being resumed, make yield() or transfer() return the result.
				if constexpr (HasValue)
				{
					fiber.set_stack_point(1, values[1]);
				}
				else
				{
					fiber.stack_push(magic_value_null);
				}
			}

			state.fiber_ = &fiber;
		}
	}// namespace

	void meta_fiber::operator_call_no_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<true, false>(state, values, "call");
	}

	void meta_fiber::operator_call_has_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<true, true>(state, values, "call");
	}

	void meta_fiber::operator_error(magic_value* values)
	{
		values[0] = values[0].as_fiber()->get_error();
	}

	void meta_fiber::operator_done(magic_value* values)
	{
		const auto* fiber = values[0].as_fiber();
		values[0]		  = (not fiber->has_frame() || fiber->has_error()) ? magic_value_true : magic_value_false;
	}

	void meta_fiber::operator_transfer_no_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<false, false>(state, values, "transfer to");
	}

	void meta_fiber::operator_transfer_has_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<false, true>(state, values, "transfer to");
	}

	void meta_fiber::operator_transfer_error(gal_virtual_machine_state& state, magic_value* values)
	{
		operator_transfer_has_args(state, values);
		state.fiber_->set_error(values[1]);
	}

	void meta_fiber::operator_try_no_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<true, false>(state, values, "try");

		// If we're switching to a valid fiber to try, remember that we're trying it.
		if (not state.fiber_->has_error())
		{
			state.fiber_->set_state(fiber_state::try_state);
		}
	}

	void meta_fiber::operator_try_has_args(gal_virtual_machine_state& state, magic_value* values)
	{
		fiber_run<true, true>(state, values, "try");

		// If we're switching to a valid fiber to try, remember that we're trying it.
		if (not state.fiber_->has_error())
		{
			state.fiber_->set_state(fiber_state::try_state);
		}
	}

	meta_function& meta_function::instance(gal_virtual_machine_state& state)
	{
		static meta_function o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_function::operator_new(gal_virtual_machine_state& state, magic_value* values)
	{
		if (not state.validate_function(values[1], "Argument"))
		{
			return;
		}

		// The block argument is already a function, so just return it.
		values[0] = values[1];
	}

	void meta_function::operator_arity(magic_value* values)
	{
		values[0] = magic_value{values[0].as_closure()->get_function().get_parameters_arity()};
	}

	namespace
	{
		void call_function(gal_virtual_machine_state& state, magic_value* values, gal_size_type num_args)
		{
			// +1 to include the function itself.
			state.fiber_->call_function(state, *values[0].as_closure(), num_args + 1);
		}
	}// namespace

	void meta_function::operator_call0(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 0);
	}

	void meta_function::operator_call1(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 1);
	}

	void meta_function::operator_call2(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 2);
	}

	void meta_function::operator_call3(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 3);
	}

	void meta_function::operator_call4(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 4);
	}

	void meta_function::operator_call5(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 5);
	}

	void meta_function::operator_call6(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 6);
	}

	void meta_function::operator_call7(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 7);
	}

	void meta_function::operator_call8(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 8);
	}

	void meta_function::operator_call9(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 9);
	}

	void meta_function::operator_call10(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 10);
	}

	void meta_function::operator_call11(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 11);
	}

	void meta_function::operator_call12(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 12);
	}

	void meta_function::operator_call13(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 13);
	}

	void meta_function::operator_call14(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 14);
	}

	void meta_function::operator_call15(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 15);
	}

	void meta_function::operator_call16(gal_virtual_machine_state& state, magic_value* values)
	{
		call_function(state, values, 16);
	}

	meta_null& meta_null::instance(gal_virtual_machine_state& state)
	{
		static meta_null o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_null::operator_not(magic_value* values)
	{
		values[0] = magic_value_true;
	}

	void meta_null::operator_to_string(gal_virtual_machine_state& state, magic_value* values)
	{
		static object_string null_str{state, null_name, null_name_length};
		values[0] = null_str.operator magic_value();
	}

	meta_number& meta_number::instance(gal_virtual_machine_state& state)
	{
		static meta_number o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	void meta_number::operator_from_string(gal_virtual_machine_state& state, magic_value* values)
	{
		if (not state.validate_string(values[1], "Argument"))
		{
			return;
		}

		const auto* string = values[1].as_string();

		// Corner case: Can't parse an empty string.
		if (string->empty())
		{
			values[0] = magic_value_null;
		}

		double	   value{0};
		const auto result = std::from_chars(string->data(), string->data() + string->size(), value);
		if (result.ec == std::errc::result_out_of_range)
		{
			auto error = object::ctor<object_string>(state, "Number literal is too large.", std::strlen("Number literal is too large.") - 1);
			state.fiber_->set_error(error->operator magic_value());
			return;
		}

		if (result.ptr != string->data() + string->size())
		{
			// We must have consumed the entire string. Otherwise, it contains non-number
			// characters, and we can't parse it.
			values[0] = magic_value_null;
		}

		values[0] = magic_value{value};
	}
}// namespace gal
