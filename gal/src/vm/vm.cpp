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
	gal_outer_method_function_type gal_virtual_machine_state::find_outer_method(const char *module_name, const char *class_name, bool is_static, const char *signature)
	{
		gal_outer_method_function_type method = nullptr;

		if (configuration.bind_outer_method_function != nullptr)
		{
			method = configuration.bind_outer_method_function(*this, module_name, class_name, is_static, signature);
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
				object_string error{*this};
				std_format::format_to(error.get_appender(), "Could not find outer method '{}' for class '{}' in module '{}'.", name, real_obj->get_class_name().data(), module.get_name().data());
				fiber_->set_error(std::move(error));
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
		}

		// If we got here, nothing caught the error, so show the stack trace.
		debugger::print_stack_trace(*this);
		fiber_	   = nullptr;
		api_stack_ = nullptr;
	}
}// namespace gal
