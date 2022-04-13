#pragma once

#ifndef GAL_LANG_BASIC_HPP
#define GAL_LANG_BASIC_HPP

#include <gal/function_register.hpp>
#include <gal/language/parser.hpp>
#include <gal/language/engine.hpp>
#include <gal/language/visitor.hpp>
#include <gal/language/optimizer.hpp>
#include <gal/foundation/standard_library.hpp>

namespace gal::lang
{
	class engine : public lang::engine_base
	{
	private:
		lang::default_visitor visitor_;
		lang::default_optimizer optimizer_;

	public:
		explicit engine(
				std::vector<std::string> preload_paths = {},
				const engine_option option = engine_option::default_option
				)
			: engine_base{
					foundation::standard_library::build(),
					std::make_unique<lang::parser>(visitor_, optimizer_),
					std::move(preload_paths),
					option
			} {}
	};
}

#endif // GAL_LANG_BASIC_HPP
