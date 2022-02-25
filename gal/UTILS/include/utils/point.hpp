#pragma once

#ifndef GAL_UTILS_POINT_HPP
#define GAL_UTILS_POINT_HPP

#include <type_traits>

namespace gal::utils
{
	template<typename T>
	struct basic_point
	{
		using size_type = T;

		size_type line{};
		size_type column{};

		template<typename U = T>
		[[nodiscard]] constexpr bool operator==(const basic_point<U>& rhs) const noexcept
		{
			using common_size_type = std::common_type_t<size_type, typename basic_point<U>::size_type>;
			return static_cast<common_size_type>(line) == static_cast<common_size_type>(rhs.line) && static_cast<common_size_type>(column) == static_cast<common_size_type>(rhs.column);
		}

		template<typename U = T>
		[[nodiscard]] constexpr auto operator<=>(const basic_point<U>& rhs) const noexcept
		{
			using common_size_type = std::common_type_t<size_type, typename basic_point<U>::size_type>;
			if (const auto ordering = static_cast<common_size_type>(line) <=> static_cast<common_size_type>(rhs.line);
				ordering == std::strong_ordering::equal) { [[unlikely]] return static_cast<common_size_type>(column) <=> static_cast<common_size_type>(rhs.column); }
			else { [[likely]] return ordering; }
		}
	};

	template<typename T>
	using basic_position = basic_point<T>;

	template<typename T>
	struct basic_line
	{
		using point_type = basic_point<T>;
		using size_type = typename point_type::size_type;

		point_type begin;
		point_type end;

		template<typename U = T>
		[[nodiscard]] constexpr bool operator==(const basic_line<U>& rhs) const noexcept { return begin == rhs.begin && end == rhs.end; }

		template<bool Closed, typename U>
		[[nodiscard]] constexpr bool contain(const basic_point<U>& p) const noexcept
		{
			if constexpr (Closed) { return begin <= p && p <= end; }
			else { return begin <= p && p < end; }
		}
	};

	template<typename T>
	using basic_location = basic_line<T>;

	using point = basic_point<std::size_t>;
	using position = point;
	using line = basic_line<std::size_t>;
	using location = line;

	template<typename T = line::size_type>
	constexpr basic_line<T> make_horizontal_line(const basic_point<T> begin, const typename basic_point<T>::size_type length) { return {begin, {begin.line, begin.column + length}}; }

	template<typename T = line::size_type>
	constexpr basic_line<T> make_vertical_line(const basic_point<T> begin, const typename basic_point<T>::size_type length) { return {begin, {begin.line + length, begin.column}}; }

	template<typename T = line::size_type>
	constexpr basic_line<T> make_longest_line(const basic_line<T> begin, const std::type_identity_t<basic_line<T>> end) { return {begin.begin, end.end}; }
}

#endif// GAL_UTILS_POINT_HPP
