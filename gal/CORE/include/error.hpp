#pragma once

#ifndef GAL_LANG_ERROR_HPP
#define GAL_LANG_ERROR_HPP

#include <def.hpp>

#if defined(GAL_LANG_DEBUG)
#include <string_view>
#include <source_location>
#endif

namespace gal::lang
{
	class gal_object;

	struct gal_error_handler
	{
		static void error_set_null(gal_object& exception GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));
		static void error_set_object(gal_object& exception, gal_object& value GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));
		static void error_set_string(gal_object& exception, const char* string GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));

		static gal_object* error_occurred(GAL_LANG_DO_IF_DEBUG(std::string_view reason, const std_source_location& location = std_source_location::current()));
		static void error_clear(GAL_LANG_DO_IF_DEBUG(std::string_view reason, const std_source_location& location = std_source_location::current()));
		static void error_fetch(gal_object** type, gal_object** value, gal_object** trace_back GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));
		static void error_restore(gal_object* type, gal_object* value, gal_object* trace_back GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));

		[[noreturn]] static void error_fatal(const char* message GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()));

		// Convenience functions
		static bool error_bad_argument();
		static gal_object* error_not_enough_memory();

		// Format should be utils::fixed_string
		template<typename Format, typename... Args>
		static gal_object* error_format(gal_object& exception, Args&&... args);

		// Error testing and normalization
		static bool exception_matches(gal_object& error, gal_object& exception);
		static bool exception_matches(gal_object& exception);

		// Trace back manipulation
		static void exception_set_trace_back(gal_object& self, gal_object& trace_back);
		static gal_object* exception_get_trace_back(gal_object& self);

		// Reason manipulation
		static void exception_set_reason(gal_object& self, gal_object& reason);
		static gal_object* exception_get_reason(gal_object& self);
	};
}

#endif // GAL_LANG_ERROR_HPP
