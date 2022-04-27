#pragma once

#ifndef GAL_LANG_GAL_HPP
#define GAL_LANG_GAL_HPP

#include <gal/language/engine.hpp>
#include <gal/language/parser.hpp>
#include <gal/extra/visitor.hpp>
#include <gal/extra/optimizer.hpp>
#include <gal/extra/standard_library.hpp>

namespace gal::lang
{
	class engine : public lang::engine_base
	{
	private:
		extra::default_visitor visitor_;
		extra::default_optimizer optimizer_;

	public:
		explicit engine(preloaded_paths_type preloaded_paths = {})
			: engine_base{
					extra::standard_library::build(),
					std::make_unique<lang::parser>(visitor_, optimizer_),
					std::move(preloaded_paths)} {}
	};
}

#endif // GAL_LANG_GAL_HPP
