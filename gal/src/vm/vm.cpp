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
				std_format::format_to(error.get_appender(), "Could not find outer method '{}' for class '{}' in module '{}'.", name, real_obj->get_class_name().str(), module.get_name().str());
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
			return;
		}

		// If we got here, nothing caught the error, so show the stack trace.
		debugger::print_stack_trace(*this);
		fiber_	   = nullptr;
		api_stack_ = nullptr;
	}

	void gal_virtual_machine_state::method_not_found(object_class &obj_class, gal_index_type symbol)
	{
		object_string error{*this};
		std_format::format_to(error.get_appender(), "{} does not implement '{}'.", obj_class.get_class_name().str(), method_names_[symbol].str());
		fiber_->set_error(std::move(error));
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
				superclass == class_class_.get() ||
				superclass == fiber_class_.get() ||
				superclass == function_class_.get() ||
				superclass == list_class_.get() ||
				superclass == map_class_.get() ||
				superclass == range_class_.get() ||
				superclass == string_class_.get() ||
				superclass == boolean_class_.get() ||
				superclass == null_class_.get() ||
				superclass == number_class_.get())
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
		if (configuration.bind_outer_class_function != nullptr)
		{
			methods = configuration.bind_outer_class_function(*this, module.get_name().data(), obj_class.get_class_name().data());
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
}// namespace gal
