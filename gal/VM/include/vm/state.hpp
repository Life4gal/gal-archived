#pragma once

#ifndef GAL_LANG_VM_STATE_HPP
#define GAL_LANG_VM_STATE_HPP

#include <vm/object.hpp>
#include <vm/tagged_method.hpp>
#include <utils/hash_container.hpp>
#include <utils/enum_utils.hpp>

namespace gal::vm
{
	enum class gc_current_state_type
	{
		pause = 0,
		propagate,
		propagate_again,
		atomic,
		sweep_string,
		sweep
	};

	struct gal_call_info
	{
		// base for this function
		stack_index_type base;
		// function index in the stack
		stack_index_type function;
		// top for this function
		stack_index_type top;

		compiler::operand_underlying_type saved_pc;

		// expected number of results from this function
		int num_returns;
		// call frame flags
		unsigned flags;
	};

	struct gc_cycle_state
	{
		std::size_t heap_goal_size_bytes = 0;
		std::size_t heap_trigger_size_bytes = 0;

		// time from end of the last cycle to the start of a new one
		double wait_time = 0;

		double begin_time_stamp = 0;
		double end_time_stamp = 0;

		double mark_time = 0;

		double atomic_begin_time_stamp = 0;
		std::size_t atomic_begin_total_size_bytes = 0;
		double atomic_time = 0;

		double sweep_time;

		std::size_t mark_items = 0;
		std::size_t sweep_items = 0;

		std::size_t assist_work = 0;
		std::size_t explicit_work = 0;

		std::size_t end_total_size_bytes = 0;
	};

	/**
	 * @brief Data for proportional-integral controller of heap trigger value
	 */
	struct gc_heap_trigger_state
	{
		constexpr static auto terminate_count = 32;
		std::int32_t terminates[terminate_count]{0};
		std::uint32_t terminate_pos = 0;
		std::int32_t integral = 0;
	};

	struct gc_state
	{
		double step_explicit_time_accumulate = 0;
		double step_assist_time_accumulate = 0;

		// when cycle is completed, last cycle values are updated
		std::uint64_t completed_cycles = 0;

		gc_cycle_state last_cycle;
		gc_cycle_state current_cycle;

		// only step count and their time is accumulated
		gc_cycle_state cycle_state_accumulate;

		gc_heap_trigger_state trigger_state;
	};

	/**
	 * @brief *global state* shared by all threads of this state
	 */
	struct global_state
	{
		// hash table for strings
		utils::hash_set<gal_string> string_table;

		// auxiliary data to allocator
		user_data_type user_data;

		object::mark_type current_white;
		// state of garbage collector
		gc_current_state_type gc_current_state;

		// position of sweep in string_table
		decltype(string_table)::size_type sweep_string_gc;
		// list of all collectable objects
		object* root_gc;
		// position of sweep in root_gc
		object** sweep_gc;
		// list of gray objects
		object* gray;
		// list of objects to be traversed atomically
		object* gray_again;
		// list of weak tables (to be cleared)
		object* weak;

		// list of all string buffer objects
		object* string_buffer_gc;

		// when total_bytes > gc_threshold; run GC step
		std::size_t gc_threshold;
		// number of bytes currently allocated
		std::size_t total_bytes;
		int gc_goal;
		int gc_step_multiple;
		int gc_step_size;

		// free page linked list for each size class
		std::array<struct memory_page*, size_classes> free_pages;

		thread_state* main_thread;
		// head of double-linked list of all open upvalues
		gal_upvalue upvalue_head;
		// meta tables for basic types
		std::array<gal_table*, static_cast<std::size_t>(object_type::tagged_value_count)> meta_tables;
		// names for basic types
		std::array<gal_string*, static_cast<std::size_t>(object_type::tagged_value_count)> type_names;
		// array with tag-method names
		std::array<gal_string*, static_cast<std::size_t>(tagged_method_type::tagged_method_count)> tagged_method_names;

		// registry table
		magic_value registry;
		// next free slot in registry
		index_type registry_free;

		// PCG random number generator state
		std::uint64_t random_generator_state;
		// pointer encoding key for display
		std::uint64_t pointer_encode_key[4];

		// for each user_data tag, a gc callback to be called immediately before freeing memory
		void (*user_data_gc[user_data_tag_limit])(user_data_type);

		coroutine::gal_callback callback;

		gc_state gc_states;

