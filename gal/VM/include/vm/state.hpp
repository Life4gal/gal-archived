#pragma once

#ifndef GAL_LANG_VM_STATE_HPP
#define GAL_LANG_VM_STATE_HPP

#include <vm/object.hpp>
#include <vm/tagged_method.hpp>
#include <utils/hash_container.hpp>
#include <utils/enum_utils.hpp>
#include <array>

namespace gal::vm
{
	struct gc_handler
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

		struct gc_cycle_state
		{
			std::size_t heap_goal_size_bytes = 0;
			std::size_t heap_trigger_size_bytes = 0;

			// time from end of the last cycle to the start of a new one
			double wait_time = 0;

			std::uint64_t begin_time_stamp = 0;
			std::uint64_t end_time_stamp = 0;

			double mark_time = 0;

			std::uint64_t atomic_begin_time_stamp = 0;
			std::size_t atomic_begin_total_size_bytes = 0;
			double atomic_time = 0;

			double sweep_time = 0;

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
			using term_type = std::int32_t;

			constexpr static auto term_count = 32;
			term_type terms[term_count]{0};
			std::uint32_t term_pos = 0;
			term_type integral = 0;
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

		using user_data_gc_handler = void(*)(user_data_type);
		using user_data_gc_handler_container_type = std::array<user_data_gc_handler, user_data_tag_limit>;

		constexpr static std::size_t sweep_max_count = 40;
		constexpr static std::size_t sweep_cost = 10;

		// state of garbage collector
		gc_current_state_type gc_current_state = gc_current_state_type::pause;

		user_data_gc_handler_container_type user_data_gc_handlers{};

		// position of sweep in main_thread::string_table
		std::size_t sweep_string_gc = 0;

		// list of all collectable objects
		object* root_gc;
		// position of sweep in root_gc
		object* sweep_gc;

		// list of gray objects
		object* gray = nullptr;
		// list of objects to be traversed atomically
		object* gray_again = nullptr;
		// list of weak tables (to be cleared)
		object* weak = nullptr;

		// list of all string buffer objects
		object* string_buffer_gc = nullptr;

		// when total_bytes > gc_threshold; run GC step
		// default init as unfinished state
		std::size_t gc_threshold = 0;
		// number of bytes currently allocated
		std::size_t total_bytes;

		int gc_goal = default_gc_goal;
		int gc_step_multiple = default_gc_step_multiple;
		int gc_step_size = default_gc_step_size << 10;

		// free page linked list for each size class
		std::array<struct memory_page*, size_classes> free_pages{};

		gc_state gc_states;

	private:
		bool traverse_table(main_state& state, object_table& table);

		std::size_t propagate_mark(main_state& state);

		std::size_t propagate_all(main_state& state)
		{
			std::size_t work = 0;
			while (gray) { work += propagate_mark(state); }
			return work;
		}

		std::size_t atomic(child_state& state);

		object* sweep_list(main_state& state, object* begin, std::size_t* traversed_count = nullptr, std::size_t count = std::numeric_limits<std::size_t>::max());

		std::size_t step(main_state& state, std::size_t limit = std::numeric_limits<std::size_t>::max());

		void begin_gc_cycle();

		void end_gc_cycle();

		void record_state_time(gc_current_state_type current_state, double second, bool assist);

		std::size_t get_heap_trigger(std::size_t heap_goal);

		GAL_ASSERT_CONSTEXPR void validate_gray_list(const object& list) const;

	public:
		explicit gc_handler(object* root);

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

		[[nodiscard]] constexpr bool running() const noexcept { return gc_current_state != gc_current_state_type::pause; }

		[[nodiscard]] constexpr object* exchange_gray(object& new_gray) noexcept { return std::exchange(gray, &new_gray); }

		[[nodiscard]] constexpr object* exchange_gray_again(object& new_gray) noexcept { return std::exchange(gray_again, &new_gray); }

		[[nodiscard]] constexpr object* exchange_weak(object& new_weak) noexcept { return std::exchange(weak, &new_weak); }

