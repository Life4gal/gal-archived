#pragma once

#ifndef GAL_LANG_FOUNDATION_PARAMETERS_HPP
#define GAL_LANG_FOUNDATION_PARAMETERS_HPP

#include <gal/foundation/boxed_value.hpp>
#include <utils/initializer_list.hpp>
#include <vector>

namespace gal::lang::foundation
{
	using parameters_view = utils::initializer_list<boxed_value>;
	using parameters = std::vector<boxed_value>;
}

#endif // GAL_LANG_FOUNDATION_PARAMETERS_HPP
