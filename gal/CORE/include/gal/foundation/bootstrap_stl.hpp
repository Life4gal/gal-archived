#pragma once

#ifndef GAL_LANG_FOUNDATION_BOOTSTRAP_STL_HPP
#define GAL_LANG_FOUNDATION_BOOTSTRAP_STL_HPP

#include <gal/function_register.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/foundation/operator_register.hpp>
#include <gal/foundation/proxy_function.hpp>
#include <utils/type_traits.hpp>

namespace gal::lang::foundation
{
	namespace bootstrap_stl_detail
	{
		template<typename ContainerType>
		struct bidirectional_range
		{
			using container_type = ContainerType;
			constexpr static bool is_const_container = std::is_const_v<container_type>;

			using iterator_type = std::conditional_t<is_const_container, typename container_type::const_iterator, typename container_type::iterator>;

		private:
			iterator_type begin_;
			iterator_type end_;

		public:
			constexpr explicit bidirectional_range(container_type& container)
				: begin_{container.begin()},
				  end_{container.end()} {}

			[[nodiscard]] constexpr bool empty() const noexcept { return begin_ == end_; }

			constexpr void pop_front()
			{
				if (empty()) { throw std::range_error{"empty range"}; }
				++begin_;
			}

			constexpr void pop_back()
			{
				if (empty()) { throw std::range_error{"empty range"}; }
				--end_;
			}

			[[nodiscard]] constexpr decltype(auto) front() const
			{
				if (empty()) { throw std::range_error{"empty range"}; }
				return *begin_;
			}

			[[nodiscard]] constexpr decltype(auto) back() const
			{
				if (empty()) { throw std::range_error{"empty_range"}; }
				return *std::ranges::prev(end_);
			}
		};

		template<typename T>
		concept sequence_container_t = true;//std::is_same_v<typename T::value_type, boxed_value>;
		template<typename T>
		concept associative_container_t = true;//std::is_same_v<typename T::mapped_type, boxed_value>;
		template<typename T>
		concept sequence_or_associative_container_t = sequence_container_t<T> || associative_container_t<T>;

		namespace detail
		{
			template<typename Container>
			using detect_key_type = typename Container::key_type;
		}

		template<typename Container>
		using container_key_type = utils::detected_or_t<typename Container::value_type, detail::detect_key_type, Container>;

		template<sequence_or_associative_container_t ContainerType>
		void register_default_constructible_container(const std::string_view name, engine_core& core) { core.add_function(name, default_ctor<ContainerType>()); }

		template<sequence_or_associative_container_t ContainerType>
		void register_assignable_container(const std::string_view name, engine_core& core)
		{
			core.add_function(name, copy_ctor<ContainerType>());
			operator_register::register_assign<ContainerType>(core);
		}

		template<sequence_or_associative_container_t ContainerType>
		void register_movable_container(const std::string_view name, engine_core& core)
		{
			core.add_function(name, move_ctor<ContainerType>());
			operator_register::register_move_assign<ContainerType>(core);
		}

		template<sequence_or_associative_container_t ContainerType>
		void register_basic_container(engine_core& core)
		{
			using container_type = ContainerType;

			// size/empty/clear
			core.add_function(
					lang::container_size_interface_name::value,
					fun(&container_type::size));
			core.add_function(
					lang::container_empty_interface_name::value,
					fun(&container_type::empty));
			core.add_function(
					lang::container_clear_interface_name::value,
					fun(&container_type::clear));
		}

		template<sequence_container_t ContainerType>
		void register_sequence_container(engine_core& core)
		{
			using container_type = ContainerType;

			// front/back
			core.add_function(
					lang::container_front_interface_name::value,
					fun(static_cast<typename container_type::reference (container_type::*)()>(&container_type::front)));
			core.add_function(
					lang::container_front_interface_name::value,
					fun(static_cast<typename container_type::const_reference (container_type::*)() const>(&container_type::front)));
			core.add_function(
					lang::container_back_interface_name::value,
					fun(static_cast<typename container_type::reference (container_type::*)()>(&container_type::back)));
			core.add_function(
					lang::container_back_interface_name::value,
					fun(static_cast<typename container_type::const_reference (container_type::*)() const>(&container_type::back)));
		}

