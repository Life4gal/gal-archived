#pragma once

#ifndef GAL_LANG_TYPES_RANGE_TYPE_HPP
#define GAL_LANG_TYPES_RANGE_TYPE_HPP

#include <gal/foundation/type_info.hpp>

namespace gal::lang::types
{
	class range_type
	{
	public:
		using size_type = std::ptrdiff_t;

		constexpr static size_type default_step = 1;

		static const foundation::gal_type_info& class_type() noexcept
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)
			static foundation::gal_type_info type = foundation::make_type_info<range_type>();
			return type;
		}

	private:
		size_type begin_;
		size_type end_;
		size_type step_;

	public:
		constexpr range_type(const size_type begin, const size_type end, const size_type step)
			: begin_{begin},
			  end_{end},
			  step_{step} {}

		constexpr explicit range_type(const size_type end)
			: range_type{0, end, default_step} {}

		constexpr range_type(const size_type begin, const size_type end)
			: range_type{begin, end, default_step} {}

		[[nodiscard]] constexpr size_type begin() const noexcept { return begin_; }

		[[nodiscard]] constexpr size_type end() const noexcept { return end_; }

		[[nodiscard]] constexpr size_type step() const noexcept { return step_; }

		[[nodiscard]] constexpr size_type get() const noexcept { return begin_; }

		[[nodiscard]] constexpr bool next() noexcept { return (begin_ += step_) < end_; }

		[[nodiscard]] constexpr size_type size() const noexcept { return (end_ - begin_) / step_; }
	};
}

#endif
