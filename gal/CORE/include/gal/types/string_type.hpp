#pragma once

#ifndef GAL_LANG_TYPES_STRING_TYPE_HPP
#define GAL_LANG_TYPES_STRING_TYPE_HPP

#include <gal/foundation/string.hpp>
#include <gal/types/view_type.hpp>
#include <gal/foundation/type_info.hpp>

namespace gal::lang::types
{
	using char_type = foundation::string_type::value_type;

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
		using pointer = container_type::pointer;
		using const_pointer = container_type::const_pointer;
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

	public:
		constexpr string_type() noexcept = default;

		// internal use only, for register type->string
		constexpr explicit string_type(container_type&& string)
			: data_{std::move(string)} {}

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

		[[nodiscard]] constexpr string_type operator+(const char_type other) const
		{
			auto tmp = *this;
			tmp.data_.push_back(other);
			return tmp;
		}

		constexpr string_type& operator+=(const string_type& other)
		{
			data_ += other.data_;
			return *this;
		}

		constexpr string_type& operator+=(const char_type other)
		{
			data_.push_back(other);
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

		// internal use only, user should use operator+= only
		constexpr string_type& append(const const_pointer string)
		{
			data_.append(string);
			return *this;
		}

		// internal use only, user should use operator+= only
		constexpr string_type& append(const const_pointer string, const size_type count)
		{
			data_.append(string, count);
			return *this;
		}

		// internal use only, user should use operator+= only
		constexpr string_type& append(const string_type& other)
		{
			data_.append(other.data());
			return *this;
		}

		// internal use only, user should use operator+= only
		constexpr string_type& append(const string_type& other, const size_type begin, const size_type count = container_type::npos)
		{
			data_.append(other.data(), begin, count);
			return *this;
		}

		//*************************************************************************
		//*********************** EXTRA INTERFACE *******************************
		//*************************************************************************

		// todo
	};
}

template<>
struct std::hash<gal::lang::types::string_type>
{
	[[nodiscard]] std::size_t operator()(const gal::lang::types::string_type& string) const noexcept { return std::hash<gal::lang::types::string_type::container_type>{}(string.data()); }
};

template<>
struct std::equal_to<gal::lang::types::string_type>
{
	using is_transparent = int;

	[[nodiscard]] bool operator()(const gal::lang::types::string_type& lhs, const gal::lang::types::string_type& rhs) const noexcept { return lhs.data() == rhs.data(); }

	[[nodiscard]] bool operator()(const gal::lang::types::string_view_type& lhs, const gal::lang::types::string_view_type& rhs) const noexcept { return lhs.data() == rhs.data(); }
};

template<>
struct std::hash<gal::lang::types::string_view_type>
{
	[[nodiscard]] std::size_t operator()(const gal::lang::types::string_view_type& object) const noexcept { return std::hash<gal::lang::types::string_view_type::container_type>{}(object.data()); }
};

template<>
struct std::equal_to<gal::lang::types::string_view_type>
{
	using is_transparent = int;

	[[nodiscard]] bool operator()(const gal::lang::types::string_view_type& lhs, const gal::lang::types::string_view_type& rhs) const noexcept { return lhs.data() == rhs.data(); }

	[[nodiscard]] bool operator()(const gal::lang::types::string_type& lhs, const gal::lang::types::string_type& rhs) const noexcept { return lhs.data() == rhs.data(); }
};

#endif // GAL_LANG_TYPES_STRING_TYPE_HPP
