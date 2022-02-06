#include <object.hpp>

namespace gal::lang
{
	void safe_clear_object(
			gal_object*& object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)
			)
	{
		if (auto* temp = object; temp)
		{
			object = nullptr;
			safe_clear_object(*temp GAL_LANG_DO_IF_DEBUG(, reason, location));
		}
	}

	void safe_clear_object(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)
			) { object.decrease_count(GAL_LANG_DO_IF_DEBUG(reason, location)); }

	void safe_increase_object_ref_count(
			gal_object* object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)) { if (object) { safe_increase_object_ref_count(*object GAL_LANG_DO_IF_DEBUG(, reason, location)); } }

	void safe_increase_object_ref_count(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)) { object.increase_count(GAL_LANG_DO_IF_DEBUG(reason, location)); }

	void safe_decrease_object_ref_count(
			gal_object* object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)) { if (object) { safe_decrease_object_ref_count(*object GAL_LANG_DO_IF_DEBUG(, reason, location)); } }

	void safe_decrease_object_ref_count(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)) { object.decrease_count(GAL_LANG_DO_IF_DEBUG(reason, location)); }

	void safe_assign_object(
			gal_object*& lhs,
			gal_object* rhs
			GAL_LANG_DO_IF_DEBUG(,
					const std::string_view reason,
					const std_source_location& location)
			)
	{
		auto* tmp = lhs;
		lhs = rhs;
		safe_decrease_object_ref_count(*tmp GAL_LANG_DO_IF_DEBUG(, reason, location));
	}

	void safe_assign_object_lhs_maybe_null(gal_object*& lhs, gal_object* rhs, std::string_view reason, const std_source_location& location)
	{
		auto* tmp = lhs;
		lhs = rhs;
		safe_decrease_object_ref_count(tmp GAL_LANG_DO_IF_DEBUG(, reason, location));
	}

	gal_type_object_type::gal_type_object_type(gal_type_object* self)
		: gal_type_object{
				self,
				"type",
				flag_default |
				flag_have_gc |
				flag_base_type |
				flag_type_subclass |
				flag_have_vectorcall,
				"use type(object) to get an object's type.\n"
				"use type(name, bases, metadata, **pair_args) to get a new type.\n",
				// todo: methods
				nullptr,
				// todo: members
				nullptr,
				// todo: rw_interfaces
				nullptr
		} { }

	gal_type_object_type& gal_type_object_type::type()
	{
		static gal_type_object_type instance{&instance};

		return instance;
	}

	gal_type_object_object::gal_type_object_object()
		: gal_type_object{
				&gal_type_object_type::type(),
				"object",
				flag_default | flag_base_type,
				"object() -- The base class of the class hierarchy.\n\n"
				"When called, it accepts no arguments and returns a new featureless\n"
				"instance that has no instance attributes and cannot be given any.\n",
				// todo: methods
				nullptr,
				nullptr,
				// todo: rw_interfaces
				nullptr
		} { }

	gal_type_object_object& gal_type_object_object::type()
	{
		static gal_type_object_object instance{};
		return instance;
	}

	gal_type_object_super::gal_type_object_super()
		: gal_type_object{
				&gal_type_object_type::type(),
				"super",
				flag_default | flag_have_gc | flag_base_type,
				"super() -> same as super(__class__, <first argument>)\n"
				"super(type) -> unbound super object\n"
				"super(type, object) -> bound super object; requires instance_of(object, type)\n"
				"super(type, type2) -> bound super object; requires subclass_of(type2, type)\n"
				"Typical use to call a cooperative superclass method.\n",
				nullptr,
				// todo: member
				nullptr
		} { }

	gal_type_object_super& gal_type_object_super::type()
	{
		static gal_type_object_super instance{};
		return instance;
	}

	gal_object* gal_type_object_null::object_life_manager::construct_type::call(host_class_type& self, gal_object* args, gal_object* pair_args)
	{
		// todo
		(void)self, (void)args, (void)pair_args;
		return nullptr;
	}

	gal_object* gal_type_object_null::object_represent_manager::represent_type::call(host_class_type& self)
	{
		// todo: return a unicode string of `null`
		(void)self;
		return nullptr;
	}

	bool gal_type_object_null::object_mathematical_manager::to_boolean::call(const gal_object& self)
	{
		(void)self;
		return false;
	}

	gal_type_object_null::gal_type_object_null()
		: gal_type_object
		{
				&gal_type_object_type::type(),
				"null",
				flag_default,
				"undefined type which can be used in contexts"
				"where nullptr is not suitable (since nullptr often means 'error')"
		} {}

	gal_type_object_null& gal_type_object_null::type()
	{
		static gal_type_object_null instance{};
		return instance;
	}

	gal_object& gal_type_object_null::instance(GAL_LANG_DO_IF_DEBUG(const std_source_location& location))
	{
		static gal_object instance{&type()};
		safe_increase_object_ref_count(instance GAL_LANG_DO_IF_DEBUG(, "get an instance of object null", location));
		return instance;
	}

	gal_object* gal_type_object_not_implemented::object_life_manager::construct_type::call(host_class_type& self, gal_object* args, gal_object* pair_args)
	{
		// todo
		(void)self, (void)args, (void)pair_args;

		return &instance();
	}

	gal_object* gal_type_object_not_implemented::object_represent_manager::represent_type::call(host_class_type& self)
	{
		// todo: return a unicode string of `not_implemented`
		(void)self;
		return nullptr;
	}

	bool gal_type_object_not_implemented::object_mathematical_manager::to_boolean::call(const gal_object& self)
	{
		// todo
		(void)self;
		return true;
	}

	gal_type_object_not_implemented::gal_type_object_not_implemented()
		: gal_type_object{
				&gal_type_object_type::type(),
				"not_implemented",
				flag_default,
				"current content is not implemented yet.",
				// todo: methods
				nullptr
		} { }

	gal_type_object_not_implemented& gal_type_object_not_implemented::type()
	{
		static gal_type_object_not_implemented instance{};
		return instance;
	}

	gal_object& gal_type_object_not_implemented::instance(GAL_LANG_DO_IF_DEBUG(const std_source_location& location))
	{
		static gal_object instance{&type()};
		safe_increase_object_ref_count(instance GAL_LANG_DO_IF_DEBUG(, "get an instance of object not_implement", location));
		return instance;
	}
}
