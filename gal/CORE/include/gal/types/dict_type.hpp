#pragma once

#ifndef GAL_LANG_TYPES_DICT_TYPE_HPP
	#define GAL_LANG_TYPES_DICT_TYPE_HPP

#include <unordered_map>
#include <utils/format.hpp>
#include <gal/types/view_type.hpp>
#include <gal/types/string_view_type.hpp>
#include <gal/foundation/type_info.hpp>

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
			explicit key_not_found_error(const types::string_view_type key)
				: out_of_range(std_format::format("key '{}' not found in the immutable dict", key.data())) {}
		};
	}

	namespace types
	{
		class dict_type
		{
		public:
			// key is immutable
			using container_type = std::unordered_map<string_view_type, foundation::boxed_value, std::hash<string_view_type>, std::equal_to<>>;

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

			using view_type = types::view_type<container_type>;
			using const_view_type = types::view_type<const container_type>;

			static const foundation::gal_type_info& class_type() noexcept
			{
				GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
				static foundation::gal_type_info type = foundation::make_type_info<dict_type>();
				return type;
			}

			static const foundation::gal_type_info& pair_class_type() noexcept
			{
				GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
				static foundation::gal_type_info type = foundation::make_type_info<value_type>();
				return type;
			}

		private:
			container_type data_;

			explicit dict_type(container_type&& map)
				: data_{std::move(map)} {}

		public:
			dict_type() noexcept = default;

			// [[nodiscard]] map_type operator+(const map_type& other) const
			// {
			// 	auto tmp = data_;
			// 	tmp.insert(other.data_.begin(), other.data_.end());
			// 	return map_type{std::move(tmp)};
			// }
			//
			// map_type& operator+=(const map_type& other)
			// {
			// 	data_.insert(other.data_.begin(), other.data_.end());
			// 	return *this;
			// }

			// view interface
			[[nodiscard]] view_type view() noexcept { return view_type{data_}; }
			[[nodiscard]] const_view_type view() const noexcept { return const_view_type{data_}; }

			//*************************************************************************
			//*********************** BASIC INTERFACE *******************************
			//*************************************************************************

			// operator[]
			[[nodiscard]] mapped_reference get(key_const_reference key) { return data_[key]; }

			// operator[]
			[[nodiscard]] mapped_const_reference get(key_const_reference key) const
			{
				if (const auto it = data_.find(key); it != data_.end()) { return it->second; }

				throw exception::key_not_found_error{key};
			}

			[[nodiscard]] size_type size() const noexcept { return data_.size(); }

			[[nodiscard]] bool empty() const noexcept { return data_.empty(); }

			void clear() noexcept { data_.clear(); }

			// insert is not necessary

			void erase_at(key_const_reference key) { data_.erase(key); }

			// internal use only
			template<typename... Args>
			decltype(auto) emplace(Args&&... args) { return data_.emplace(std::forward<Args>(args)...); }

			//*************************************************************************
			//*********************** EXTRA INTERFACE *******************************
			//*************************************************************************

			// todo
		};
	}
}

#endif // GAL_LANG_TYPES_DICT_TYPE_HPP
