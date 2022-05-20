#pragma once

#ifndef GAL_LANG_DEFINES_HPP
#define GAL_LANG_DEFINES_HPP

#include <utils/macro.hpp>

#ifdef _MSC_VER
#define GAL_LANG_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
	#define GAL_LANG_MODULE_EXPORT extern "C"
#endif


#ifdef _DEBUG
#define GAL_LANG_DEBUG true
#define GAL_LANG_BUILD_VERSION GAL_LANG_COMPILER_VERSION "-Debug"
#else
		#define GAL_LANG_BUILD_VERSION GAL_LANG_COMPILER_VERSION "-Release"
#endif

#ifndef GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT
#ifdef GAL_LANG_DEBUG
#define GAL_LANG_ARITHMETIC_DIVIDE_ZERO_PROTECT
#endif
#endif

#ifndef GAL_LANG_TYPE_INFO_UNKNOWN_NAME
#define GAL_LANG_TYPE_INFO_UNKNOWN_NAME "unknown-type"
#endif

#ifndef GAL_LANG_FUNCTION_METHOD_MISSING_NAME
#define GAL_LANG_FUNCTION_METHOD_MISSING_NAME "missing_method"
#endif

#ifndef GAL_LANG_NO_TYPE_INFO_DEBUG
#ifdef GAL_LANG_DEBUG
#define GAL_LANG_TYPE_INFO_DEBUG
#endif
#endif

#ifndef GAL_LANG_NO_RECODE_CALL_LOCATION_DEBUG
#ifdef GAL_LANG_DEBUG
#define GAL_LANG_RECODE_CALL_LOCATION_DEBUG
#endif
#endif

#ifndef GAL_LANG_NO_AST_VISIT_PRINT
#ifdef GAL_LANG_DEBUG
			#define GAL_LANG_AST_VISIT_PRINT
#endif
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
#include <gal/tools/logger.hpp>
#include <utils/source_location.hpp>
#else
#define GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(...)
#endif

#ifdef GAL_LANG_AST_VISIT_PRINT
	#define GAL_LANG_AST_VISIT_PRINT_DO(...) __VA_ARGS__
#else
#define GAL_LANG_AST_VISIT_PRINT_DO(...)
#endif

namespace gal::lang
{
	constexpr static int gal_lang_version_major = GAL_LANG_MAJOR_VERSION;
	constexpr static int gal_lang_version_minor = GAL_LANG_MINOR_VERSION;
	constexpr static int gal_lang_version_patch = GAL_LANG_PATCH_VERSION;

	constexpr static std::string_view gal_lang_version{GAL_LANG_VERSION};
	constexpr static std::string_view gal_lang_compiler_name{GAL_LANG_COMPILER_NAME};
	constexpr static std::string_view gal_lang_compiler_version{GAL_LANG_COMPILER_VERSION};
	constexpr static bool gal_lang_debug_build = GAL_LANG_DEBUG;
	constexpr static std::string_view gal_lang_build_version{GAL_LANG_BUILD_VERSION};

	struct build_info
	{
		[[nodiscard]] constexpr static int version_major() noexcept { return gal_lang_version_major; }

		[[nodiscard]] constexpr static int version_minor() noexcept { return gal_lang_version_minor; }

		[[nodiscard]] constexpr static int version_patch() noexcept { return gal_lang_version_patch; }

		[[nodiscard]] constexpr static std::string_view version() noexcept { return gal_lang_version; }

		[[nodiscard]] constexpr static std::string_view compiler_name() noexcept { return gal_lang_compiler_name; }

		[[nodiscard]] constexpr static std::string_view compiler_version() noexcept { return gal_lang_compiler_version; }

		[[nodiscard]] constexpr static bool is_debug_build() noexcept { return gal_lang_debug_build; }

		[[nodiscard]] constexpr static std::string_view build_version() { return gal_lang_build_version; }
	};
}

#endif // GAL_LANG_DEFINES_HPP
