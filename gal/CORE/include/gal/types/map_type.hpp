#pragma once

#ifndef GAL_LANG_TYPES_MAP_TYPE_HPP
#define GAL_LANG_TYPES_MAP_TYPE_HPP

#include <unordered_map>
#include <utils/format.hpp>

namespace gal::lang
{
	namespace foundation
	{
		class boxed_value;
	}

	namespace exception
	{
		class key_not_found_error final : public std::out_of_range
		{
		public:
			explicit key_not_found_error(const std::string_view key)
				: out_of_range(std_format::format("key '{}' not found in the map", key)) {}
		};
	}

	namespace types
	{
		template<typename First, typename Second>
		using pair_type = std::pair<First, Second>;

		class map_type
		{
		public:
			using container_type = std::unordered_map<foundation::string_view_type, foundation::boxed_value, std::hash<foundation::string_view_type>, std::equal_to<>>;

			using size_type = container_type::size_type;
			using difference_type = container_type::difference_type;
			using value_type = container_type::value_type;
			using reference = container_type::reference;
			using const_reference = container_type::const_reference;
			using iterator = container_type::iterator;
			using const_iterator = container_type::const_iterator;

			using key_type = container_type::key_type;
			using key_reference = key_type&;
			using key_const_reference = const key_type&;
			using mapped_type = container_type::mapped_type;
			using mapped_reference = mapped_type&;
			using mapped_const_reference = const mapped_type&;

		private:
			container_type data_;

		public:
			map_type() noexcept = default;

			map_type(const std::initializer_list<value_type> values)
				: data_{values} {}

			// operator[]
			[[nodiscard]] mapped_reference get(const key_type& key) { return data_[key]; }

			// operator[]
			[[nodiscard]] mapped_const_reference get(const key_type& key) const
			{
				if (const auto it = data_.find(key); it != data_.end()) { return it->second; }

				throw exception::key_not_found_error{key};
			}

			[[nodiscard]] size_type size() const noexcept { return data_.size(); }

			[[nodiscard]] iterator begin() noexcept { return data_.begin(); }

			[[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }

			[[nodiscard]] iterator end() noexcept { return data_.end(); }

			[[nodiscard]] const_iterator end() const noexcept { return data_.end(); }
		};
	}
}

#endif // GAL_LANG_TYPES_MAP_TYPE_HPP
