#pragma once

#ifndef GAL_LANG_UTILS_POINT_HPP
#define GAL_LANG_UTILS_POINT_HPP

#include <string>

namespace gal
{
	struct point
	{
		std::size_t line;
		std::size_t column;

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

	using location = line;
}

#endif// GAL_LANG_UTILS_POINT_HPP
