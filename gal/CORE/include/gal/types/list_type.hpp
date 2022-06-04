#pragma once

#ifndef GAL_LANG_TYPES_LIST_TYPE_HPP
#define GAL_LANG_TYPES_LIST_TYPE_HPP

#include <list>
#include <gal/types/view_type.hpp>
#include <gal/foundation/type_info.hpp>

namespace gal::lang
{
	namespace foundation
	{
		class boxed_value;
	}

	namespace types
	{
		class list_type
		{
		public:
			using container_type = std::list<foundation::boxed_value>;

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
				static foundation::gal_type_info type = foundation::make_type_info<list_type>();
				return type;
			}

		private:
			container_type data_;

			[[nodiscard]] difference_type locate_index(const difference_type index) const noexcept
			{
				auto i = index % static_cast<difference_type>(data_.size());
				if (i < 0) { i = static_cast<difference_type>(data_.size()) + i; }
				return i;
			}

			explicit list_type(container_type&& list)
				: data_{std::move(list)} {}

		public:
			list_type() noexcept = default;

			explicit list_type(foundation::parameters_type&& list)
				: data_{std::make_move_iterator(list.begin()), std::make_move_iterator(list.end())} {}

			explicit list_type(const foundation::parameters_view_type list)
				: data_{list.begin(), list.end()} {}

			// [[nodiscard]] list_type operator+(const list_type& other) const
			// {
			// 	auto tmp = data_;
			// 	tmp.insert(tmp.end(), other.data_.begin(), other.data_.end());
			// 	return list_type{std::move(tmp)};
			// }
			//
			// list_type& operator+=(const list_type& other)
			// {
			// 	data_.insert(data_.end(), other.data_.begin(), other.data_.end());
			// 	return *this;
			// }
			//
			// [[nodiscard]] list_type operator*(const size_type times) const
			// {
			// 	auto tmp = data_;
			// 	for (auto i = times; i != 0; --i) { tmp.insert(tmp.end(), data_.begin(), data_.end()); }
			// 	return list_type{std::move(tmp)};
			// }
			//
			// list_type& operator*=(const size_type times)
			// {
			// 	const auto e = data_.end();
			// 	for (auto i = times; i != 0; --i) { data_.insert(data_.end(), data_.begin(), e); }
			// 	return *this;
			// }

			// view interface
			[[nodiscard]] view_type view() noexcept { return view_type{data_}; }
			[[nodiscard]] const_view_type view() const noexcept { return const_view_type{data_}; }

			//*************************************************************************
			//*********************** BASIC INTERFACE *******************************
			//*************************************************************************

			// operator[]
			[[nodiscard]] reference get(const difference_type index) noexcept { return *std::ranges::next(data_.begin(), locate_index(index)); }

			// operator[]
			[[nodiscard]] const_reference get(const difference_type index) const noexcept { return *std::ranges::next(data_.begin(), locate_index(index)); }

			[[nodiscard]] size_type size() const noexcept { return data_.size(); }

			[[nodiscard]] bool empty() const noexcept { return data_.empty(); }

			void clear() noexcept { data_.clear(); }

			[[nodiscard]] reference front() noexcept { return data_.front(); }

			[[nodiscard]] const_reference front() const noexcept { return data_.front(); }

			[[nodiscard]] reference back() noexcept { return data_.back(); }

			[[nodiscard]] const_reference back() const noexcept { return data_.back(); }

			void insert_at(const difference_type index, const_reference value) noexcept { data_.insert(std::ranges::next(data_.begin(), locate_index(index)), value); }

			void erase_at(const difference_type index) { data_.erase(std::ranges::next(data_.begin(), locate_index(index))); }

			void push_back(const_reference value) { data_.push_back(value); }

			void pop_back() { data_.pop_back(); }

			void push_front(const_reference value) { data_.push_front(value); }

			void pop_front() { data_.pop_front(); }

			//*************************************************************************
			//*********************** EXTRA INTERFACE *******************************
			//*************************************************************************

			// operator[begin:end]
			[[nodiscard]] auto slice(const difference_type begin, const difference_type end)
			{
				// [1, 2, 3, 4, 5, 6]
				//      ^begin = 1/-5
				//                 ^end = 4/-2
				// => [2, 3, 4]

				// [1, 2, 3, 4, 5, 6]
				//                 ^begin = 4/-2
				//      ^end = 1/-5
				// => []
				// todo: maybe return [5, 6, 1] ?

				const auto b = locate_index(begin);
				const auto e = locate_index(end);

				if (b <= e) { return data_ | std::views::drop(b) | std::views::take(e - b); }
				return data_ | std::views::drop(0) | std::views::take(0);
			}

			[[nodiscard]] auto slice_front(const difference_type begin) { return slice(begin, data_.size()); }

			[[nodiscard]] auto slice_back(const difference_type end) { return slice(0, end); }

			void reverse() { data_.reverse(); }

			// the type cast of the sorting function is up to the caller
			// note: boxed_value does not have any form of comparison operation, so there is no default sort method
			template<typename Predicate>
				requires std::is_invocable_r_v<bool, Predicate, const foundation::boxed_value&, const foundation::boxed_value>
			void sort(Predicate&& p) { data_.sort(std::forward<Predicate>(p)); }

			// the type cast of the sorting function is up to the caller
			// note: boxed_value does not have any form of comparison operation, so there is no default unique method
			template<typename Predicate>
				requires std::is_invocable_r_v<bool, Predicate, const foundation::boxed_value&, const foundation::boxed_value>
			void unique(Predicate&& p) { data_.unique(std::forward<Predicate>(p)); }

			// the type cast of the sorting function is up to the caller
			// note: boxed_value does not have any form of comparison operation, so there is no default count method
			template<typename Predicate>
				requires std::is_invocable_r_v<bool, Predicate, const foundation::boxed_value&>
			[[nodiscard]] size_type count_if(Predicate&& p) { return std::ranges::count_if(data_, std::forward<Predicate>(p)); }

			// todo: more interface
		};
	}
}

#endif // GAL_LANG_TYPES_LIST_TYPE_HPP
