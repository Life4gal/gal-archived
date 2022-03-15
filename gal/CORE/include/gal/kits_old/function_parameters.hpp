#pragma once

#ifndef GAL_LANG_KITS_FUNCTION_PARAMETERS_HPP
#define GAL_LANG_KITS_FUNCTION_PARAMETERS_HPP

#include <gal/kits/boxed_value.hpp>
#include <utils/initializer_list.hpp>

namespace gal::lang::kits
{
	using function_parameters = utils::initializer_list<boxed_value>;
}

#endif // GAL_LANG_KITS_FUNCTION_PARAMETERS_HPP
