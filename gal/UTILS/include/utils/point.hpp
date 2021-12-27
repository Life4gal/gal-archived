#pragma once

#ifndef GAL_LANG_UTILS_POINT_HPP
#define GAL_LANG_UTILS_POINT_HPP

#include <string>

namespace gal::utils
{
	struct point
	{
		using size_type = std::size_t;

		size_type line;
		size_type column;

		[[nodiscard]] constexpr bool operator==(const point& rhs) const { return line == rhs.line && column == rhs.column; }

		[[nodiscard]] constexpr auto operator<=>(const point& rhs) const
		{
			if (const auto ordering = line <=> rhs.line;
				ordering == std::strong_ordering::equal) { [[unlikely]] return column <=> rhs.column; }
			else { [[likely]] return ordering; }
		}

		[[nodiscard]] std::string to_string() const;
	};

	using position = point;

	struct line
	{
		point begin;
		point end;

		[[nodiscard]] constexpr bool operator==(const line& rhs) const { return begin == rhs.begin && end == rhs.end; }

		[[nodiscard]] constexpr bool operator!=(const line& rhs) const { return not this->operator==(rhs); }

		template<bool Closed>
		[[nodiscard]] constexpr bool contain(const point& p) const
		{
			if constexpr (Closed) { return begin <= p && p <= end; }
			else { return begin <= p && p < end; }
		}

		[[nodiscard]] std::string to_string() const;
	};

	constexpr line make_horizontal_line(const point begin, const point::size_type length)
	{
		return {begin, {begin.line, begin.column + length}};
	}

	constexpr line make_vertical_line(const point begin, const point::size_type length)
	{
		return {begin, {begin.line + length, begin.column}};
	}

	using location = line;
}

#endif// GAL_LANG_UTILS_POINT_HPP
