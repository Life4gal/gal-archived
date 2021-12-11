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
		}

		values[0] = state.make_object<object_fiber>(state, closure)->operator magic_value();
	}
}// namespace gal
