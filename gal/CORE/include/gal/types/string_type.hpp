#pragma once

#ifndef GAL_LANG_TYPES_STRING_TYPE_HPP
#define GAL_LANG_TYPES_STRING_TYPE_HPP

#include <gal/foundation/string.hpp>
#include <gal/types/view_type.hpp>
#include <gal/foundation/type_info.hpp>

namespace gal::lang::types
{
	// todo: better string_type
	class string_type
	{
	public:
		using container_type = foundation::string_type;

		using size_type = container_type::size_type;
		using difference_type = container_type::difference_type;
		using value_type = container_type::value_type;
		using reference = container_type::reference;
		using const_reference = container_type::const_reference;
		using iterator = container_type::iterator;
		using const_iterator = container_type::const_iterator;

		using view_type = types::view_type<container_type>;
		using const_view_type = types::view_type<const container_type>;

		static const foundation::gal_type_info& class_type() noexcept
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
			static foundation::gal_type_info type = foundation::make_type_info<string_type>();
			return type;
		}

	private:
		container_type data_;

		constexpr explicit string_type(container_type&& string)
			: data_{std::move(string)} {}

	public:
		constexpr string_type() noexcept = default;

		// for cast from foundation::string_type
		constexpr explicit string_type(const foundation::string_type& string)
			: data_{string} {}

		// for cast from string_view_type/foundation::string_view_type
		constexpr explicit string_type(const foundation::string_view_type string)
			: data_{string} {}

		// for cast to string_view_type
		[[nodiscard]] constexpr const container_type& data() const noexcept { return data_; }

		[[nodiscard]] constexpr bool operator==(const string_type& other) const noexcept { return data_ == other.data_; }

		[[nodiscard]] constexpr auto operator<=>(const string_type& other) const { return data_ <=> other.data_; }

		[[nodiscard]] constexpr string_type operator+(const string_type& other) const { return string_type{data_ + other.data_}; }

		constexpr string_type& operator+=(const string_type& other)
		{
			data_ += other.data_;
			return *this;
		}

		[[nodiscard]] constexpr string_type operator*(const size_type times) const
		{
			auto tmp = data_;
			for (auto i = times; i != 0; --i) { tmp.append(data_); }
			return string_type{std::move(tmp)};
		}

		constexpr string_type& operator*=(const size_type times)
		{
			const auto s = size();
			for (auto i = times; i != 0; --i) { data_.append(data_, 0, s); }
			return *this;
		}

		// view interface
		[[nodiscard]] constexpr view_type view() noexcept { return view_type{data_}; }
		[[nodiscard]] constexpr const_view_type view() const noexcept { return const_view_type{data_}; }

		//*************************************************************************
		//*********************** BASIC INTERFACE *******************************
		//*************************************************************************

		// operator[]
		[[nodiscard]] constexpr reference get(const difference_type index) noexcept { return data_.at(index); }

		// operator[]
		[[nodiscard]] constexpr const_reference get(const difference_type index) const noexcept { return data_.at(index); }

		[[nodiscard]] constexpr size_type size() const noexcept { return data_.size(); }

		[[nodiscard]] constexpr bool empty() const noexcept { return data_.empty(); }

		constexpr void clear() noexcept { data_.clear(); }

		[[nodiscard]] constexpr reference front() noexcept { return data_.front(); }

		[[nodiscard]] constexpr const_reference front() const noexcept { return data_.front(); }

		[[nodiscard]] constexpr reference back() noexcept { return data_.back(); }

		[[nodiscard]] constexpr const_reference back() const noexcept { return data_.back(); }

		constexpr void insert_at(const difference_type index, const_reference value) noexcept { data_.insert(std::ranges::next(data_.begin(), index), value); }

		constexpr void erase_at(const difference_type index) { data_.erase(std::ranges::next(data_.begin(), index)); }

		constexpr void push_back(const_reference value) { data_.push_back(value); }

		constexpr void pop_back() { data_.pop_back(); }

		//*************************************************************************
		//*********************** EXTRA INTERFACE *******************************
		//*************************************************************************

		// todo
	};
}

#endif // GAL_LANG_TYPES_STRING_TYPE_HPP
