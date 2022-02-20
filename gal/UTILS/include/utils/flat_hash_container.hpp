#pragma once

#ifndef GAL_UTILS_FLAT_HASH_CONTAINER_HPP
#define GAL_UTILS_FLAT_HASH_CONTAINER_HPP

#include <utils/macro.hpp>
#include <utils/default_hasher.hpp>
#include<utils/3rd_party_source/flat_hash_map.hpp>

namespace gal::utils
{
	template<
		typename Key,
		typename Value,
		typename Hasher = default_hasher<Key>,
		typename KeyEqual = std::equal_to<>,
		typename Allocator = std::allocator<std::pair<Key, Value>>// Note that Key is not modified by const
	>
	using flat_hash_map = ska::flat_hash_map<Key, Value, Hasher, KeyEqual, Allocator>;

	template<typename Key, typename Hasher = default_hasher<Key>, typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<Key>>
	using flat_hash_set = ska::flat_hash_set<KeyEqual, Hasher, KeyEqual, Allocator>;
}

#endif // GAL_UTILS_FLAT_HASH_CONTAINER_HPP
