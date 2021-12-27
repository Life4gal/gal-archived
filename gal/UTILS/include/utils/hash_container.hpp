#pragma once

#ifndef GAL_LANG_UTILS_HASH_CONTAINER_HPP
#define GAL_LANG_UTILS_HASH_CONTAINER_HPP

// todo: implement it

#include <unordered_map>
#include <unordered_set>

namespace gal::utils
{
	template<typename Key, typename Hasher = std::hash<Key>, typename KeyEqual = std::equal_to<>>
	using hash_set = std::unordered_set<Key, Hasher, KeyEqual>;
	template<typename Key, typename Value, typename Hasher = std::hash<Key>, typename KeyEqual = std::equal_to<>>
	using hash_map = std::unordered_map<Key, Value, Hasher, KeyEqual>;
}

#endif // GAL_LANG_UTILS_HASH_CONTAINER_HPP