		template<sequence_or_associative_container_t ContainerType>
		void register_iterable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// begin/end
			core.add_function(
					lang::container_begin_interface_name::value,
					fun(static_cast<typename container_type::iterator (container_type::*)()>(&container_type::begin)));
			core.add_function(
					lang::container_begin_interface_name::value,
					fun(static_cast<typename container_type::const_iterator (container_type::*)() const>(&container_type::begin)));
			core.add_function(
					lang::container_end_interface_name::value,
					fun(static_cast<typename container_type::iterator (container_type::*)()>(&container_type::end)));
			core.add_function(
					lang::container_end_interface_name::value,
					fun(static_cast<typename container_type::const_iterator (container_type::*)() const>(&container_type::end)));
		}

		template<sequence_container_t ContainerType>
		void register_index_access_container(engine_core& core)
		{
			using container_type = ContainerType;

			// operator[](size_type index)
			// note: In the interest of runtime safety for the m, we prefer the at() method for [] access,
			// to throw an exception in an out-of-bounds condition.
			core.add_function(
					lang::container_subscript_interface_name::value,
					fun(static_cast<typename container_type::reference (container_type::*)(typename container_type::size_type)>(&container_type::at)));
			core.add_function(
					lang::container_subscript_interface_name::value,
					fun(static_cast<typename container_type::const_reference (container_type::*)(typename container_type::size_type) const>(&container_type::at)));
		}

		template<associative_container_t ContainerType>
		void register_key_access_container(engine_core& core)
		{
			using container_type = ContainerType;

			// operator[](const key_type key)
			// todo: allow transparent search? how to implement it?
			core.add_function(
					lang::container_subscript_interface_name::value,
					fun(static_cast<typename container_type::mapped_type& (container_type::*)(const typename container_type::key_type&)>(&container_type::operator[])));
		}

