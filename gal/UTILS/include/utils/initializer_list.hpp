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

		template<template<typename...> typename Container, typename... AnyOther>
			requires requires(const Container<value_type, AnyOther...>& container)
			{
				container.size();
				container.empty();
				{
					container.data()
				} -> std::convertible_to<const_iterator>;
			}
		constexpr explicit initializer_list(const Container<value_type, AnyOther...>& container) noexcept
			: begin_{container.empty() ? nullptr : container.data()},
			  end_{container.empty() ? nullptr : container.data() + container.size()} {}

		template<size_type Size, template<typename, size_type> typename Container>
			requires(Size != 0) && requires(Container<value_type, Size> container)
			{
				{
					container.data()
				} -> std::convertible_to<const_iterator>;
			}
		constexpr explicit initializer_list(const Container<value_type, Size>& container) noexcept
			: begin_{container.data()},
			  end_{container.data() + Size} {}

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

		template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<value_type, AnyOther...>, const_iterator, const_iterator>
		[[nodiscard]] Container<value_type> to() const { return Container<value_type, AnyOther...>(begin(), end()); }

		template<typename Container>
			requires std::is_constructible_v<Container, const_iterator, const_iterator>
		[[nodiscard]] Container to() const { return Container(begin(), end()); }

		template<
			template<typename...>
			typename Container,
			typename... AnyOther,
			typename PushFunction
		>
			requires std::is_invocable_v<PushFunction, Container<value_type, AnyOther...>&, const_reference>
		[[nodiscard]] Container<value_type, AnyOther...> to(PushFunction push_function) const
		{
			Container<value_type, AnyOther...> ret{};
			std::ranges::for_each(
					begin(),
					end(),
					[&](const_reference v) { push_function(ret, v); });
			return ret;
		}

		template<
			template<typename...>
			typename Container,
			typename... AnyOther>
			requires (not std::is_constructible_v<Container<value_type, AnyOther...>, const_iterator, const_iterator>) &&
			         requires(Container<value_type, AnyOther...>& container)
			         {
				         container.push_back(std::declval<const_reference>());
			         }
		[[nodiscard]] Container<value_type, AnyOther...> to() const { return to<Container, AnyOther...>([](auto& container, const_reference v) { container.push_back(v); }); }

		template<typename Container, typename PushFunction>
			requires std::is_invocable_v<PushFunction, Container&, const_reference>
		[[nodiscard]] Container to(PushFunction push_function) const
		{
			Container ret{};
			std::ranges::for_each(
					begin(),
					end(),
					[&](const value_type& v) { push_function(ret, v); });
			return ret;
		}

		template<typename Container>
			requires (not std::is_constructible_v<Container, const_iterator, const_iterator>) &&
			         requires(Container& container)
			         {
				         container.push_back(std::declval<const_reference>());
			         }
		[[nodiscard]] Container to() const { return to<Container>([](auto& container, const_reference v) { container.push_back(v); }); }
	};
}

#endif // GAL_UTILS_INITIALIZER_LIST_HPP
