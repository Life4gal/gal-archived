#pragma once

#ifndef GAL_LANG_FOUNDATION_RETURN_WRAPPER_HPP
#define GAL_LANG_FOUNDATION_RETURN_WRAPPER_HPP

#include <functional>
#include <gal/foundation/boxed_value.hpp>
#include <gal/types/number_type.hpp>

namespace gal::lang::foundation
{
	template<typename FunctionSignature, typename Callable>
	class callable_function_proxy;
	template<typename FunctionSignature>
	class assignable_function_proxy;

	namespace return_wrapper_detail
	{
		template<typename Result, bool IsPointer>
		struct reference_return_wrapper
		{
			template<typename T>
			static auto wrapper(T&& result) { return boxed_value{std::cref(result), true}; }
		};

		template<typename Result>
		struct reference_return_wrapper<Result, true>
		{
			template<typename T>
			static boxed_value wrapper(T&& result) { return boxed_value{std::remove_reference_t<decltype(result)>{result}, true}; }
		};

		/**
		 * @brief Used internally for handling a return value from a function_proxy call
		 */
		template<typename Result>
		struct return_wrapper
		{
			template<typename T>
				requires std::is_trivial_v<std::decay_t<T>>
			static boxed_value wrapper(T result) { return boxed_value{std::move(result), true}; }

			template<typename T>
				requires not std::is_trivial_v<std::decay_t<T>>
			static boxed_value wrapper(T&& result) { return boxed_value{std::make_shared<T>(std::forward<T>(result)), true}; }
		};

		template<typename Result>
		struct return_wrapper<const Result>
		{
			static boxed_value wrapper(Result result) { return boxed_value{std::move(result)}; }
		};

		template<typename Result>
		struct return_wrapper<Result&>
		{
			static boxed_value wrapper(Result& result) { return boxed_value{std::ref(result)}; }
		};

		template<typename Result>
		struct return_wrapper<const Result&> : reference_return_wrapper<const Result&, std::is_pointer_v<std::remove_reference_t<const Result&>>> { };

		template<typename Result>
		struct return_wrapper<Result*>
		{
			static boxed_value wrapper(Result* ptr) { return boxed_value{ptr, true}; }
		};

		template<typename Result>
		struct return_wrapper<const Result*>
		{
			static boxed_value wrapper(const Result* ptr) { return boxed_value{ptr, true}; }
		};

		template<typename Result>
		struct return_wrapper<Result*&>
		{
			static boxed_value wrapper(Result* ptr) { return boxed_value{ptr, true}; }
		};

		template<typename Result>
		struct return_wrapper<const Result*&>
		{
			static boxed_value wrapper(const Result* ptr) { return boxed_value{ptr, true}; }
		};

		template<typename Result>
		struct return_wrapper<std::function<Result>> : return_wrapper<const std::function<Result>&> { };

		template<typename Result>
		struct return_wrapper<std::function<Result>&>
		{
			static boxed_value wrapper(std::function<Result>& function) { return boxed_value{std::make_shared<assignable_function_proxy<return_wrapper>>(std::ref(function), std::shared_ptr<std::function<Result>>{})}; }

			static boxed_value wrapper(const std::function<Result>& function) { return boxed_value{std::make_shared<callable_function_proxy<Result, std::function<Result>>>(function)}; }
		};

		template<typename Result>
		struct return_wrapper<const std::function<Result>&>
		{
			static boxed_value wrapper(const std::function<Result>& function) { return boxed_value{std::make_shared<callable_function_proxy<Result, std::function<Result>>>(function)}; }
		};

		template<typename Result>
		struct return_wrapper<std::unique_ptr<Result>> : return_wrapper<std::unique_ptr<Result>&>
		{
			static boxed_value wrapper(std::unique_ptr<Result>&& result) { return boxed_value{std::move(result), true}; }
		};

		template<typename Result>
		struct return_wrapper<std::shared_ptr<Result>> : return_wrapper<std::shared_ptr<Result>&> { };

		template<typename Result>
		struct return_wrapper<std::shared_ptr<Result>&>
		{
			static boxed_value wrapper(const std::shared_ptr<Result>& result) { return boxed_value{result, true}; }
		};

		template<typename Result>
		struct return_wrapper<const std::shared_ptr<Result>&> : return_wrapper<std::shared_ptr<Result>&> { };

		template<typename Result>
		struct return_wrapper<std::shared_ptr<std::function<Result>>> : return_wrapper<const std::shared_ptr<std::function<Result>>> { };

		template<typename Result>
		struct return_wrapper<const std::shared_ptr<std::function<Result>>>
		{
			static boxed_value wrapper(const std::shared_ptr<std::function<Result>>& function) { return boxed_value{std::make_shared<assignable_function_proxy<Result>>(std::ref(*function), function)}; }
		};

		template<typename Result>
		struct return_wrapper<const std::shared_ptr<std::function<Result>>&> : return_wrapper<const std::shared_ptr<std::function<Result>>> { };

		template<>
		struct return_wrapper<boxed_value>
		{
			static boxed_value wrapper(const boxed_value& result) noexcept { return result; }
		};

		template<>
		struct return_wrapper<const boxed_value> : return_wrapper<boxed_value> { };

		template<>
		struct return_wrapper<boxed_value&> : return_wrapper<boxed_value> { };

		template<>
		struct return_wrapper<const boxed_value&> : return_wrapper<boxed_value> { };

		template<>
		struct return_wrapper<types::number_type>
		{
			static boxed_value wrapper(const types::number_type& result) noexcept { return result.value; }
		};

		template<>
		struct return_wrapper<const types::number_type> : return_wrapper<types::number_type> { };

		template<>
		struct return_wrapper<types::number_type&> : return_wrapper<types::number_type> { };

		template<>
		struct return_wrapper<const types::number_type&> : return_wrapper<types::number_type> { };

		template<>
		struct return_wrapper<void>
		{
			static boxed_value wrapper() { return void_var(); }
		};
	}// namespace return_wrapper_detail
}

#endif // GAL_LANG_FOUNDATION_RETURN_WRAPPER_HPP
