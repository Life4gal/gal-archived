#include <gal.hpp>
#include <vm/state.hpp>
#include <utils/assert.hpp>
#include <vm/tagged_method.hpp>

namespace gal::vm
{
	namespace stack
	{
		[[nodiscard]] index_type abs_index(const child_state& state, const index_type index) noexcept
		{
			gal_assert(
					(index > 0 && std::cmp_less_equal(index, state.get_current_stack_size())) ||
					(index > 0 && std::cmp_less_equal(-index, state.get_current_stack_size())) ||
					is_pseudo(index));

			return index > 0 || is_pseudo(index) ? index : static_cast<index_type>(state.get_current_stack_size()) + index + 1;
		}

		[[nodiscard]] index_type get_top(const child_state& state) noexcept { return static_cast<index_type>(state.get_current_stack_size()); }

		void set_top(child_state& state, const index_type index) noexcept
		{
			if (index >= 0)
			{
				gal_assert(std::cmp_less_equal(index, state.get_total_stack_size()));
				state.fill_stack(static_cast<child_state::stack_slot_type>(index));
			}
			else
			{
				gal_assert(std::cmp_less_equal(-(index + 1), state.get_current_stack_size()));
				state.drop_stack(static_cast<child_state::stack_slot_type>(-(index + 1)));
			}
		}

		void push(child_state& state, const index_type index) noexcept
		{
			state.wake_me();

			state.push_into_stack(state.get_stack_element(index));
		}

		void remove(child_state& state, const index_type index) noexcept { state.remove_stack_element(index); }

		void insert(child_state& state, const index_type index) noexcept
		{
			state.wake_me();

			state.insert_stack_element(index);
		}

		void replace(child_state& state, const index_type index) noexcept
		{
			state.wake_me();

			state.replace_stack_element(index);
		}

		boolean_type check(child_state& state, const stack_size_type size) noexcept
		{
			if (size > max_internal_stack_size || size + state.get_current_stack_size() > max_internal_stack_size)
			{
				// stack overflow
				return false;
			}

			if (size > 0) { state.check_stack(size); }

			return true;
		}

		void raw_check(child_state& state, const stack_size_type size)
		{
			gal_assert(size > 0);
			state.check_stack(size);
		}

		void exchange_move(child_state& from, child_state& to, const stack_size_type num)
		{
			if (&from == &to) { return; }

			to.wake_me();

			from.move_stack_element(to, num);
		}

		void exchange_push(child_state& from, child_state& to, index_type index)
		{
			gal_assert(from.is_brother(to));

			to.wake_me();

			to.push_into_stack(from.get_stack_element(index));
		}
	}

	namespace internal
	{
		[[nodiscard]] boolean_type is_number(const child_state& state, const index_type index) { return state.get_stack_element(index).object_is_number(); }

		[[nodiscard]] boolean_type is_string(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			return v.is_number() || v.is_string();
		}

		[[nodiscard]] boolean_type is_internal_function(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			return v.is_function() && v.as_function()->is_internal();
		}

		[[nodiscard]] boolean_type is_gal_function(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			return v.is_function() && not v.as_function()->is_internal();
		}

		[[nodiscard]] boolean_type is_user_data(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			return v.is_user_data();
		}

		object_type get_type(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			if (v.is_null()) { return object_type::null; }
			if (v.is_boolean()) { return object_type::boolean; }
			if (v.is_number()) { return object_type::number; }
			if (v.is_string()) { return object_type::string; }
			if (v.is_table()) { return object_type::table; }
			if (v.is_function()) { return object_type::function; }
			if (v.is_user_data()) { return object_type::user_data; }
			if (v.is_thread()) { return object_type::thread; }
			return static_cast<object_type>(unknown_object_type);
		}

		[[nodiscard]] string_type get_typename(const child_state& state, const object_type type)
		{
			static_assert(std::is_unsigned_v<std::underlying_type_t<object_type>>);
			return
					static_cast<std::underlying_type_t<object_type>>(type) > std::size(gal_typename)
						? "UNKNOWN"
						: gal_typename[static_cast<std::underlying_type_t<object_type>>(type)].data();
		}

		[[nodiscard]] boolean_type is_equal(const child_state& state, const index_type index1, index_type index2)
		{
			const auto v1 = state.get_stack_element(index1);
			const auto v2 = state.get_stack_element(index2);
			return v1.equal(v2);
		}

		[[nodiscard]] boolean_type is_raw_equal(const child_state& state, const index_type index1, const index_type index2)
		{
			const auto v1 = state.get_stack_element(index1);
			const auto v2 = state.get_stack_element(index2);
			return v1.raw_equal(v2);
		}
	}
}
