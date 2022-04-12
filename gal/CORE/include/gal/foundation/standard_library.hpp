#pragma once

#ifndef GAL_LANG_FOUNDATION_STANDARD_LIBRARY_HPP
#define GAL_LANG_FOUNDATION_STANDARD_LIBRARY_HPP

#include <gal/foundation/bootstrap.hpp>
#include <gal/foundation/bootstrap_stl.hpp>
#include <gal/foundation/dispatcher.hpp>

namespace gal::lang::foundation
{
	class standard_library
	{
	public:
		[[nodiscard]] static shared_engine_core build()
		{
			auto library = std::make_shared<engine_core>();

			bootstrap::do_bootstrap(*library);

			register_vector_type<std::vector<boxed_value>>(lang::vector_type_name::value, *library);
			// register_list_type<std::list<boxed_value>>(lang::list_type_name::value, *library);
			// todo: test string_view_type is safe enough
			register_map_type<std::map<string_view_type, boxed_value>>(lang::map_type_name::value, *library);
			register_pair_type<std::pair<boxed_number, boxed_value>>(lang::pair_type_name::value, *library);

			// todo: more internal foundation

			return library;
		}
	};
}

#endif // GAL_LANG_FOUNDATION_STANDARD_LIBRARY_HPP
