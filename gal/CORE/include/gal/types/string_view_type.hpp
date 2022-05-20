#pragma once

#ifndef GAL_LANG_TYPES_STRING_VIEW_TYPE_HPP
#define GAL_LANG_TYPES_STRING_VIEW_TYPE_HPP

#include <gal/foundation/string.hpp>
#include <gal/types/view_type.hpp>
#include <gal/foundation/type_info.hpp>

namespace gal::lang::types
{
	// todo: better string_view_type
	class string_view_type
	{
	public:
		using container_type = foundation::string_view_type;

		using size_type = container_type::size_type;
		using difference_type = container_type::difference_type;
		using value_type = container_type::value_type;
		using reference = container_type::reference;
		using const_reference = container_type::const_reference;
		using iterator = container_type::iterator;
		using const_iterator = container_type::const_iterator;

		using const_view_type = types::view_type<const container_type>;

		static const foundation::gal_type_info& class_type() noexcept
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
			static foundation::gal_type_info type = foundation::make_type_info<string_view_type>();
			return type;
		}

	private:
		container_type data_;

	public:
		constexpr string_view_type() noexcept = default;

		// for cast from string_type/foundation::string_type
		constexpr explicit string_view_type(const foundation::string_type& string)
			: data_{string} {}

		// for cast from string_view_type
		constexpr explicit string_view_type(const foundation::string_view_type string)
			: data_{string} {}

		// for cast to string_type
		[[nodiscard]] constexpr const container_type& data() const noexcept { return data_; }

		[[nodiscard]] constexpr bool operator==(const string_view_type& other) const noexcept { return data_ == other.data_; }

		[[nodiscard]] constexpr auto operator<=>(const string_view_type& other) const { return data_ <=> other.data_; }

		// view interface
		[[nodiscard]] constexpr const_view_type view() const noexcept { return const_view_type{data_}; }

		//*************************************************************************
		//*********************** BASIC INTERFACE *******************************
		//*************************************************************************

		// operator[]
		[[nodiscard]] constexpr const_reference get(const difference_type index) const noexcept { return data_.at(index); }

		[[nodiscard]] constexpr size_type size() const noexcept { return data_.size(); }

		[[nodiscard]] constexpr bool empty() const noexcept { return data_.empty(); }

		[[nodiscard]] constexpr const_reference front() const noexcept { return data_.front(); }

		[[nodiscard]] constexpr const_reference back() const noexcept { return data_.back(); }

		//*************************************************************************
		//*********************** EXTRA INTERFACE *******************************
		//*************************************************************************

		// todo
	};
}

#endif // GAL_LANG_TYPES_STRING_VIEW_TYPE_HPP
