#pragma once

#ifndef GAL_UTILS_FLAT_UNORDERED_CONTAINER_HPP
#define GAL_UTILS_FLAT_UNORDERED_CONTAINER_HPP

#include <utils/macro.hpp>
#include <utils/default_hasher.hpp>

DISABLE_WARNING_PUSH
#include<utils/3rd_party_source/unordered_map.hpp>
DISABLE_WARNING_POP

namespace gal::utils
{
	template<
		typename Key,
		typename Value,
		typename Hasher = default_hasher<Key>,
		typename KeyEqual = std::equal_to<>,
		typename Allocator = std::allocator<std::pair<Key, Value>>// Note that Key is not modified by const
	>
	using unordered_hash_map = ska::unordered_map<Key, Value, Hasher, KeyEqual, Allocator>;

	template<typename Key, typename Hasher = default_hasher<Key>, typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<Key>>
	using unordered_hash_set = ska::unordered_set<Key, Hasher, KeyEqual, Allocator>;
}

#endif // GAL_UTILS_FLAT_UNORDERED_CONTAINER_HPP
