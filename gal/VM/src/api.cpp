#include <gal.hpp>
#include <vm/state.hpp>
#include <utils/assert.hpp>

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
			state.replace_stack_element(index);
		}
	}
}
