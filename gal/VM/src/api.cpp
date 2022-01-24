#include <gal.hpp>
#include <vm/state.hpp>
#include <utils/assert.hpp>
#include <vm/meta_method.hpp>

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

			if (size > 0)
			{
				state.check_stack(size);
				state.expand_stack_limit(size);
			}

			return true;
		}

		void raw_check(child_state& state, const stack_size_type size)
		{
			gal_assert(size > 0);
			state.check_stack(size);
			state.expand_stack_limit(size);
		}

		void exchange_move(child_state& from, child_state& to, const stack_size_type num)
		{
			if (&from == &to) { return; }

			to.wake_me();

			from.move_stack_element(to, num);
		}

		void exchange_push(const child_state& from, child_state& to, const index_type index)
		{
			gal_assert(from.is_brother(to));

			to.wake_me();

			to.push_into_stack(from.get_stack_element(index));
		}
	}

	namespace internal
	{
		[[nodiscard]] boolean_type is_number(const child_state& state, const index_type index) noexcept { return state.get_stack_element(index).number_convertible(); }

		[[nodiscard]] boolean_type is_string(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_number() || v.is_string();
		}

		[[nodiscard]] boolean_type is_internal_function(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_function() && v.as_function()->is_internal();
		}

		[[nodiscard]] boolean_type is_gal_function(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_function() && not v.as_function()->is_internal();
		}

		[[nodiscard]] boolean_type is_user_data(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_user_data();
		}

		object_type get_type(const child_state& state, const index_type index) noexcept { return state.get_stack_element(index).get_type(); }

		[[nodiscard]] string_type get_typename(const object_type type) noexcept
		{
			static_assert(std::is_unsigned_v<std::underlying_type_t<object_type>>);
			return
					static_cast<std::underlying_type_t<object_type>>(type) > std::size(gal_typename)
						? "UNKNOWN"
						: gal_typename[static_cast<std::underlying_type_t<object_type>>(type)].data();
		}

		[[nodiscard]] unsigned_type get_object_length(const child_state& state, const index_type index)
		{
			const auto v = state.get_stack_element(index);
			if (v.is_number())
			{
				const auto string = v.to_string(state.get_parent());
				return string ? static_cast<unsigned_type>(string->size()) : 0;
			}
			if (v.is_string()) { return static_cast<unsigned_type>(v.as_string()->size()); }
			if (v.is_user_data()) { return static_cast<unsigned_type>(v.as_user_data()->size()); }
			if (v.is_table()) { return static_cast<unsigned_type>(v.as_table()->size()); }
			return 0;
		}

		[[nodiscard]] boolean_type is_equal(child_state& state, const index_type index1, const index_type index2)
		{
			const auto v1 = state.get_stack_element(index1);
			const auto v2 = state.get_stack_element(index2);
			return v1.equal(state, v2);
		}

		[[nodiscard]] boolean_type is_raw_equal(const child_state& state, const index_type index1, const index_type index2) noexcept
		{
			const auto v1 = state.get_stack_element(index1);
			const auto v2 = state.get_stack_element(index2);
			return v1.raw_equal(v2);
		}

		[[nodiscard]] boolean_type to_boolean(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.as_boolean();
		}

		[[nodiscard]] number_type to_number(const child_state& state, const index_type index, boolean_type* converted) noexcept
		{
			const auto v = state.get_stack_element(index);
			if (const auto result = v.to_number();
				magic_value{result} != magic_value_null)
			{
				if (converted) { *converted = true; }
				return result;
			}
			else
			{
				if (converted) { *converted = false; }
				return 0;
			}
		}

		[[nodiscard]] string_type to_string(child_state& state, const index_type index, size_t* length)
		{
			const auto v = state.get_stack_element(index);

			object_string* string;

			if (v.is_string()) { string = v.as_string(); }
			else
			{
				state.wake_me();

				string = v.to_string(state.get_parent());
				// conversion failed?
				if (not string)
				{
					if (length) { *length = 0; }
					return nullptr;
				}
			}

			if (length) { *length = string->size(); }
			return string->get_raw_data();
		}

		[[nodiscard]] string_type to_string_atomic(const child_state& state, const index_type index, int* atomic) noexcept
		{
			const auto v = state.get_stack_element(index);

			if (not v.is_string()) { return nullptr; }

			const auto* string = v.as_string();
			if (atomic) { *atomic = string->get_atomic(); }
			return string->get_raw_data();
		}

		[[nodiscard]] string_type to_named_call_atomic(const child_state& state, int* atomic) noexcept
		{
			const auto* call = state.get_named_call();
			if (not call) { return nullptr; }

			if (atomic) { *atomic = call->get_atomic(); }
			return call->get_raw_data();
		}

		[[nodiscard]] internal_function_type to_internal_function(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_function() ? v.as_function()->get_internal_function() : nullptr;
		}

		[[nodiscard]] child_state* to_thread(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_thread() ? v.as_thread() : nullptr;
		}

		GAL_API [[nodiscard]] const void* to_pointer(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			if (v.is_function()) { return v.as_function(); }
			if (v.is_user_data()) { return to_user_data(state, index); }
			if (v.is_table()) { return v.as_table(); }
			if (v.is_user_data()) { return v.as_user_data(); }
			return nullptr;
		}

		[[nodiscard]] user_data_type to_user_data(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_user_data() ? v.as_user_data()->get_data() : nullptr;
		}

		[[nodiscard]] user_data_type to_user_data_tagged(const child_state& state, const index_type index, const user_data_tag_type tag) noexcept
		{
			if (const auto v = state.get_stack_element(index);
				v.is_user_data()) { if (auto* data = v.as_user_data(); data->get_tag() == tag) { return data; } }
			return nullptr;
		}

		[[nodiscard]] user_data_tag_type get_user_data_tag(const child_state& state, const index_type index) noexcept
		{
			const auto v = state.get_stack_element(index);
			return v.is_user_data() ? v.as_user_data()->get_tag() : user_data_tag_invalid;
		}

		void push_null(child_state& state) noexcept { state.push_into_stack_no_check(magic_value_null); }

		void push_boolean(child_state& state, const boolean_type boolean) noexcept { state.push_into_stack_no_check(boolean ? magic_value_true : magic_value_false); }

		void push_number(child_state& state, const number_type number) noexcept { state.push_into_stack(magic_value{number}); }

		void push_string_sized(child_state& state, string_type string, size_t length)
		{
			state.get_parent().check_gc();
			state.wake_me();

			state.push_into_stack(
					CREATE_OBJECT(
							object_string,
							state.get_parent(),
							state.get_parent(),
							string,
							length)->operator magic_value());
		}

		void push_string(child_state& state, const string_type string)
		{
			if (string) { push_string_sized(state, string, std::strlen(string)); }
			else { push_null(state); }
		}

		void push_internal_closure(child_state& state, stack_size_type num_params, internal_function_type function, continuation_function_type continuation, string_type debug_name)
		{
			state.get_parent().check_gc();
			state.wake_me();
			gal_assert(state.is_stack_enough(num_params));

			auto* closure = CREATE_OBJECT(
					object_closure,
					state.get_parent(),
					state.get_parent(),
					num_params,
					state.get_current_environment(),
					function,
					continuation,
					debug_name)			;

			for (index_type i = 1; i < num_params + 1; ++i)
			{
				const auto upvalue = state.peek_stack_element(i);
				state.get_parent().check_alive(upvalue);
				closure->push_upvalue(upvalue);
			}

			gal_assert(closure->is_mark_white());
			state.push_into_stack(closure->operator magic_value());
		}

		boolean_type push_thread(child_state& state) noexcept
		{
			state.wake_me();
			state.push_into_stack(state.operator magic_value());
			return state.is_oldest_child();
		}
	}

	namespace interface
	{
		void get_table(child_state& state, const index_type index)
		{
			state.wake_me();

			const auto v = state.get_stack_element(index);
			gal_assert(v != magic_value_null);

			// todo
		}
	}
}
