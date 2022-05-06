#pragma once

#ifndef GAL_LANG_PLUGIN_STANDARD_LIBRARY_HPP
	#define GAL_LANG_PLUGIN_STANDARD_LIBRARY_HPP

#include <gal/foundation/dispatcher.hpp>
#include <gal/plugins/bootstrap.hpp>

namespace gal::lang::plugin
{
	struct standard_library
	{
	public:
		[[nodiscard]] static foundation::engine_module_type build()
		{
			auto library = foundation::make_engine_module();

			bootstrap::do_bootstrap(*library);

			return library;
		}
	};
}

#endif // GAL_LANG_PLUGIN_STANDARD_LIBRARY_HPP
