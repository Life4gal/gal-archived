#pragma once

#ifndef GAL_LANG_GAL_HPP
#define GAL_LANG_GAL_HPP

#include <gal/foundation/engine.hpp>
#include <gal/addons/ast_parser.hpp>
#include <gal/plugins/standard_library.hpp>

namespace gal::lang
{
	class engine : public foundation::engine_base
	{
	public:
		explicit engine(const std::size_t max_parse_depth = 512, preloaded_paths_type preloaded_paths = {})
			: engine_base{
					plugin::standard_library::build(),
					std::make_unique<addon::ast_parser>(max_parse_depth),
					std::move(preloaded_paths)} {}
	};
}

#endif // GAL_LANG_GAL_HPP
