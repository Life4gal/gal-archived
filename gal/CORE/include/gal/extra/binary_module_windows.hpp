#pragma once

#ifndef GAL_LANG_EXTRA_BINARY_MODULE_WINDOWS_HPP
#define GAL_LANG_EXTRA_BINARY_MODULE_WINDOWS_HPP

#ifndef GAL_LANG_WINDOWS
#error "This file should not be used before macro GAL_LANG_WINDOWS is defined"
#endif

#define VC_EXTRA_LEAN
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
// ReSharper disable once IdentifierTypo
#define NOMINMAX
#include <Windows.h>

#include "gal/foundation/dispatcher.hpp"

namespace gal::lang::extra
{
	struct binary_module
	{
	private:
		static auto string_transformer(const std::string_view string)
		{
			#if defined(_UNICODE) || defined(UNICODE)
			return std::wstring{
					string.begin(),
					string.end()};
			#else
			return std::string{
					string.begin(),
					string.end()};
			#endif
		}

		static std::string get_error_message(const DWORD error)
		{
			#if defined(_UNICODE) || defined(UNICODE)
			std::wstring ret = L"Unknown Error";
			#else
			std::string ret = "Unknown Error";
			#endif

			LPSTR msg = nullptr;
			if (FormatMessage(
					    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					    nullptr,
					    error,
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					    reinterpret_cast<LPSTR>(&msg),
					    0,
					    nullptr
					    ) != 0 && msg)
			{
				ret = msg;
				LocalFree(msg);
			}

			return std::string{ret.begin(), ret.end()};
		}

	public:
		struct dynamic_load_module
		{
		private:
			struct module_deleter
			{
				auto operator()(const HMODULE m) const { return FreeLibrary(m); }
			};

		public:
			std::unique_ptr<std::remove_cvref_t<decltype(*std::declval<HMODULE>())>, module_deleter> handle;

			explicit dynamic_load_module(const std::string_view filename)
				: handle{LoadLibrary(string_transformer(filename).c_str())} { if (not handle) { throw exception::load_module_error{get_error_message(GetLastError())}; } }
		};

		template<typename T>
			requires std::is_pointer_v<T>
		struct dynamic_load_symbol
		{
			T symbol;

			explicit dynamic_load_symbol(const dynamic_load_module& m, const std::string_view s)
				: symbol{reinterpret_cast<T>(GetProcAddress(m.handle.get(), s.data()))} { if (not symbol) { throw exception::load_module_error{get_error_message(GetLastError())}; } }
		};

		inline static std::string module_load_function_prefix = "create_module_";

		dynamic_load_module dlm;
		dynamic_load_symbol<foundation::engine_module_maker> function;
		foundation::engine_module_type module_ptr;

		binary_module(const std::string_view module_name, const std::string_view filename)
			: dlm{filename},
			  function{dlm, std::string{module_load_function_prefix}.append(module_name)},
			  module_ptr{function.symbol()} { }
	};
}

#endif // GAL_LANG_EXTRA_BINARY_MODULE_WINDOWS_HPP