		/**
		 * @brief Tell when main invariant (white objects cannot point to black
		 * ones) must be kept. During a collection, the sweep phase may break
		 * the invariant, as objects turned white may point to still-black objects.
		 * The invariant is restored when sweep ends and all objects are white again.
		 */
		[[nodiscard]] constexpr bool keep_invariant() const noexcept
		{
			return utils::is_any_enum_of(
					gc_current_state,
					gc_current_state_type::propagate,
					gc_current_state_type::propagate_again,
					gc_current_state_type::atomic);
		}

		[[nodiscard]] constexpr object::mark_type another_white() const noexcept { return current_white ^ object::mark_white_bits_mask; }

		[[nodiscard]] constexpr object::mark_type get_white() const noexcept { return current_white & object::mark_white_bits_mask; }

		constexpr void make_white(object& obj) const noexcept { return obj.set_mark((obj.get_mark() & object::mask_marks) | get_white()); }

		[[nodiscard]] constexpr bool is_dead(const object& value) const noexcept
		{
			return
					(value.get_mark() & (object::mark_white_bits_mask | object::mark_fixed_bit_mask)) ==
					(another_white() & object::mark_white_bits_mask);
		}

		GAL_ASSERT_CONSTEXPR void check_alive([[maybe_unused]] const magic_value value) const noexcept { gal_assert(not value.is_object() || not is_dead(*value.as_object())); }

		constexpr object* exchange_gray(object& new_gray) noexcept { return std::exchange(gray, &new_gray); }

		void step(thread_state& state, bool assist);
		void full_gc(thread_state& state);

		void link_object(object& object)
		{
			object.link_next(root_gc);
			root_gc = &object;
			object.set_mark(get_white());
		}

		void link_upvalue(gal_upvalue& upvalue)
		{
			// link upvalue into root_gc list
			upvalue.link_next(root_gc);
			root_gc = &upvalue;

			if (upvalue.is_mark_gray())
			{
				if (keep_invariant())
				{
					// closed upvalues need barrier
					upvalue.set_mark_gray_to_black();
					barrier(upvalue, *upvalue.get_index());
				}
				else
				{
					// sweep phase: sweep it (turning it into white)
					make_white(upvalue);
					gal_assert(gc_current_state != gc_current_state_type::pause);
				}
			}
		}

		void barrier_upvalue(thread_state& state, object& value)
		{
			gal_assert(value.is_mark_white() && not is_dead(value));

			if (keep_invariant()) { value.mark(*this); }
		}

		void barrier_finalize(object& obj, object& value)
		{
			gal_assert(obj.is_mark_black() && value.is_mark_white());
			gal_assert(not is_dead(value) && not is_dead(obj));
			gal_assert(gc_current_state != gc_current_state_type::pause);

			// must keep invariant?
			if (keep_invariant())
			{
				// restore invariant
				value.mark(*this);
			}
			// don't mind
			else
			{
				// mark as white just to avoid other barriers
				make_white(obj);
			}
		}

		void barrier_back(gal_table& table)
		{
			gal_assert(table.is_mark_black() && not is_dead(table));
			gal_assert(gc_current_state != gc_current_state_type::pause);

			// make table gray (again)
			table.set_mark_black_to_gray();
			table.set_gc_list(gray_again);
			gray_again = &table;
		}

		void validate();

		void check_gc(thread_state& state) { if (total_bytes >= gc_threshold) { step(state, true); } }

		void barrier(object& obj, const magic_value value) { if (value.is_object() && obj.is_mark_black() && value.as_object()->is_mark_white()) { barrier_finalize(obj, *value.as_object()); } }

		void barrier(object& obj, object& value) { if (obj.is_mark_black() && value.is_mark_white()) { barrier_finalize(obj, value); } }

		void barrier_fast(const magic_value table) { if (table.as_table()->is_mark_black()) { barrier_back(*table.as_table()); } }

		void barrier_table(gal_table& table, const magic_value value) { if (value.is_object()) { barrier_table(table, *value.as_object()); } }

		void barrier_table(gal_table& table, object& value)
		{
			if (table.is_mark_black() && value.is_mark_white())
			{
				// in the second propagation stage, table assignment barrier works as a forward barrier
				if (gc_current_state == gc_current_state_type::propagate_again)
				{
					gal_assert(table.is_mark_black() && value.is_mark_white());
					gal_assert(not is_dead(value) && not is_dead(table));

					value.mark(*this);
				}
				else
				{
					gal_assert(table.is_mark_black() && not is_dead(table));
					gal_assert(gc_current_state != gc_current_state_type::pause);

					// make table gray (again)
					table.set_gc_list(gray_again);
					gray_again = &table;
				}
			}
		}
	};

	GAL_ASSERT_CONSTEXPR void object::mark(global_state& state)
	{
		gal_assert(is_mark_white() && not state.is_dead(*this));
		set_mark_white_to_gray();
		do_mark(state);
	}

