#pragma once

#ifndef GAL_LANG_TYPES_LIST_TYPE_HPP
#define GAL_LANG_TYPES_LIST_TYPE_HPP

#include <list>

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

		private:
			container_type data_;

		public:
			list_type() noexcept = default;

			list_type(const std::initializer_list<value_type> values)
				: data_{values} {}

			// operator[]
			[[nodiscard]] reference get(difference_type index)
			{
				index %= static_cast<difference_type>(data_.size());
				if (index >= 0) { return *std::ranges::next(data_.begin(), index); }
				return *std::ranges::prev(data_.end(), -index);
			}

			// operator[]
			[[nodiscard]] const_reference get(difference_type index) const
			{
				index %= static_cast<difference_type>(data_.size());
				if (index >= 0) { return *std::ranges::next(data_.begin(), index); }
				return *std::ranges::prev(data_.end(), -index);
			}

			// operator[begin:end]
			[[nodiscard]] auto slice(difference_type begin, difference_type end)
			{
				begin %= static_cast<difference_type>(data_.size());
				end %= static_cast<difference_type>(data_.size());

				// [1, 2, 3, 4, 5, 6]
				//      ^begin = 1/-5
				//                 ^end = 4/-2
				// => [2, 3, 4]

				// [1, 2, 3, 4, 5, 6]
				//                 ^begin = 4/-2
				//      ^end = 1/-5
				// => []
				// todo: maybe return [5, 6, 1] ?

				if (begin < 0) { begin = static_cast<difference_type>(data_.size()) + begin; }
				if (end < 0) { end = static_cast<difference_type>(data_.size()) + end; }

				if (begin <= end) { return data_ | std::views::drop(begin) | std::views::take(end - begin); }
				return data_ | std::views::drop(0) | std::views::take(0);
			}

			[[nodiscard]] auto slice_front(const difference_type begin) { return slice(begin, data_.size()); }

			[[nodiscard]] auto slice_back(const difference_type end) { return slice(0, end); }

			[[nodiscard]] size_type size() const noexcept { return data_.size(); }

			[[nodiscard]] iterator begin() noexcept { return data_.begin(); }

			[[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }

			[[nodiscard]] iterator end() noexcept { return data_.end(); }

			[[nodiscard]] const_iterator end() const noexcept { return data_.end(); }

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