		constexpr void link_object(object& object) noexcept
		{
			object.link_next(root_gc);
			root_gc = &object;
		}

		void step(child_state& state, bool assist);

		void full_gc(main_state& state);

		void validate(main_state& state) const;
	};

	struct call_info
	{
		// base for this function
		stack_element_type base;
		// function index in the stack
		stack_element_type function;
		// top for this function
		stack_element_type top;

		compiler::debug_pc_type saved_pc;

		// expected number of results from this function
		compiler::operand_abc_underlying_type num_returns;
		// call frame flags
		unsigned flags;
	};

	class child_state final : public object
	{
		friend struct gc_handler;
		friend class main_state;

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

		using stack_type = std::array<magic_value, basic_stack_size + extra_stack_size>;
		using stack_slot_type = stack_type::size_type;

		using call_info_container_type = std::array<call_info, basic_call_info_size>;
		using call_info_slot_type = call_info_container_type::size_type;

	private:
		main_state& parent_;

		std::uint8_t status_;
		std::uint8_t stack_state_;

		// call debug_step hook after each instruction
		bool single_step_;

		// stack base
		stack_type stack_;
		// first free slot in the stack
		stack_slot_type top_;
		// base of current function
		stack_slot_type base_;

		// array of call_info's
		call_info_container_type call_infos_;
		// call info for current function
		call_info_slot_type current_call_info_;

		// number of nested internal calls
		std::uint16_t num_internal_calls_;
		// nested internal calls when resuming coroutine
		std::uint16_t base_internal_calls_;

		// when table operations or INDEX/NEW_INDEX is invoked from GAL, what is the expected slot for lookup?
		index_type cached_slot_;

		// table of globals
		magic_value global_table_;
		// temporary place for environments
		magic_value environment_;
		// list of open upvalues in this stack
		object* open_upvalue_;
		object* gc_list_;

		// when invoked from GAL using NAMED_CALL, what method do we need to invoke?
		object_string* named_call_;

		user_data_type user_data_;

		void do_mark(main_state& state) override
		{
			// todo
		}

		void do_destroy(main_state& state) override;

	public:
		explicit child_state(main_state& parent)
			: object{object_type::thread},
			  parent_{parent},
			  status_{0},
			  stack_state_{0},
			  single_step_{false},
			  // function entry for this call info
			  top_{1},
			  base_{1},
			  call_infos_{},
			  current_call_info_{0},
			  num_internal_calls_{0},
			  base_internal_calls_{0},
			  cached_slot_{0},
			  global_table_(create<object_table>(parent, parent)->operator magic_value()),
			  open_upvalue_{nullptr},
			  gc_list_{nullptr},
			  named_call_{nullptr},
			  user_data_{nullptr}
		{
			// todo
			// initialize first call info
			call_infos_[0] = {
					.base = &stack_[0],
					.function = &stack_[0],
					.top = &stack_[min_stack_size],
					.saved_pc = 0,
					.num_returns = 0,
					.flags = 0};
		}

		[[nodiscard]] constexpr std::size_t memory_usage() const noexcept override { return sizeof(child_state); }

		void traverse(main_state& state, bool clear_stack);

		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }

		[[nodiscard]] constexpr object* get_gc_list() noexcept { return gc_list_; }

		[[nodiscard]] constexpr const object* get_gc_list() const noexcept { return gc_list_; }

		[[nodiscard]] constexpr bool is_thread_active() const noexcept { return stack_state_ & thread_active_bit_mask; }

		[[nodiscard]] constexpr bool is_thread_sleeping() const noexcept { return stack_state_ & thread_sleeping_bit_mask; }

		constexpr void make_stack_wake() noexcept { stack_state_ &= ~thread_sleeping_bit_mask; }

		constexpr void make_stack_sleep() noexcept { stack_state_ |= thread_sleeping_bit_mask; }