	constexpr void magic_value::copy_magic_value(const global_state& state, const magic_value target) noexcept
	{
		data_ = target.data_;
		state.check_alive(*this);
	}

	constexpr void gal_prototype::do_mark(global_state& state) { gc_list_ = state.exchange_gray(*this); }

	constexpr void gal_closure::do_mark(global_state& state) { gc_list_ = state.exchange_gray(*this); }

	constexpr void gal_table::do_mark(global_state& state) { gc_list_ = state.exchange_gray(*this); }

	class thread_state final : public object
	{
	public:
		// Thread stack states
		// thread is currently active
		constexpr static std::uint8_t thread_active_bit = 0;
		// thread is not executing and stack should not be modified
		constexpr static std::uint8_t thread_sleeping_bit = 1;

		constexpr static std::uint8_t thread_active_bit_mask = 1 << thread_active_bit;
		constexpr static std::uint8_t thread_sleeping_bit_mask = 1 << thread_sleeping_bit;

		constexpr static auto basic_call_info_size = 8;
		constexpr static auto basic_stack_size = 2 * min_stack_size;
		constexpr static auto extra_stack_size = 5;

	private:
		std::uint8_t status_;
		std::uint8_t stack_state_;

		// call debug_step hook after each instruction
		bool single_step_;

		global_state& global_;

		// stack base
		std::array<magic_value, basic_stack_size + extra_stack_size> stack_;
		// first free slot in the stack
		std::array<magic_value, basic_stack_size + extra_stack_size>::size_type top_;
		// base of current function
		std::array<magic_value, basic_stack_size + extra_stack_size>::size_type base_;

		// array of call_info's
		std::array<gal_call_info, basic_call_info_size> base_call_info_;
		// call info for current function
		std::array<gal_call_info, basic_call_info_size>::size_type current_call_info_;

		// number of nested internal calls
		uint16_t num_internal_calls_;
		// nested internal calls when resuming coroutine
		uint16_t base_internal_calls_;

		// when table operations or INDEX/NEW_INDEX is invoked from GAL, what is the expected slot for lookup?
		int cached_slot_;

		// table of globals
		magic_value global_table_;
		// temporary place for environments
		magic_value environment_;
		// list of open upvalues in this stack 
		object* open_upvalue_;
		object* gc_list_;

		// when invoked from GAL using NAMED_CALL, what method do we need to invoke?
		gal_string* named_call_;

		user_data_type user_data_;

		void do_mark(global_state& state) override { gc_list_ = state.exchange_gray(*this); }

		void new_stack();
		void destroy_stack();
		void close_stack(stack_index_type level);

		static void destroy_list(object** begin, object* end);

	public:
		explicit thread_state(global_state& global);

		// todo: interface
		void destroy(thread_state& state) override;

		/**
		 * @brief Create a new child thread
		 */
		thread_state* new_thread();

		/**
		 * @brief Destroy a child thread
		 */
		void destroy_thread(thread_state& child);

		[[nodiscard]] constexpr bool is_thread_active() const noexcept { return stack_state_ & thread_active_bit_mask; }

		[[nodiscard]] constexpr bool is_thread_sleeping() const noexcept { return stack_state_ & thread_sleeping_bit_mask; }

		constexpr void wake_thread()
		{
			if (not is_thread_sleeping()) { return; }

			stack_state_ &= ~thread_sleeping_bit_mask;

			if (global_.keep_invariant())
			{
				gc_list_ = std::exchange(global_.gray_again, this);

				set_mark_black_to_gray();
			}
		}

		void close_upvalue() { open_upvalue_->delete_chain(*this, nullptr); }

		[[nodiscard]] constexpr auto* get_stack_last() noexcept { return &stack_[basic_stack_size - 1]; }

		[[nodiscard]] constexpr auto* get_stack_last() const noexcept { return &stack_[basic_stack_size - 1]; }
	};

	constexpr void object::delete_chain(thread_state& state, object* end)
	{
		auto* current = this;
		auto* next = current->next_;

		while (current != end)
		{
			if (current->type() == object_type::thread)
			{
				// delete open upvalues of each thread
				dynamic_cast<thread_state*>(current)->close_upvalue();
			}

			current->destroy(state);
			current = next;
			next = current->next_;
		}
	}

	inline thread_state* magic_value::as_thread() const noexcept
	{
		gal_assert(is_thread());
		return dynamic_cast<thread_state*>(as_object());
	}
}

#endif // GAL_LANG_VM_STATE_HPP
