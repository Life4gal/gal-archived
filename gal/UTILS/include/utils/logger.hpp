#pragma once

#ifndef GAL_UTILS_LOGGER_HPP
#define GAL_UTILS_LOGGER_HPP

#include <utils/macro.hpp>
DISABLE_WARNING_PUSH
DISABLE_WARNING(4189)

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

namespace gal::utils
{
	namespace logger = spdlog;
}

DISABLE_WARNING_POP

#endif // GAL_UTILS_LOGGER_HPP
