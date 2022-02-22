#pragma once

#ifndef GAL_LANG_UTILITY_FLAT_CONTINUOUS_MAP_HPP
#define GAL_LANG_UTILITY_FLAT_CONTINUOUS_MAP_HPP

#include <vector>
#include <utils/assert.hpp>

namespace gal::lang::utility
{
	template<typename Key, typename Value, typename KeyEqual = std::equal_to<>, typename Allocator = std::allocator<std::pair<Key, Value>>>
	class flat_continuous_map
	{
	public:
		using key_type = Key;
		using mapped_type = Value;
		using value_type = std::pair<key_type, mapped_type>;

		using key_reference = key_type&;
		using key_const_reference = const key_type&;
		using mapped_reference = mapped_type&;
		using mapped_const_reference = const mapped_type&;

		using key_comparator_type = KeyEqual;
		using allocator_type = Allocator;

		static_assert(std::is_same_v<typename allocator_type::value_type, value_type>);

	private:
		using internal_container_type = std::vector<value_type>;

		[[no_unique_address]] key_comparator_type key_comparator_;
		[[no_unique_address]] allocator_type allocator_;
		internal_container_type data_;

	public:
		using size_type = typename internal_container_type::size_type;
		using iterator = typename internal_container_type::iterator;
		using const_iterator = typename internal_container_type::const_iterator;
		using reference = typename internal_container_type::reference;
		using const_reference = typename internal_container_type::const_reference;

	private:
		void grow()
		{
			if (data_.capacity() == data_.size())
			{
				// todo: calc grow size
				data_.reserve(data_.size() + 2);
			}
		}

	public:
		[[nodiscard]] constexpr size_type size() const noexcept { return data_.size(); }

		[[nodiscard]] constexpr bool empty() const noexcept { return data_.empty(); }

		constexpr void clear() noexcept { data_.clear(); }

		[[nodiscard]] constexpr iterator begin() noexcept { return data_.begin(); }

		[[nodiscard]] constexpr iterator end() noexcept { return data_.end(); }

		[[nodiscard]] constexpr const_iterator begin() const noexcept { return data_.begin(); }

		[[nodiscard]] constexpr const_iterator end() const noexcept { return data_.end(); }

		[[nodiscard]] constexpr reference back() noexcept { return data_.back(); }

		[[nodiscard]] constexpr const_reference back() const noexcept { return data_.back(); }

		template<typename K>
		[[nodiscard]] constexpr iterator find(const K& key) noexcept
		{
			return std::ranges::find_if(
					data_,
					[&](const auto& pair) { return key_comparator_(pair.first, key); });
		}

		template<typename K>
		[[nodiscard]] constexpr const_iterator find(const K& key) const noexcept { return const_cast<flat_continuous_map&>(*this).find(key); }

		template<typename K>
		[[nodiscard]] constexpr const_iterator find(const K& key, const size_type hint) const noexcept
		{
			if (size() > hint && key_comparator_(data_[hint].first, key)) { return std::ranges::next(begin(), static_cast<typename internal_container_type::difference_type>(hint)); }
			return find(key);
		}

		template<typename K>
		[[nodiscard]] constexpr bool count(const K& key) const noexcept { return find(key) == end() ? false : true; }

		template<typename K>
		[[nodiscard]] constexpr bool contain(const K& key) const noexcept { return count(key); }

		[[nodiscard]] constexpr mapped_reference operator[](key_const_reference key)
			requires std::is_default_constructible_v<mapped_type>
		{
			if (const auto it = find(key);
				it != end()) { return it->second; }
			grow();
			return data_.emplace_back(key, mapped_type{}).second;
		}

		[[nodiscard]] constexpr mapped_reference at(const size_type index) noexcept
		{
			gal_assert(index < size(), "index out of range");
			return data_[index].second;
		}

		[[nodiscard]] constexpr mapped_const_reference at(const size_type index) const noexcept { return const_cast<flat_continuous_map&>(*this).at(index); }

		[[nodiscard]] constexpr mapped_reference at(const key_type& key)
		{
			if (const auto it = find(key);
				it != end()) { return it->second; }
			throw std::out_of_range{"key not exist"};
		}

		[[nodiscard]] constexpr mapped_const_reference at(const key_type& key) const { return const_cast<flat_continuous_map&>(*this).at(key); }

		template<typename Iterator>
		constexpr void assign(Iterator begin, Iterator end)
			requires requires
			{
				data_.assign(begin, end);
			} { data_.assign(begin, end); }

		constexpr std::pair<iterator, bool> insert(value_type&& value)
		{
			if (const auto it = find(value.first);
				it != end()) { return {it, false}; }
			grow();
			return {data_.insert(end(), std::move(value)), true};
		}

		template<typename K, typename M>
		constexpr std::pair<iterator, bool> emplace(K&& key, M&& mapped)
		{
			if (const auto it = find(key);
				it != end()) { return {it, false}; }
			grow();
			return {data_.emplace(end(), std::forward<K>(key), std::forward<M>(mapped)), true};
		}

		template<typename M>
		constexpr std::pair<iterator, bool> insert_or_assign(const key_type& key, M&& mapped) requires requires
		{
			std::declval<mapped_reference>() = std::forward<M>(mapped);
		}
		{
			if (const auto it = find(key);
				it != end())
			{
				it->second = std::forward<M>(mapped);
				return {it, false};
			}
			grow();
			return {data_.emplace(end(), key, std::forward<M>(mapped)), true};
		}

		template<typename M>
		constexpr std::pair<iterator, bool> insert_or_assign(key_type&& key, M&& mapped)
			requires requires
			{
				std::declval<mapped_reference>() = std::forward<M>(mapped);
			}
		{
			if (const auto it = find(key);
				it != end())
			{
				it->second = std::forward<M>(mapped);
				return {it, false};
			}
			grow();
			return {data_.emplace(end(), std::move(key), std::forward<M>(mapped)), true};
		}

		constexpr void push_back(const value_type& value) { data_.push_back(value); }

		constexpr void push_back(value_type&& value) { data_.push_back(std::move(value)); }

		template<typename... Args>
		constexpr reference emplace_back(Args&&... args) { return data_.emplace_back(std::forward<Args>(args)...); }
	};
}

#endif // GAL_LANG_UTILITY_FLAT_CONTINUOUS_MAP_HPP
