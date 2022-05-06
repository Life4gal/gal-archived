#pragma once

#ifndef GAL_LANG_GAL_NO_ADDON_HPP
#define GAL_LANG_GAL_NO_ADDON_HPP

#include <gal/foundation/engine.hpp>
#include <gal/plugins/standard_library.hpp>

namespace gal::lang
{
	template<typename ParserType>
		requires std::is_base_of_v<ast::ast_parser_base, ParserType>
	class engine_no_addon : public foundation::engine_base
	{
	public:
		template<typename... Args>
		explicit engine_no_addon(preloaded_paths_type preloaded_paths = {}, Args&&... args)
			: engine_base{
					plugin::standard_library::build(),
					std::make_unique<ParserType>(std::forward<Args>(args)...),
					std::move(preloaded_paths)} {}
	};
}// namespace gal::lang

#endif// GAL_LANG_GAL_NO_ADDON_HPP
