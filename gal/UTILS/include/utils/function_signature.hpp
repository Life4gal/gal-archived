#pragma once

#ifndef GAL_UTILS_FUNCTION_SIGNATURE_HPP
#define GAL_UTILS_FUNCTION_SIGNATURE_HPP

#include<type_traits>

namespace gal::utils
{
	namespace detail
	{
		template<typename... Params>
		struct function_param_type { };
	}

	template<
		typename ReturnType,
		typename ParamType,
		bool IsNoexcept = false,
		bool IsMember = false,
		bool IsMemberObject = false,
		bool IsObject = false
	>
	struct function_signature
	{
		using return_type = ReturnType;
		using param_type = ParamType;
		constexpr static bool is_noexcept = IsNoexcept;
		constexpr static bool is_member_object = IsMemberObject;
		constexpr static bool is_object = IsObject;

		constexpr function_signature() noexcept = default;

		template<typename T>
		constexpr explicit function_signature(T&&) noexcept { }// NOLINT(bugprone-forwarding-reference-overload)

		// template<typename T>
		// constexpr explicit operator T() noexcept = delete;
	};

	// normal function
	template<typename ReturnType, typename...ParamTypes>
	function_signature(ReturnType (*)(ParamTypes ...)) -> function_signature<ReturnType, detail::function_param_type<ParamTypes...>>;
	template<typename ReturnType, typename...ParamTypes>
	function_signature(ReturnType (*)(ParamTypes ...) noexcept) -> function_signature<ReturnType, detail::function_param_type<ParamTypes...>, true>;

	// object's member data
	template<typename ReturnType, typename ObjectType>
	function_signature(ReturnType ObjectType::*) -> function_signature<ReturnType, detail::function_param_type<ObjectType&>, true, true, true>;

	// ***************************************************************************************
	// object's member function with cv-qualification
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...)) -> function_signature<ReturnType, detail::function_param_type<ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) noexcept) -> function_signature<ReturnType, detail::function_param_type<ObjectType&, ParamTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const noexcept) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&, ParamTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile noexcept) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&, ParamTypes...>, true, true>;
	// const & volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile noexcept) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&, ParamTypes...>, true, true>;

	// ***************************************************************************************
	// object's member function with cv-qualification && lvalue only(&)
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) &) -> function_signature<ReturnType, detail::function_param_type<ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) & noexcept) -> function_signature<ReturnType, detail::function_param_type<ObjectType&, ParamTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const &) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const & noexcept) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&, ParamTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile &) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile & noexcept) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&, ParamTypes...>, true, true>;
	// const && volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile &) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile & noexcept) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&, ParamTypes...>, true, true>;

	// ***************************************************************************************
	// object's member function with cv-qualification && rvalue only(&&)
	// ***************************************************************************************
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) &&) -> function_signature<ReturnType, detail::function_param_type<ObjectType&&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) && noexcept) -> function_signature<ReturnType, detail::function_param_type<ObjectType&&, ParamTypes...>, true, true>;
	// const
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const &&) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const && noexcept) -> function_signature<ReturnType, detail::function_param_type<const ObjectType&&, ParamTypes...>, true, true>;
	// volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile &&) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) volatile && noexcept) -> function_signature<ReturnType, detail::function_param_type<volatile ObjectType&&, ParamTypes...>, true, true>;
	// const && volatile
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile &&) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&&, ParamTypes...>, false, true>;
	template<typename ReturnType, typename ObjectType, typename... ParamTypes>
	function_signature(ReturnType (ObjectType::*)(ParamTypes ...) const volatile && noexcept) -> function_signature<ReturnType, detail::function_param_type<const volatile ObjectType&&, ParamTypes...>, true, true>;

	template<typename Function>
	constexpr auto make_function_signature(const Function& function) noexcept
	{
		if constexpr (requires { &Function::operator(); })
		{
			return function_signature<
				typename decltype(function_signature{&std::decay_t<Function>::operator()})::return_type,
				typename decltype(function_signature{&std::decay_t<Function>::operator()})::param_type,
				decltype(function_signature{&std::decay_t<Function>::operator()})::is_noexcept,
				false,
				false,
				true>{};
		}
		else { return function_signature{function}; }
	}

	template<typename ReturnType, bool IsNoexcept, bool IsMember, bool IsMemberObject, bool IsObject, typename... Params>
	using function_signature_t = function_signature<ReturnType, detail::function_param_type<Params...>, IsNoexcept, IsMember, IsMemberObject, IsObject>;

	/**
	 * @brief For call an object's member function.
	 */
	template<typename Object, typename ReturnType, typename ObjectType, typename... Params, bool IsNoexcept, bool IsMember, bool IsMemberObject, bool IsObject>
	constexpr ObjectType get_object_instance(function_signature<ReturnType, detail::function_param_type<ObjectType, Params...>, IsNoexcept, IsMember, IsMemberObject, IsObject>, Object&& object) noexcept { return static_cast<ObjectType>(std::forward<Object>(object)); }
}

#endif // GAL_UTILS_FUNCTION_SIGNATURE_HPP
