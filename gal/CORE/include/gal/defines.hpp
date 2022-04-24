#pragma once

#ifndef GAL_LANG_DEFINES_HPP
#define GAL_LANG_DEFINES_HPP

#define GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT

#ifndef NDEBUG
#define GAL_LANG_TYPE_INFO_DEBUG
#define GAL_LANG_RECODE_CALL_LOCATION_DEBUG
#endif

#ifdef GAL_LANG_TYPE_INFO_DEBUG
#define GAL_LANG_TYPE_INFO_DEBUG_DO(...) __VA_ARGS__
#define GAL_LANG_TYPE_INFO_DEBUG_DO_OR(otherwise, ...) __VA_ARGS__
#include <utils/format.hpp>
#else
#define GAL_LANG_TYPE_INFO_DEBUG_DO(...)
#define GAL_LANG_TYPE_INFO_DEBUG_DO_OR(otherwise, ...) otherwise
#endif

#ifdef GAL_LANG_RECODE_CALL_LOCATION_DEBUG
#define GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(...) __VA_ARGS__
#include <utils/source_location.hpp>
#include <gal/tools/logger.hpp>
#else
#define GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(...)
#endif

#include <utils/macro.hpp>

namespace gal::lang { }

#endif // GAL_LANG_DEFINES_HPP