		template<sequence_container_t ContainerType>
		void register_resizable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// resize
			core.add_function(
					lang::container_resize_interface_name::value,
					fun(static_cast<void (container_type::*)(typename container_type::size_type)>(&container_type::resize)));
			core.add_function(
					lang::container_resize_interface_name::value,
					fun(static_cast<void (container_type::*)(typename container_type::size_type, typename container_type::const_reference)>(&container_type::resize)));
		}

		template<sequence_container_t ContainerType>
		void register_reservable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// reserve/capacity
			core.add_function(
					lang::container_reserve_interface_name::value,
					fun(&container_type::reserve));
			core.add_function(
					lang::container_capacity_interface_name::value,
					fun(&container_type::capacity));
		}

		template<sequence_or_associative_container_t ContainerType>
		void register_modifiable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// insert/erase
			core.add_function(
					[]
					{
						if constexpr (std::is_same_v<typename container_type::value_type, boxed_value>) { return lang::container_insert_ref_interface_name::value; }
						else { return lang::container_insert_interface_name::value; }
					}(),
					fun(static_cast<typename container_type::iterator (container_type::*)(typename container_type::const_iterator, typename container_type::const_reference)>(&container_type::insert)));
			core.add_function(
					lang::container_erase_interface_name::value,
					fun(static_cast<typename container_type::iterator (container_type::*)(typename container_type::const_iterator)>(&container_type::erase)));
		}

		template<sequence_container_t ContainerType>
		void register_back_insertable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// push_back/pop_back
			core.add_function(
					[]
					{
						if constexpr (std::is_same_v<typename container_type::value_type, boxed_value>) { return lang::container_push_back_ref_interface_name::value; }
						else { return lang::container_push_back_interface_name::value; }
					}(),
					fun(static_cast<void (container_type::*)(typename container_type::const_reference)>(&container_type::push_back)));
			core.add_function(
					lang::container_pop_back_interface_name::value,
					fun(&container_type::pop_back));
		}

		template<sequence_container_t ContainerType>
		void register_front_insertable_container(engine_core& core)
		{
			using container_type = ContainerType;

			// push_front/pop_front
			core.add_function(
					[]
					{
						if constexpr (std::is_same_v<typename container_type::value_type, boxed_value>) { return lang::container_push_front_ref_interface_name::value; }
						else { return lang::container_push_front_interface_name::value; }
					},
					fun(static_cast<void (container_type::*)(typename container_type::const_reference)>(&container_type::push_front)));
			core.add_function(
					lang::container_pop_front_interface_name::value,
					fun(&container_type::pop_front));
		}

		template<typename ContainerType>
		void register_range_type(const string_view_type name, engine_core& core)
		{
			using container_type = ContainerType;

			auto range_register = [&core]<typename RangeType>(const string_view_type range_name)
			{
				using range_type = RangeType;

				core.add_type_info(range_name, make_type_info<range_type>());

				core.add_function(range_name, copy_ctor<range_type>());

				core.add_function(
						range_name,
						ctor<range_type(typename range_type::container_type&)>());

				core.add_function(
						lang::container_empty_interface_name::value,
						fun(&range_type::empty));
				core.add_function(
						lang::container_pop_front_interface_name::value,
						fun(&range_type::pop_front));
				core.add_function(
						lang::container_pop_back_interface_name::value,
						fun(&range_type::pop_back));
				core.add_function(
						lang::container_front_interface_name::value,
						fun(&range_type::front));
				core.add_function(
						lang::container_back_interface_name::value,
						fun(&range_type::back));
			};

			auto range_name = string_type{name} + lang::range_suffix_name::value;

			range_register.decltype(range_register)::template operator()<bidirectional_range<container_type>>(range_name);
			range_register.decltype(range_register)::template operator()<bidirectional_range<const container_type>>(lang::range_const_prefix_name::value + range_name);
		}

		template<typename ContainerType>
		void register_findable_container(engine_core& core)
		{
			using container_type = ContainerType;
			using key_type = container_key_type<ContainerType>;

			// find
			// todo: allow transparent search? how to implement it?
			core.add_function(
					lang::container_find_interface_name::value,
					fun(static_cast<typename container_type::iterator (container_type::*)(const key_type&)>(&container_type::find)));
			core.add_function(
					lang::container_find_interface_name::value,
					fun(static_cast<typename container_type::const_iterator (container_type::*)(const key_type&) const>(&container_type::find)));
		}
	}

	template<typename PairType>
	void register_pair_type(const string_view_type name, engine_core& core)
	{
		using pair_type = PairType;

		core.add_type_info(name, make_type_info<pair_type>());

		core.add_function(
				name,
				default_ctor<pair_type>());
		core.add_function(
				name,
				copy_ctor<pair_type>());
		core.add_function(
				name,
				ctor<pair_type(const typename pair_type::first_type&, const typename pair_type::second_type&)>());

		core.add_function(
				lang::pair_first_interface_name::value,
				fun(&pair_type::first));
		core.add_function(
				lang::pair_second_interface_name::value,
				fun(&pair_type::second));
	}

	namespace bootstrap_stl_detail
	{
		template<typename Container>
		void register_associative_container_pair_type(const string_view_type name, engine_core& core) { register_pair_type<typename Container::value_type>(string_type{name} + lang::pair_suffix_name::value, core); }
	}

	template<bootstrap_stl_detail::sequence_container_t VectorType>
	void register_vector_type(const string_view_type name, engine_core& core)
	{
		using vector_type = VectorType;
		using namespace bootstrap_stl_detail;

		core.add_type_info(name, make_type_info<vector_type>());

		// default constructible
		register_default_constructible_container<vector_type>(name, core);
		// assignable
		register_assignable_container<vector_type>(name, core);
		// moveable
		register_movable_container<vector_type>(name, core);

		// register size/empty/clear
		register_basic_container<vector_type>(core);

		// front/back
		register_sequence_container<vector_type>(core);

		// begin/end
		register_iterable_container<vector_type>(core);

		// operator[]
		register_index_access_container<vector_type>(core);

		// resize
		register_resizable_container<vector_type>(core);

		// reserve/capacity
		register_reservable_container<vector_type>(core);

		// insert/erase
		register_modifiable_container<vector_type>(core);

		// push_back/pop_back
		register_back_insertable_container<vector_type>(core);

		// range
		register_range_type<vector_type>(name, core);

		// todo: register vector::operator== ?
	}

	template<typename ListType>
	void register_list_type(const string_view_type name, engine_core& core)
	{
		using list_type = ListType;
		using namespace bootstrap_stl_detail;

		core.add_type_info(name, make_type_info<list_type>());

		// default constructible
		register_default_constructible_container<list_type>(name, core);
		// assignable
		register_assignable_container<list_type>(name, core);
		// moveable
		register_movable_container<list_type>(name, core);

		// register size/empty/clear
		register_basic_container<list_type>(core);

		// begin/end
		register_iterable_container<list_type>(core);

		// resize
		register_resizable_container<list_type>(core);

		// reserve/capacity
		register_reservable_container<list_type>(core);

		// insert/erase
		register_modifiable_container<list_type>(core);

		// push_back/pop_back
		register_back_insertable_container<list_type>(core);

		// push_front/pop_front
		register_front_insertable_container<list_type>(core);

		// range
		register_range_type<list_type>(name, core);

		// find
		register_findable_container<list_type>(core);

		// todo: register list::operator== ?
	}

	template<typename MapType>
	void register_map_type(const string_view_type name, engine_core& core)
	{
		using map_type = MapType;
		using namespace bootstrap_stl_detail;

		core.add_type_info(name, make_type_info<map_type>());

		// default constructible
		register_default_constructible_container<map_type>(name, core);
		// assignable
		register_assignable_container<map_type>(name, core);
		// moveable
		register_movable_container<map_type>(name, core);

		// register size/empty/clear
		register_basic_container<map_type>(core);

		// begin/end
		register_iterable_container<map_type>(core);

		// operator[]
		register_key_access_container<map_type>(core);

		// range
		register_range_type<map_type>(name, core);

		// find
		register_findable_container<map_type>(core);

		// map pair
		register_associative_container_pair_type<map_type>(name, core);

		// todo: register map::operator== ?
	}

	template<typename StringType>
	void register_string_type(const string_view_type name, engine_core& core)
	{
		using string_type = string_type;
		using namespace bootstrap_stl_detail;

		core.add_type_info(name, make_type_info<string_type>());

		// default constructible
		register_default_constructible_container<string_type>(name, core);
		// assignable
		register_assignable_container<string_type>(name, core);
		// moveable
		register_movable_container<string_type>(name, core);

		// register size/empty/clear
		register_basic_container<string_type>(core);

		// front/back
		register_sequence_container<string_type>(core);

		// begin/end
		register_iterable_container<string_type>(core);

		// operator[]
		register_index_access_container<string_type>(core);

		// resize
		register_resizable_container<string_type>(core);

		// reserve/capacity
		register_reservable_container<string_type>(core);

		// insert/erase
		register_modifiable_container<string_type>(core);

		// push_back/pop_back
		register_back_insertable_container<string_type>(core);

		// range
		register_range_type<string_type>(name, core);

		// find
		register_findable_container<string_type>(core);

		// operator+/operator+=
		operator_register::register_plus<string_type>(core);
		operator_register::register_plus_assign<string_type>(core);

		register_comparison<string_type>(core);

		// todo: more interface?
	}
}

#endif // GAL_LANG_FOUNDATION_BOOTSTRAP_STL_HPP
