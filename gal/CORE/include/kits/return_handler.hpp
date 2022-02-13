#pragma once

#ifndef GAL_LANG_KITS_RETURN_HANDLER_HPP
#define GAL_LANG_KITS_RETURN_HANDLER_HPP

#include <functional>
#include<kits/boxed_number.hpp>
#include<kits/boxed_value.hpp>

namespace gal::lang::kits
{
	class proxy_function_base;
	template<typename Function, typename Callable>
	class proxy_function_callable_impl;
	template<typename Function>
	class assignable_proxy_function_impl;

	namespace detail
	{
		template<typename Result, bool IsPointer>
		struct return_handler_reference
		{
			template<typename T>
			static boxed_value handle(T&& result) { return {std::cref(result), true}; }
		};

		template<typename Result>
		struct return_handler_reference<Result, true>
		{
			template<typename T>
			static boxed_value handle(T&& result) { return {std::remove_reference_t<decltype(result)>{result}, true}; }
		};

		/**
		 * @brief Used internally for handling a return value from a proxy_function call
		 */
		template<typename Result>
		struct return_handler
		{
			template<typename T>
				requires std::is_trivial_v<std::decay_t<T>>
			static boxed_value handle(T result) { return {std::move(result), true}; }

			template<typename T>
				requires not std::is_trivial_v<std::decay_t<T>>
			static boxed_value handle(T&& result) { return {std::make_shared<T>(std::forward<T>(result)), true}; }
		};

		template<typename Result>
		struct return_handler<const Result>
		{
			static boxed_value handle(Result result) { return {std::move(result)}; }
		};

		template<typename Result>
		struct return_handler<Result&>
		{
			static boxed_value handle(Result& result) { return {std::ref(result)}; }
		};

		template<typename Result>
		struct return_handler<const Result&> : return_handler_reference<const Result&, std::is_pointer_v<std::remove_reference_t<const Result&>>> {};

		template<typename Result>
		struct return_handler<Result*>
		{
			static boxed_value handle(Result* ptr) { return {ptr, true}; }
		};

		template<typename Result>
		struct return_handler<const Result*>
		{
			static boxed_value handle(const Result* ptr) { return {ptr, true}; }
		};

		template<typename Result>
		struct return_handler<Result*&>
		{
			static boxed_value handle(Result* ptr) { return {ptr, true}; }
		};

		template<typename Result>
		struct return_handler<const Result*&>
		{
			static boxed_value handle(const Result* ptr) { return {ptr, true}; }
		};

		template<typename Result>
		struct return_handler<std::function<Result>> : return_handler<const std::function<Result>&> { };

		template<typename Result>
		struct return_handler<std::function<Result>&>
		{
			static boxed_value handle(std::function<Result>& function) { return {std::make_shared<proxy_function_base, assignable_proxy_function_impl<return_handler>>(std::ref(function), std::shared_ptr<std::function<Result>>{})}; }

			static boxed_value handle(const std::function<Result>& function) { return {std::make_shared<proxy_function_base, proxy_function_callable_impl<Result, std::function<Result>>>(function)}; }
		};

		template<typename Result>
		struct return_handler<const std::function<Result>&>
		{
			static boxed_value handle(const std::function<Result>& function) { return {std::make_shared<proxy_function_base, proxy_function_callable_impl<Result, std::function<Result>>>(function)}; }
		};

		template<typename Result>
		struct return_handler<std::unique_ptr<Result>> : return_handler<std::unique_ptr<Result>&>
		{
			static boxed_value handle(std::unique_ptr<Result>&& result) { return {std::move(result), true}; }
		};

		template<typename Result>
		struct return_handler<std::shared_ptr<Result>> : return_handler<std::shared_ptr<Result>&> {};

		template<typename Result>
		struct return_handler<std::shared_ptr<Result>&>
		{
			static boxed_value handle(const std::shared_ptr<Result>& result) { return {result, true}; }
		};

		template<typename Result>
		struct return_handler<const std::shared_ptr<Result>&> : return_handler<std::shared_ptr<Result>&> {};

		template<typename Result>
		struct return_handler<std::shared_ptr<std::function<Result>>> : return_handler<const std::shared_ptr<std::function<Result>>> { };

		template<typename Result>
		struct return_handler<const std::shared_ptr<std::function<Result>>>
		{
			static boxed_value handle(const std::shared_ptr<std::function<Result>>& function) { return {std::make_shared<proxy_function_base, assignable_proxy_function_impl<Result>>(std::ref(*function), function)}; }
		};

		template<typename Result>
		struct return_handler<const std::shared_ptr<std::function<Result>>&> : return_handler<const std::shared_ptr<std::function<Result>>> { };

		template<>
		struct return_handler<boxed_value>
		{
			static boxed_value handle(const boxed_value& result) noexcept { return result; }
		};

		template<>
		struct return_handler<const boxed_value> : return_handler<boxed_value> {};

		template<>
		struct return_handler<boxed_value&> : return_handler<boxed_value> {};

		template<>
		struct return_handler<const boxed_value&> : return_handler<boxed_value> {};

		template<>
		struct return_handler<boxed_number>
		{
			static boxed_value handle(const boxed_number& result) noexcept { return result.value; }
		};

		template<>
		struct return_handler<const boxed_number> : return_handler<boxed_number> {};

		template<>
		struct return_handler<boxed_number&> : return_handler<boxed_number> { };

		template<>
		struct return_handler<const boxed_number&> : return_handler<boxed_number> { };

		template<>
		struct return_handler<void>
		{
			static boxed_value handle() { return void_var(); }
		};
	}
}

#endif // GAL_LANG_KITS_RETURN_HANDLER_HPP
