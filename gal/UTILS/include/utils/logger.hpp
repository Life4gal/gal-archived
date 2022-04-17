#pragma once

#ifndef GAL_UTILS_LOGGER_HPP
#define GAL_UTILS_LOGGER_HPP

#include <utils/macro.hpp>
DISABLE_WARNING_PUSH
DISABLE_WARNING(4189)

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

// default silent logger
#define GAL_UTILS_LOGGER_NO_DEBUG

#ifndef GAL_UTILS_LOGGER_NO_DEBUG
#ifdef _NODEBUG
#define GAL_UTILS_LOGGER_NO_DEBUG
#endif
#endif

#ifndef GAL_UTILS_LOGGER_NO_DEBUG
#include <utils/source_location.hpp>
#endif

namespace gal::utils
{
	namespace logger = spdlog;

	#ifndef GAL_UTILS_LOGGER_NO_DEBUG
	#define GAL_UTILS_DO_IF_DEBUG(...) __VA_ARGS__
	#else
	#define GAL_UTILS_DO_IF_DEBUG(...)
	#endif
}

DISABLE_WARNING_POP

#endif // GAL_UTILS_LOGGER_HPP