		/**
		 * @brief Close all upvalues for this thread
		 */
		void close_upvalue()
		{
			gal_assert(dynamic_cast<object_upvalue*>(open_upvalue_));
			open_upvalue_ = dynamic_cast<object_upvalue*>(open_upvalue_)->close_until(parent_, stack_.data());
		}

		[[nodiscard]] constexpr auto* get_stack_last() noexcept { return &stack_[basic_stack_size - 1]; }

		[[nodiscard]] constexpr auto* get_stack_last() const noexcept { return &stack_[basic_stack_size - 1]; }
	};

	constexpr void object::delete_chain(main_state& state, object* end)
	{
		auto* current = this;
		auto* next = current->next_;

		while (current != end)
		{
			if (current->type() == object_type::thread)
			{
				// delete open upvalues of each thread
				dynamic_cast<child_state*>(current)->close_upvalue();
			}

			destroy(state, current);
			current = next;
			next = current->next_;
		}
	}

	class main_state final
	{
		friend struct gc_handler;
		friend struct raw_memory;
		friend struct memory_page;

	public:
		// todo: string pool?
		using string_table_type = std::vector<object_string*>;

		using builtin_type_meta_table_type = std::array<object_table*, static_cast<std::size_t>(object_type::tagged_value_count)>;
		using builtin_type_name_table_type = std::array<object_string*, static_cast<std::size_t>(object_type::tagged_value_count)>;
		using tagged_method_name_table_type = std::array<object_string*, static_cast<std::size_t>(tagged_method_type::tagged_method_count)>;

	private:
		string_table_type string_table_;

		object::mark_type current_white_;

		child_state main_thread_;

		// head of double-linked list of all open upvalues
		object_upvalue upvalue_head_;

		// meta tables for basic types
		builtin_type_meta_table_type meta_table_{};
		// names for basic types
		builtin_type_name_table_type type_name_{};
		// names for tagged method
		tagged_method_name_table_type tagged_method_name_{};

		// registry table
		magic_value registry_;
		// next free slot in registry
		index_type registry_free_;

		gc_handler gc_;

		debug::callback_info callback_;

		void mark_meta_table();

		/**
		 * @brief Mark root set
		 */
		void mark_root()
		{
			gc_.gray = nullptr;
			gc_.gray_again = nullptr;
			gc_.weak = nullptr;

			main_thread_.try_mark(*this);

			// make global table be traversed before main stack
			main_thread_.global_table_.mark(*this);
			registry_.mark(*this);

			mark_meta_table();

			gc_.gc_current_state = gc_handler::gc_current_state_type::propagate;
		}

	public:
		main_state();

		main_state(const main_state&) = delete;
		main_state& operator=(const main_state&) = delete;
		main_state(main_state&&) = delete;
		main_state& operator=(main_state&&) = delete;

		~main_state() noexcept;

		[[nodiscard]] constexpr object::mark_type another_white() const noexcept { return current_white_ ^ object::mark_white_bits_mask; }

		[[nodiscard]] constexpr object::mark_type get_white() const noexcept { return current_white_ & object::mark_white_bits_mask; }

		constexpr void make_white(object& obj) const noexcept { return obj.set_mark((obj.get_mark() & object::mask_marks) | get_white()); }

		constexpr void flip_white() noexcept { current_white_ = another_white(); }

		[[nodiscard]] constexpr bool check_is_dead(const object& value) const noexcept
		{
			return (value.get_mark() & (object::mark_white_bits_mask | object::mark_fixed_bit_mask)) ==
			       (another_white() & object::mark_white_bits_mask);
		}

		GAL_ASSERT_CONSTEXPR void check_alive([[maybe_unused]] const magic_value value) const noexcept { gal_assert(not value.is_object() || not check_is_dead(*value.as_object())); }

		GAL_ASSERT_CONSTEXPR std::size_t remark_upvalues() { return upvalue_head_.remark(*this); }

		[[nodiscard]] constexpr gc_handler::gc_current_state_type get_gc_state() const noexcept { return gc_.gc_current_state; }

		constexpr void wake_child(child_state& child)
		{
			if (not child.is_thread_sleeping()) { return; }

			child.make_stack_wake();

			if (gc_.keep_invariant())
			{
				child.set_gc_list(gc_.exchange_gray_again(child));
				child.set_mark_black_to_gray();
			}
		}

		[[nodiscard]] const object_string* get_table_mode(const object_table& table) const
		{
			return
					table.has_meta_table()
						? not table.check_flag(tagged_method_type::mode)
							  ? tagged_method_name_[static_cast<std::size_t>(tagged_method_type::mode)]
							  : nullptr
						: nullptr;
		}

		debug::callback_info& get_callback_info() noexcept { return callback_; }

		void add_string_into_table(object_string& string);

		void remove_string_from_table(object_string& string);

		GAL_ASSERT_CONSTEXPR void barrier_upvalue(object& value)
		{
			gal_assert(value.is_mark_white() && not check_is_dead(value));

			if (gc_.keep_invariant()) { value.mark(*this); }
		}

		void barrier_finalize(object& obj, object& value)
		{
			gal_assert(obj.is_mark_black() && value.is_mark_white());
			gal_assert(not check_is_dead(value) && not check_is_dead(obj));
			gal_assert(gc_.running());

			// must keep invariant?
			if (gc_.keep_invariant())
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

		void barrier_back(object_table& table)
		{
			gal_assert(table.is_mark_black() && not check_is_dead(table));
			gal_assert(gc_.running());

			// make table gray (again)
			table.set_mark_black_to_gray();
			table.set_gc_list(gc_.exchange_gray_again(table));
		}

		void barrier(object& obj, const magic_value value) { if (value.is_object() && obj.is_mark_black() && value.as_object()->is_mark_white()) { barrier_finalize(obj, *value.as_object()); } }

		void barrier(object& obj, object& value) { if (obj.is_mark_black() && value.is_mark_white()) { barrier_finalize(obj, value); } }

		void barrier_fast(const magic_value table) { if (table.as_table()->is_mark_black()) { barrier_back(*table.as_table()); } }

		void barrier_table(object_table& table, const magic_value value) { if (value.is_object()) { barrier_table(table, *value.as_object()); } }

		void barrier_table(object_table& table, object& value)
		{
			if (table.is_mark_black() && value.is_mark_white())
			{
				// in the second propagation stage, table assignment barrier works as a forward barrier
				if (gc_.gc_current_state == gc_handler::gc_current_state_type::propagate_again)
				{
					gal_assert(table.is_mark_black() && value.is_mark_white());
					gal_assert(not check_is_dead(value) && not check_is_dead(table));

					value.mark(*this);
				}
				else
				{
					gal_assert(table.is_mark_black() && not check_is_dead(table));
					gal_assert(gc_.running());

					// make table gray (again)
					table.set_gc_list(gc_.exchange_gray_again(table));
				}
			}
		}

		constexpr void link_object(object& object) noexcept
		{
			gc_.link_object(object);
			object.set_mark(get_white());
		}

		void link_upvalue(object_upvalue& upvalue)
		{
			gc_.link_object(upvalue);

			if (upvalue.is_mark_gray())
			{
				if (gc_.keep_invariant())
				{
					// closed upvalues need barrier
					upvalue.set_mark_gray_to_black();
					barrier(upvalue, *upvalue.get_index());
				}
				else
				{
					// sweep phase: sweep it (turning it into white)
					make_white(upvalue);
					gal_assert(gc_.running());
				}
			}
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR gc_handler::user_data_gc_handler get_user_data_gc_handler(const user_data_tag_type tag) const noexcept
		{
			gal_assert(tag < user_data_tag_limit);

			return gc_.user_data_gc_handlers[tag];
		}
	};

	GAL_ASSERT_CONSTEXPR void object::mark(main_state& state)
	{
		gal_assert(is_mark_white() && not state.check_is_dead(*this));
		set_mark_white_to_gray();
		do_mark(state);
	}
}

#endif // GAL_LANG_VM_STATE_HPP
