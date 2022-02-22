#pragma once

#ifndef GAL_UTILS_DEFAULT_HASHER_HPP
	#define GAL_UTILS_DEFAULT_HASHER_HPP

#include <utility>

namespace gal::utils
{
	template<typename T>
	struct default_hasher : std::hash<T>
	{
	};

	/**
	 * @brief Unfortunately, containers that rely on hash seem to treat pointers with different decorations as different types,
	 * which means that we cannot use constant pointers as keys to find containers that store non-constant pointers.
	 *
	 * Generally, the hash value of the pointer is the address of the pointer, so whether the pointer is modified by const should have no effect on the result.
	 */
	template<typename T>
	requires std::is_pointer_v<T>
	struct default_hasher<T>
	{
		using is_transparent = int;

		using value_type	 = typename std::pointer_traits<T>::element_type;

		[[nodiscard]] constexpr std::size_t operator()(value_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<value_type*>, value_type*>) { return std::hash<value_type*>{}(ptr); }

		[[nodiscard]] constexpr std::size_t operator()(const value_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<const value_type*>, const value_type*>) { return std::hash<const value_type*>{}(ptr); }

		template<typename U>
		requires std::is_base_of_v<value_type, typename std::pointer_traits<U>::element_type>
		[[nodiscard]] constexpr std::size_t operator()(typename std::pointer_traits<U>::element_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<typename std::pointer_traits<U>::element_type*>, typename std::pointer_traits<U>::element_type*>) { return std::hash<typename std::pointer_traits<U>::element_type*>{}(ptr); }

		template<typename U>
		requires std::is_base_of_v<value_type, typename std::pointer_traits<U>::element_type>
		[[nodiscard]] constexpr std::size_t operator()(const typename std::pointer_traits<U>::element_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<const typename std::pointer_traits<U>::element_type*>, const typename std::pointer_traits<U>::element_type*>) { return std::hash<const typename std::pointer_traits<U>::element_type*>{}(ptr); }
	};
}

#endif // GAL_UTILS_DEFAULT_HASHER_HPP
