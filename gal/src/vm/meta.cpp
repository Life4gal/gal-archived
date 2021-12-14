#include <charconv>
#include <vm/meta.hpp>
#include <vm/vm.hpp>
#include <limits>
#include <functional>

namespace gal
{
	meta_object& meta_object::instance(const gal_virtual_machine_state& state)
	{
		static meta_object o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_object::operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(magic_value_false);
		return true;
	}

	bool meta_object::operator_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[0].equal(args[1]) ? magic_value_true : magic_value_false);
		return true;
	}

	bool meta_object::operator_not_eq([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[0].equal(args[1]) ? magic_value_false : magic_value_true);
		return true;
	}

	bool meta_object::operator_instance_of(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not args[0].is_class())
		{
			const auto error = object::create<object_string>(state);
			std_format::format_to(error->get_appender(), "Right operand must be a class.");
			state.fiber_->set_error(error->operator magic_value());
			return false;
		}

		auto* obj_class = &get_meta_class(args[0]);
		auto* base_class = args[1].as_class();

		// Walk the superclass chain looking for the class.
		do
		{
			if (obj_class == base_class)
			{
				args[0].discard_set(magic_value_true);
				return true;
			}
			obj_class = obj_class->get_class();
		} while (obj_class);

		args[0].discard_set(magic_value_false);
		return true;
	}

	bool meta_object::operator_to_string([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].as_object()->get_class()->get_class_name().operator magic_value());
		return true;
	}

	bool meta_object::operator_typeof([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(get_meta_class(args[0]).operator magic_value());
		return true;
	}

	meta_class& meta_class::instance(const gal_virtual_machine_state& state)
	{
		static meta_class o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_class::operator_nameof([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].as_class()->get_class_name().operator magic_value());
		return true;
	}

	bool meta_class::operator_super_type([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		const auto* obj_class = args[1].as_class();
		// Object has no superclass.
		if (const auto* super = obj_class->get_super_class(); super) { args[0].discard_set(super->operator magic_value()); }
		else { args[0].discard_set(magic_value_null); }
		return true;
	}

	bool meta_class::operator_to_string([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].as_class()->get_class_name().operator magic_value());
		return true;
	}

	bool meta_class::operator_attributes([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].as_class()->get_attributes());
		return true;
	}

	meta_object_metaclass& meta_object_metaclass::instance(const gal_virtual_machine_state& state)
	{
		static meta_object_metaclass o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_object_metaclass::operator_is_same([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].equal(args[2]) ? magic_value_true : magic_value_false);
		return true;
	}

	meta_boolean& meta_boolean::instance(const gal_virtual_machine_state& state)
	{
		static meta_boolean o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_boolean::operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[0].as_boolean() ? magic_value_false : magic_value_true);
		return true;
	}

	bool meta_boolean::operator_to_string(const gal_virtual_machine_state& state, magic_value* args)
	{
		if (args[0].as_boolean())
		{
			static object_string true_str{state, true_name, true_name_length};
			args[0] = true_str.operator magic_value();
		}
		else
		{
			static object_string false_str{state, false_name, false_name_length};
			args[0] = false_str.operator magic_value();
		}
		return true;
	}

	meta_fiber& meta_fiber::instance(const gal_virtual_machine_state& state)
	{
		static meta_fiber o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_fiber::operator_new(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_function(args[0], "Argument")) { return false; }

		auto* closure = args[0].as_closure();
		if (closure->get_function().get_parameters_arity() > 1)
		{
			const auto error = object::create<object_string>(state);
			std_format::format_to(error->get_appender(), "Function cannot take more than one parameter.");
			state.fiber_->set_error(error->operator magic_value());
			return false;
		}

		// fiber will hold the closure at the stack bottom, so we should not destroy the closure.
		args[0] = state.make_object<object_fiber>(state, closure)->operator magic_value();
		return true;
	}

	bool meta_fiber::operator_abort(const gal_virtual_machine_state& state, const magic_value* args)
	{
		state.fiber_->set_error(args[1]);

		// If the error is explicitly null, it's not really an abort.
		return args[1].is_null();
	}

	bool meta_fiber::operator_current(const gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(state.fiber_->operator magic_value());
		return true;
	}

	bool meta_fiber::operator_suspend(gal_virtual_machine_state& state, [[maybe_unused]] magic_value* args)
	{
		// Switching to a null fiber tells the interpreter to stop and exit.
		state.fiber_ = nullptr;
		state.shutdown_stack();
		return false;
	}

	bool meta_fiber::operator_yield_no_args(gal_virtual_machine_state& state, [[maybe_unused]] magic_value* args)
	{
		auto* current = state.fiber_;
		state.fiber_ = current->get_caller();

		// Unhook this fiber from the one that called it.
		current->set_caller(nullptr);
		current->set_state(fiber_state::other_state);

		if (state.fiber_)
		{
			// Make the caller's run method return null.
			state.fiber_->set_stack_point(1, magic_value_null);
		}
		return false;
	}

	bool meta_fiber::operator_yield_has_args(gal_virtual_machine_state& state, const magic_value* args)
	{
		auto* current = state.fiber_;
		state.fiber_ = current->get_caller();

		// Unhook this fiber from the one that called it.
		current->set_caller(nullptr);
		current->set_state(fiber_state::other_state);

		if (state.fiber_)
		{
			// Make the caller's run method return the argument passed to yield.
			state.fiber_->set_stack_point(1, args[1]);

			// When the yielding fiber resumes, we'll store the result of the yield
			// call in its stack. Since Fiber.yield(value) has two arguments (the Fiber
			// class and the value) and we only need one slot for the result, discard
			// the other slot now.
			current->stack_drop();
		}
		return false;
	}

	/**
	 * @brief Transfer execution to [fiber(args[0])] coming from the current fiber whose stack has [args(args[1:...])].
	 *
	 * [is_call] is true if [fiber] is being called and not transferred.
	 * [has_value] is true if a value in [args] is being passed to the new fiber.
	 * Otherwise, `null` is implicitly being passed.
	 */
	namespace
	{
		template<bool IsCall, bool HasValue>
		bool fiber_run(gal_virtual_machine_state& state, magic_value* args, const char* verb)
		{
			auto& fiber = *args[0].as_fiber();

			if (fiber.has_error())
			{
				const auto error = object::create<object_string>(state);
				std_format::format_to(error->get_appender(), "Cannot {} an aborted fiber.", verb);
				state.fiber_->set_error(error->operator magic_value());
				return false;
			}

			if constexpr (IsCall)
			{
				// You can't call a called fiber, but you can transfer directly to it,
				// which is why this check is gated on `is_call`. This way, after resuming a
				// suspended fiber, it will run and then return to the fiber that called it
				// and so on.
				if (fiber.has_caller())
				{
					const auto error = object::create<object_string>(state);
					std_format::format_to(error->get_appender(), "Fiber has already been called.");
					state.fiber_->set_error(error->operator magic_value());
					return false;
				}

				if (fiber.get_state() == fiber_state::root_state)
				{
					const auto error = object::create<object_string>(state);
					std_format::format_to(error->get_appender(), "Cannot call root fiber.");
					state.fiber_->set_error(error->operator magic_value());
					return false;
				}

				// Remember who ran it.
				fiber.set_caller(state.fiber_);
			}

			if (not fiber.has_frame())
			{
				const auto error = object::create<object_string>(state);
				std_format::format_to(error->get_appender(), "Cannot {} an finished fiber.", verb);
				state.fiber_->set_error(error->operator magic_value());
				return false;
			}

			// When the calling fiber resumes, we'll store the result of the call in its
			// stack. If the call has two arguments (the fiber and the value), we only
			// need one slot for the result, so discard the other slot now.
			if constexpr (HasValue) { state.fiber_->stack_drop(); }

			if (fiber.get_frames_size() == 1 && fiber.get_recent_frame().closure->get_function().has_code())
			{
				// The fiber is being started for the first time. If its function takes a
				// parameter, bind an argument to it.
				if (fiber.get_recent_frame().closure->get_function().get_parameters_arity() == 1)
				{
					if constexpr (HasValue) { fiber.stack_push(args[1]); }
					else { fiber.stack_push(magic_value_null); }
				}
			}
			else
			{
				// The fiber is being resumed, make yield() or transfer() return the result.
				if constexpr (HasValue) { fiber.set_stack_point(1, args[1]); }
				else { fiber.stack_push(magic_value_null); }
			}

			state.fiber_ = &fiber;
			return false;
		}
	}// namespace

	bool meta_fiber::operator_call_no_args(gal_virtual_machine_state& state, magic_value* args) { return fiber_run<true, false>(state, args, "call"); }

	bool meta_fiber::operator_call_has_args(gal_virtual_machine_state& state, magic_value* args) { return fiber_run<true, true>(state, args, "call"); }

	bool meta_fiber::operator_error([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(args[1].as_fiber()->get_error());
		return true;
	}

	bool meta_fiber::operator_done([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		const auto* fiber = args[1].as_fiber();
		args[0].discard_set((not fiber->has_frame() || fiber->has_error()) ? magic_value_true : magic_value_false);
		return true;
	}

	bool meta_fiber::operator_transfer_no_args(gal_virtual_machine_state& state, magic_value* args) { return fiber_run<false, false>(state, args, "transfer to"); }

	bool meta_fiber::operator_transfer_has_args(gal_virtual_machine_state& state, magic_value* args) { return fiber_run<false, true>(state, args, "transfer to"); }

	bool meta_fiber::operator_transfer_error(gal_virtual_machine_state& state, magic_value* args)
	{
		operator_transfer_has_args(state, args);
		state.fiber_->set_error(args[1]);
		return false;
	}

	bool meta_fiber::operator_try_no_args(gal_virtual_machine_state& state, magic_value* args)
	{
		fiber_run<true, false>(state, args, "try");

		// If we're switching to a valid fiber to try, remember that we're trying it.
		if (not state.fiber_->has_error()) { state.fiber_->set_state(fiber_state::try_state); }
		return false;
	}

	bool meta_fiber::operator_try_has_args(gal_virtual_machine_state& state, magic_value* args)
	{
		fiber_run<true, true>(state, args, "try");

		// If we're switching to a valid fiber to try, remember that we're trying it.
		if (not state.fiber_->has_error()) { state.fiber_->set_state(fiber_state::try_state); }
		return false;
	}

	meta_function& meta_function::instance(const gal_virtual_machine_state& state)
	{
		static meta_function o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_function::operator_new(gal_virtual_machine_state& state, const magic_value* args)
	{
		if (not state.validate_function(args[0], "Argument")) { return false; }

		// The block argument is already a function, so just do nothing.
		return true;
	}

	bool meta_function::operator_arity([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0].discard_set(magic_value{args[1].as_closure()->get_function().get_parameters_arity()});
		return true;
	}

	namespace
	{
		bool call_function(gal_virtual_machine_state& state, const magic_value* args, const gal_size_type num_args)
		{
			// +1 to include the function itself.
			state.fiber_->call_function(state, *args[0].as_closure(), num_args + 1);
			return false;
		}
	}// namespace

	bool meta_function::operator_call0(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 0); }

	bool meta_function::operator_call1(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 1); }

	bool meta_function::operator_call2(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 2); }

	bool meta_function::operator_call3(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 3); }

	bool meta_function::operator_call4(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 4); }

	bool meta_function::operator_call5(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 5); }

	bool meta_function::operator_call6(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 6); }

	bool meta_function::operator_call7(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 7); }

	bool meta_function::operator_call8(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 8); }

	bool meta_function::operator_call9(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 9); }

	bool meta_function::operator_call10(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 10); }

	bool meta_function::operator_call11(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 11); }

	bool meta_function::operator_call12(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 12); }

	bool meta_function::operator_call13(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 13); }

	bool meta_function::operator_call14(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 14); }

	bool meta_function::operator_call15(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 15); }

	bool meta_function::operator_call16(gal_virtual_machine_state& state, const magic_value* args) { return call_function(state, args, 16); }

	meta_null& meta_null::instance(const gal_virtual_machine_state& state)
	{
		static meta_null o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	bool meta_null::operator_not([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value_true;
		return true;
	}

	bool meta_null::operator_to_string(const gal_virtual_machine_state& state, magic_value* args)
	{
		static object_string null_str{state, null_name, null_name_length};
		args[0] = null_str.operator magic_value();
		return true;
	}

	meta_number& meta_number::instance(const gal_virtual_machine_state& state)
	{
		static meta_number o{static_cast<gal_size_type>(0), object_string{state, name, name_length}};
		return o;
	}

	namespace 
	{
		template<std::floating_point T>
		constexpr bool float_eq(T lhs, std::type_identity_t<T> rhs) noexcept
		{
			return lhs - rhs <= std::numeric_limits<T>::epsilon() && rhs - lhs <= std::numeric_limits<T>::epsilon();
		}
	}

	bool meta_number::operator_eq(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not args[1].is_number())
		{
			args[0] = magic_value_false;
		}
		else
		{
			args[0] = float_eq(args[0].as_number(), args[1].as_number()) ? magic_value_true : magic_value_false;
		}
		return true;
	}

	bool meta_number::operator_not_eq(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not args[1].is_number())
		{
			args[0] = magic_value_true;
		}
		else
		{
			args[0] = float_eq(args[0].as_number(), args[1].as_number()) ? magic_value_false : magic_value_true;
		}
		return true;
	}

	bool meta_number::operator_from_string(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_string(args[1], "Argument")) { return false; }

		const auto* string = args[1].as_string();

		// Corner case: Can't parse an empty string.
		if (string->empty())
		{
			args[0].discard_set(magic_value_null);
			return true;
		}

		double value{0};
		const auto [ptr, ec] = std::from_chars(string->data(), string->data() + string->size(), value);
		if (ec == std::errc::result_out_of_range)
		{
			const auto error = object::create<object_string>(state, "Number literal is too large.", std::strlen("Number literal is too large.") - 1);
			state.fiber_->set_error(error->operator magic_value());
			return false;
		}

		if (ptr != string->data() + string->size())
		{
			// We must have consumed the entire string. Otherwise, it contains non-number
			// characters, and we can't parse it.
			args[0].discard_set(magic_value_null);
			return true;
		}

		args[0].discard_set(magic_value{value});
		return true;
	}

	bool meta_number::operator_infinity([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::numeric_limits<double>::infinity()};
		return true;
	}

	bool meta_number::operator_nan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::numeric_limits<double>::quiet_NaN()};
		return true;
	}

	bool meta_number::operator_pi([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{3.14159265358979323846264338327950288};
		return true;
	}

	bool meta_number::operator_tau([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{6.28318530717958647692528676655900577};
		return true;
	}

	bool meta_number::operator_max([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::numeric_limits<double>::max()};
		return true;
	}

	bool meta_number::operator_min([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::numeric_limits<double>::min()};
		return true;
	}

	bool meta_number::operator_max_safe_integer(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{9007199254740991.0};
		return true;
	}

	bool meta_number::operator_min_safe_integer(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{-9007199254740991.0};
		return true;
	}

	bool meta_number::operator_fraction(gal_virtual_machine_state& state, magic_value* args)
	{
		double dummy{};
		args[0] = magic_value{std::modf(args[0].as_number(), &dummy)};
		return true;
	}

	bool meta_number::operator_truncate(gal_virtual_machine_state& state, magic_value* args)
	{
		double ret{};
		std::modf(args[0].as_number(), &ret);
		args[0] = magic_value{ret};
		return true;
	}

	bool meta_number::operator_is_inf(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = std::isinf(args[0].as_number()) ? magic_value_true : magic_value_false;
		return true;
	}

	bool meta_number::operator_is_nan(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = std::isnan(args[0].as_number()) ? magic_value_true : magic_value_false;
		return true;
	}

	bool meta_number::operator_is_integer(gal_virtual_machine_state& state, magic_value* args)
	{
		if (const auto num = args[0].as_number();
			std::isinf(num) || std::isnan(num)) { args[0] = magic_value_false; }
		else { args[0] = std::trunc(num) == num ? magic_value_true : magic_value_false; }
		return true;
	}

	bool meta_number::operator_sign(gal_virtual_machine_state& state, magic_value* args)
	{
		if (const auto num = args[0].as_number(); num > 0) { args[0] = magic_value{decltype(num){1}}; }
		else if (num < 0) { args[0] = magic_value{decltype(num){-1}}; }
		else { args[0] = magic_value{decltype(num){0}}; }
		return true;
	}

	bool meta_number::operator_to_string(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = object::create<object_string>(state, args[0].as_number())->operator magic_value();
		return true;
	}

	namespace
	{
		template<typename Operator>
		bool mathematical_operation(gal_virtual_machine_state& state, magic_value* args)
		{
			if (not state.validate_number(args[1], "Right operand")) { return false; }
			args[0] = magic_value{Operator{}(args[0].as_number(), args[1].as_number())};
			return true;
		}
	}

	bool meta_number::operator_plus(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::plus<>>(state, args); }

	bool meta_number::operator_minus(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::minus<>>(state, args); }

	bool meta_number::operator_multiplies(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::multiplies<>>(state, args); }

	bool meta_number::operator_divides(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::divides<>>(state, args); }

	bool meta_number::operator_modulus(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::modulus<>>(state, args); }

	bool meta_number::operator_less(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::ranges::less>(state, args); }

	bool meta_number::operator_less_equal(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::ranges::less_equal>(state, args); }

	bool meta_number::operator_greater(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::ranges::greater>(state, args); }

	bool meta_number::operator_greater_equal(gal_virtual_machine_state& state, magic_value* args) { return mathematical_operation<std::ranges::greater_equal>(state, args); }

	namespace
	{
		template<typename Operator>
		bool bitwise_operation(gal_virtual_machine_state& state, magic_value* args)
		{
			if (not state.validate_number(args[1], "Right operand")) { return false; }

			const auto left = static_cast<magic_value::value_type>(args[0].as_number());
			const auto right = static_cast<magic_value::value_type>(args[0].as_number());
			args[0] = magic_value{Operator{}(left, right)};
			return true;
		}
	}

	bool meta_number::operator_bitwise_and(gal_virtual_machine_state& state, magic_value* args) { return bitwise_operation<std::bit_and<>>(state, args); }

	bool meta_number::operator_bitwise_or(gal_virtual_machine_state& state, magic_value* args) { return bitwise_operation<std::bit_or<>>(state, args); }

	bool meta_number::operator_bitwise_xor(gal_virtual_machine_state& state, magic_value* args) { return bitwise_operation<std::bit_xor<>>(state, args); }

	bool meta_number::operator_bitwise_left_shift(gal_virtual_machine_state& state, magic_value* args)
	{
		struct left_shift
		{
			constexpr auto operator()(const magic_value::value_type lhs, const magic_value::value_type rhs) const noexcept { return lhs << rhs; }
		};
		return bitwise_operation<left_shift>(state, args);
	}

	bool meta_number::operator_bitwise_right_shift(gal_virtual_machine_state& state, magic_value* args)
	{
		struct right_shift
		{
			constexpr auto operator()(const magic_value::value_type lhs, const magic_value::value_type rhs) const noexcept { return lhs >> rhs; }
		};
		return bitwise_operation<right_shift>(state, args);
	}

	bool meta_number::operator_bitwise_not(gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::bit_not{}(static_cast<magic_value::value_type>(args[0].as_number()))};
		return true;
	}

	bool meta_number::operator_cmp_min(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_number(args[1], "Right operand")) { return false; }

		args[0] = magic_value{std::ranges::min(args[0].as_number(), args[1].as_number())};
		return true;
	}

	bool meta_number::operator_cmp_max(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_number(args[1], "Right operand")) { return false; }

		args[0] = magic_value{std::ranges::max(args[0].as_number(), args[1].as_number())};
		return true;
	}

	bool meta_number::operator_clamp(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_number(args[1], "Min value")) { return false; }
		if (not state.validate_number(args[2], "Max value")) { return false; }

		args[0] = magic_value{std::ranges::clamp(args[0].as_number(), args[1].as_number(), args[2].as_number())};
		return true;
	}

	bool meta_number::operator_abs([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::abs(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_negate([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{-args[0].as_number()};
		return true;
	}

	bool meta_number::operator_sqrt([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::sqrt(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_pow(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_number(args[1], "Right operand")) { return false; }

		args[0] = magic_value{std::pow(args[0].as_number(), args[1].as_number())};
		return true;
	}

	bool meta_number::operator_cos([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::cos(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_sin([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::sin(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_tan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::tan(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_log([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::log(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_log2([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::log2(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_exp([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::exp(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_exp2([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::exp2(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_acos([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::acos(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_asin([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::asin(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_atan([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::atan(args[0].as_number())};
		return true;
	}


	bool meta_number::operator_atan2(gal_virtual_machine_state& state, magic_value* args)
	{
		if (not state.validate_number(args[1], "x value")) { return false; }

		args[0] = magic_value{std::atan2(args[0].as_number(), args[1].as_number())};
		return true;
	}

	bool meta_number::operator_cbrt([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::cbrt(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_ceil([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::ceil(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_floor([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::floor(args[0].as_number())};
		return true;
	}

	bool meta_number::operator_round([[maybe_unused]] gal_virtual_machine_state& state, magic_value* args)
	{
		args[0] = magic_value{std::round(args[0].as_number())};
		return true;
	}
}// namespace gal
