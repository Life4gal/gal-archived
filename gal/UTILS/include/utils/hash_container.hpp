#pragma once

#ifndef GAL_LANG_UTILS_HASH_CONTAINER_HPP
#define GAL_LANG_UTILS_HASH_CONTAINER_HPP

// todo: implement it

#include <unordered_map>
#include <unordered_set>

namespace gal::utils
{
	template<typename T>
	struct default_hasher : std::hash<T> {};

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

		using value_type = typename std::pointer_traits<T>::element_type;

		[[nodiscard]] constexpr std::size_t operator()(value_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<value_type*>, value_type*>) { return std::hash<value_type*>{}(ptr); }

		[[nodiscard]] constexpr std::size_t operator()(const value_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<const value_type*>, const value_type*>) { return std::hash<const value_type*>{}(ptr); }

		template<typename U>
			requires std::is_base_of_v<value_type, typename std::pointer_traits<U>::element_type>
		[[nodiscard]] constexpr std::size_t operator()(typename std::pointer_traits<U>::element_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<typename std::pointer_traits<U>::element_type*>, typename std::pointer_traits<U>::element_type*>) { return std::hash<typename std::pointer_traits<U>::element_type*>{}(ptr); }

		template<typename U>
			requires std::is_base_of_v<value_type, typename std::pointer_traits<U>::element_type>
		[[nodiscard]] constexpr std::size_t operator()(const typename std::pointer_traits<U>::element_type* ptr) const noexcept(std::is_nothrow_invocable_v<std::hash<const typename std::pointer_traits<U>::element_type*>, const typename std::pointer_traits<U>::element_type*>) { return std::hash<const typename std::pointer_traits<U>::element_type*>{}(ptr); }
	};

	template<typename Key, typename Hasher = default_hasher<Key>, typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<Key>>
	using hash_set = std::unordered_set<Key, Hasher, KeyEqual, Allocator>;
	template<typename Key, typename Value, typename Hasher = default_hasher<Key>, typename KeyEqual = std::equal_to<>, typename Allocator = std::pair<const Key, Value>>
	using hash_map = std::unordered_map<Key, Value, Hasher, KeyEqual>;
}

#endif // GAL_LANG_UTILS_HASH_CONTAINER_HPP
