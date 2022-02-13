#pragma once

#ifndef GAL_UTILS_INITIALIZER_LIST_HPP
#define GAL_UTILS_INITIALIZER_LIST_HPP

namespace gal::utils
{
	template<typename T>
	class initializer_list
	{
	public:
		using value_type = T;
		using size_type = std::size_t;

		using iterator = value_type*;
		using const_iterator = const value_type*;

		using reference = value_type&;
		using const_reference = const value_type&;

	private:
		const_iterator begin_;
		const_iterator end_;

	public:
		constexpr initializer_list(const const_iterator begin, const const_iterator end) noexcept
			: begin_{begin},
			  end_{end} {}

		constexpr explicit initializer_list(const_reference object) noexcept
			: begin_{&object},
			  end_{begin_ + 1} {}

		template<template<typename> typename Container>
			requires requires(Container<value_type> container)
			{
				container.size();
				container.empty();
				{
					container.begin()
				} -> std::convertible_to<const_iterator>;
				{
					container.end()
				} -> std::convertible_to<const_iterator>;
			}
		constexpr explicit initializer_list(const Container<value_type>& container) noexcept
			: begin_{container.empty() ? nullptr : container.begin()},
			  end_{container.empty() ? nullptr : container.end()} {}

		template<size_type Size, template<typename, size_type> typename Container>
			requires(Size != 0) && requires(Container<value_type, Size> container)
			{
				{
					container.begin()
				} -> std::convertible_to<const_iterator>;
				{
					container.end()
				} -> std::convertible_to<const_iterator>;
			}
		constexpr explicit initializer_list(const Container<value_type, Size>& container) noexcept
			: begin_{container.begin()},
			  end_{container.end()} {}

		template<size_type Size, template<typename, size_type> typename Container>
			requires(Size == 0)
		constexpr explicit initializer_list(const Container<value_type, Size>&) noexcept
			: begin_{nullptr},
			  end_{nullptr} {}

		[[nodiscard]] constexpr const_iterator begin() const noexcept { return begin_; }

		[[nodiscard]] constexpr const_iterator end() const noexcept { return end_; }

		[[nodiscard]] constexpr const_reference front() const noexcept { return *begin(); }

		[[nodiscard]] constexpr const_reference back() const noexcept { return *(end() - 1); }

		[[nodiscard]] constexpr size_type size() const noexcept { return static_cast<size_type>(end() - begin()); }

		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		[[nodiscard]] constexpr const_reference operator[](const std::size_t index) const noexcept { return begin()[index]; }

		template<template<typename> typename Container>
			requires std::is_constructible_v<Container<value_type>, const_iterator, const_iterator>
		[[nodiscard]] Container<value_type> to() const { return Container<value_type>{begin(), end()}; }

		template<
			template<typename>
			typename Container,
			void (Container<value_type>::*PushFunction)(const value_type&) = &Container<value_type>::push_back>
		[[nodiscard]] Container<value_type> to() const
		{
			Container<value_type> ret{};
			std::ranges::for_each(
					begin(),
					end(),
					[&](const value_type& v) { ret.PushFunction(v); });
			return ret;
		}
	};
}

#endif // GAL_UTILS_INITIALIZER_LIST_HPP
