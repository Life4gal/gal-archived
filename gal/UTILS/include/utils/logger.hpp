#pragma once

#ifndef GAL_UTILS_LOGGER_HPP
#define GAL_UTILS_LOGGER_HPP

#include <utils/macro.hpp>
DISABLE_WARNING_PUSH
DISABLE_WARNING(4189)

#include <spdlog/spdlog.h>

#ifndef GAL_UTILS_LOGGER_DEBUG
#ifndef _NODEBUG
#define GAL_UTILS_LOGGER_DEBUG
#include <utils/source_location.hpp>
#endif
#endif

namespace gal::utils
{
	namespace logger = spdlog;

	#ifdef GAL_UTILS_LOGGER_DEBUG
	#define GAL_UTILS_DO_IF_DEBUG(...) __VA_ARGS__
	#else
		#define GAL_UTILS_DO_IF_DEBUG(...)
	#endif

	#define GAL_UTILS_DO_IF_LOG_INFO(...) __VA_ARGS__
}

DISABLE_WARNING_POP

#endif // GAL_UTILS_LOGGER_HPP
