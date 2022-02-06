#include <object_method.hpp>

namespace gal::lang
{
	gal_type_object_internal_method::gal_type_object_internal_method()
		: gal_type_object{
				&gal_type_object_type::type(),
				"internal_method",
				flag_default | flag_have_gc | flag_have_vectorcall,
				"internal method",
				nullptr,
				// todo: methods
				nullptr,
				// todo: members
				nullptr,
				// todo: rw_interfaces
				nullptr
		} { }

	gal_type_object_internal_method& gal_type_object_internal_method::type()
	{
		static gal_type_object_internal_method instance{};
		return instance;
	}
}
