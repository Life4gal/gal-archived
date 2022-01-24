#pragma once

#ifndef GAL_LANG_VM_STATE_HPP
#define GAL_LANG_VM_STATE_HPP

#include <vm/object.hpp>
#include <vm/meta_method.hpp>
#include <utils/enum_utils.hpp>
#include <array>
#include <vector>
#include <utils/macro.hpp>

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
		std::array<struct memory_page*, size_classes> free_pages;

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

		GAL_ASSERT_CONSTEXPR void link_object(object& object) noexcept
		{
			object.link_next(root_gc);
			root_gc = &object;
		}

		void step(child_state& state, bool assist);

		void check(child_state& state) { if (total_bytes >= gc_threshold) { step(state, true); } }

		void full_gc(main_state& state);

		void validate(main_state& state) const;
	};

	struct call_info
	{
		using flag_type = std::uint32_t;

		/**
		 * @brief Should the interpreter return after returning from this call_info? first frame must have this set.
		 */
		constexpr static flag_type flag_return = 1 << 0;

		/**
		 * @brief Should the error thrown during execution get handled by continuation from this call_info? function must be internal type.
		 */
		constexpr static flag_type flag_handle = 1 << 1;

		// base for this function
		stack_element_type base;
		// function index in the stack
		stack_element_type function;
		// top for this function
		stack_element_type top;

		object_prototype::code_pc_type saved_pc;

		// expected number of results from this function
		compiler::operand_abc_underlying_type num_returns;
		// call frame flags
		flag_type flags;
	};

	class child_state final : public object
	{
		friend class object;
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

		using stack_type = std::vector<magic_value, vm_allocator<magic_value>>;
		using stack_slot_type = stack_type::size_type;

		using call_info_container_type = std::vector<call_info, vm_allocator<call_info>>;
		using call_info_slot_type = call_info_container_type::size_type;

	private:
		main_state& parent_;

		thread_status status_;
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
			(void)state;
		}

		void do_destroy(main_state& state) override;

		GAL_ASSERT_CONSTEXPR void grow_stack(const stack_size_type needed) noexcept
		{
			if (std::cmp_greater(get_stack_last_pos() - top_, needed)) { return; }

			const auto needed_stack_size =
					// double size is enough?
					(std::cmp_less_equal(needed, stack_.capacity())
						 ? 2 * stack_.capacity()
						 : needed + stack_.capacity()) + 1 + extra_stack_size;

			stack_type new_stack{needed_stack_size, stack_.get_allocator()};

			// correct stack
			for (auto* upvalue = open_upvalue_; upvalue; upvalue = upvalue->get_next())
			{
				gal_assert(dynamic_cast<object_upvalue*>(upvalue));
				auto* u = dynamic_cast<object_upvalue*>(upvalue);
				u->redirect_stack_index(new_stack.data() + (u->get_index() - stack_.data()));
			}
			for (decltype(current_call_info_) i = 0; i <= current_call_info_; ++i)
			{
				auto& [base, function, top
							, _dummy1, _dummy2, _dummy3]
						= call_infos_[i];

				base = new_stack.data() + (base - stack_.data());
				function = new_stack.data() + (base - stack_.data());
				top = new_stack.data() + (base - stack_.data());
			}

			// exchange stack
			stack_.swap(new_stack);
		}

		void grow_call_infos();

		constexpr void clear_stack() noexcept
		{
			stack_type{{parent_}}.swap(stack_);
			call_info_container_type{{parent_}}.swap(call_infos_);
		}

	public:
		explicit child_state(main_state& parent);

		explicit child_state(child_state& brother);

		child_state& operator=(const child_state&) = delete;
		child_state(child_state&&) = delete;
		child_state& operator=(child_state&&) = delete;

		~child_state() override = default;

		[[nodiscard]] constexpr std::size_t memory_usage() const noexcept override { return sizeof(child_state); }

		void reset();

		[[nodiscard]] constexpr bool is_reset() const noexcept { return current_call_info_ == 0 && base_ == top_ && base_ == 0 && status_ == thread_status::ok; }

		void traverse(main_state& state, bool clear_stack);

		[[nodiscard]] constexpr bool is_brother(const child_state& another) const noexcept { return global_table_ == another.global_table_; }

		[[nodiscard]] constexpr bool is_oldest_child() const noexcept;

		[[nodiscard]] constexpr main_state& get_parent() const noexcept { return parent_; }

		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }

		[[nodiscard]] constexpr object* get_gc_list() noexcept { return gc_list_; }

		[[nodiscard]] constexpr const object* get_gc_list() const noexcept { return gc_list_; }

		[[nodiscard]] constexpr bool is_thread_active() const noexcept { return stack_state_ & thread_active_bit_mask; }

		constexpr void make_thread_inactive() noexcept { stack_state_ &= ~thread_active_bit_mask; }

		constexpr void make_thread_active() noexcept { stack_state_ |= thread_active_bit_mask; }

		[[nodiscard]] constexpr bool is_thread_sleeping() const noexcept { return stack_state_ & thread_sleeping_bit_mask; }

		constexpr void make_thread_wake() noexcept { stack_state_ &= ~thread_sleeping_bit_mask; }

		constexpr void make_thread_sleep() noexcept { stack_state_ |= thread_sleeping_bit_mask; }

		/**
		 * @brief Close all upvalues for this thread
		 */
		void close_upvalue()
		{
			if (open_upvalue_)
			{
				gal_assert(dynamic_cast<object_upvalue*>(open_upvalue_));
				open_upvalue_ = dynamic_cast<object_upvalue*>(open_upvalue_)->close_until(parent_, stack_.data());
			}
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR auto* get_current_environment() const noexcept
		{
			// no enclosing function?
			if (current_call_info_ == 0)
			{
				// use global table as environment
				gal_assert(global_table_.is_table());
				return global_table_.as_table();
			}

			gal_assert(call_infos_[current_call_info_].function->is_function());
			return call_infos_[current_call_info_].function->as_function()->get_environment();
		}

		GAL_ASSERT_CONSTEXPR void set_current_environment(const object& env) noexcept;

		[[nodiscard]] constexpr const object_string* get_named_call() const noexcept { return named_call_; }

	private:
		void push_error(object_string::data_type&& data);

		/**
		 * @brief basic error handler manipulation below
		 */

	public:
		[[noreturn]] void error_type(magic_value value, const object_string::data_type::value_type* operand);

		[[noreturn]] void error_for(magic_value value, const object_string::data_type::value_type* what);

		[[noreturn]] void error_arithmetic(magic_value lhs, magic_value rhs, meta_method_type operand);

		[[noreturn]] void error_order(magic_value lhs, magic_value rhs, meta_method_type operand);

		[[noreturn]] void error_index(magic_value lhs, magic_value rhs);

		[[noreturn]] void error_runtime(const object_string::data_type::value_type* data) { error_runtime(object_string::data_type{data, object_string::data_type::traits_type::length(data), {parent_}}); }

		[[noreturn]] void error_runtime(object_string::data_type&& data);


	private:
		[[nodiscard]] GAL_ASSERT_CONSTEXPR stack_element_type get_stack_element_address(index_type index) noexcept;

		[[nodiscard]] GAL_ASSERT_CONSTEXPR const_stack_element_type get_stack_element_address(const index_type index) const noexcept { return const_cast<child_state&>(*this).get_stack_element_address(index); }

	public:
		/**
		 * @brief basic stack manipulation below
		 */


		/**
		 * @note For singleton type, use it carefully!
		 */
		GAL_ASSERT_CONSTEXPR void push_into_stack_no_check(const magic_value value) noexcept
		{
			gal_assert(value.is_null() || value.is_boolean() || value.is_number());

			gal_assert(&stack_[top_] < call_infos_[current_call_info_].top);
			stack_[top_++] = value;
		}

		GAL_ASSERT_CONSTEXPR void push_into_stack(const magic_value value) noexcept
		{
			gal_assert(&stack_[top_] < call_infos_[current_call_info_].top);
			stack_[top_++].copy_magic_value(parent_, value);
		}

		GAL_ASSERT_CONSTEXPR void fill_stack(const stack_slot_type n) noexcept { while (top_ < base_ + n) { push_into_stack_no_check(magic_value_null); } }

		constexpr void drop_stack(const stack_slot_type n) noexcept { top_ -= n; }

		[[nodiscard]] constexpr stack_slot_type get_stack_last_pos() const noexcept { return stack_.capacity() - extra_stack_size - 1; }

		[[nodiscard]] constexpr auto get_current_stack_size() const noexcept { return top_ - base_; }

		[[nodiscard]] constexpr auto get_total_stack_size() const noexcept { return get_stack_last_pos() - base_; }

		[[nodiscard]] constexpr auto* get_stack_last() noexcept { return &stack_[get_stack_last_pos()]; }

		[[nodiscard]] constexpr auto* get_stack_last() const noexcept { return &stack_[get_stack_last_pos()]; }

		/**
		 * @brief Just peek stack
		 */
		[[nodiscard]] GAL_ASSERT_CONSTEXPR magic_value peek_stack_element(const index_type index) const noexcept
		{
			gal_assert(index > 0 ? top_ + index < stack_.capacity() : std::cmp_greater_equal(top_, -index));
			return stack_[top_ + index];
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR magic_value get_stack_element(index_type index) const noexcept;

		GAL_ASSERT_CONSTEXPR void remove_stack_element(index_type index) noexcept;

		GAL_ASSERT_CONSTEXPR void insert_stack_element(index_type index) noexcept;

		GAL_ASSERT_CONSTEXPR void replace_stack_element(index_type index) noexcept;

		GAL_ASSERT_CONSTEXPR void check_stack(const stack_size_type needed) noexcept { grow_stack(needed); }

		GAL_ASSERT_CONSTEXPR void expand_stack_limit(const stack_size_type needed) noexcept
		{
			gal_assert(std::cmp_less_equal(top_ + needed, get_stack_last_pos()));
			if (auto& top = call_infos_[current_call_info_].top;
				top < &stack_[top_ + needed]) { top = &stack_[top_ + needed]; }
		}

		[[nodiscard]] constexpr bool is_stack_enough(const stack_size_type needed) const noexcept { return std::cmp_greater_equal(call_infos_[current_call_info_].top - &stack_[top_], needed); }

		GAL_ASSERT_CONSTEXPR void move_stack_element(child_state& to, const stack_size_type num) noexcept
		{
			gal_assert(std::cmp_less_equal(num, get_current_stack_size()));
			gal_assert(is_brother(to));
			gal_assert(to.is_stack_enough(num));

			top_ -= num;
			for (auto index = stack_size_type{0}; index < num; ++index) { to.push_into_stack(stack_[top_ + index]); }
		}

	private:
		// internal use only
		void push_string(object_string::data_type&& data)
		{
			// todo
			(void)data;
		}

	public:
		constexpr void wake_me() noexcept;

	private:
		enum class prepare_call_result
		{
			// initiated a call to a GAL function
			gal,
			// did a call to a Internal function
			internal,
			// internal function yielded
			yield
		};

		void execute()
		{
			// todo
		}

		prepare_call_result prepare_call(stack_element_type function, stack_size_type num_results)
		{
			// todo
			(void)function;
			(void)num_results;
			return prepare_call_result::gal;
		}

	public:
		/**
		 * @brief invoke interface below
		 */

		/**
		 * @brief Call a function (GAL or Internal). The function to be called is at function.
		 * The arguments are on the stack, right after the function.
		 * When returns, all the results are on the stack, starting at the original
		 * function position.
		 */
		void call(stack_element_type function, stack_size_type num_results);


		/**
		 * @brief vm utils below
		 * @note internal use only
		 */

		void meta_method_invoke(stack_element_type result, magic_value function, magic_value lhs, magic_value rhs);

		[[nodiscard]] magic_value meta_method_order(magic_value lhs, magic_value rhs, meta_method_type event);

		[[nodiscard]] magic_value meta_method_compare(magic_value lhs, magic_value rhs, meta_method_type event);
	};

	constexpr void object::delete_chain(main_state& state, object* end)
	{
		for (auto* current = this; current != end;)
		{
			auto* next = current->get_next();

			switch (current->type())
			{
				case object_type::string:
				{
					destroy(state, dynamic_cast<object_string*>(current));
					break;
				}
				case object_type::table:
				{
					destroy(state, dynamic_cast<object_table*>(current));
					break;
				}
				case object_type::function:
				{
					destroy(state, dynamic_cast<object_closure*>(current));
					break;
				}
				case object_type::user_data:
				{
					destroy(state, dynamic_cast<object_user_data*>(current));
					break;
				}
				case object_type::thread:
				{
					// delete open upvalues of each thread
					dynamic_cast<child_state*>(current)->close_upvalue();
					break;
				}
				case object_type::null:
				case object_type::boolean:
				case object_type::number:
				case object_type::prototype:
				case object_type::upvalue:
				case object_type::dead_key: { UNREACHABLE(); }
			}

			current = next;
		}
	}

	class main_state final
	{
		friend struct gc_handler;
		friend struct raw_memory;
		friend struct memory_page;

	public:
		/**
		 * @todo The implementation of string_table_ should be a fixed-size bucket array, and then the string is put into the bucket according to the hash value. If there is already a string in the target bucket, the new string is placed in the head of the object_chain where the bucket is located.
		 *
		 * @note The current design causes each string to cause a new node to be inserted, which not only results in additional memory allocation, but also causes the object_chain carried by the string to be wasted.
		 */
		using string_table_type = std::vector<object_string*>;

		using builtin_type_meta_table_type = std::array<object_table*, static_cast<std::size_t>(object_type::tagged_value_count)>;
		using builtin_type_name_table_type = std::array<object_string*, static_cast<std::size_t>(object_type::tagged_value_count)>;
		using meta_method_name_table_type = std::array<object_string*, static_cast<std::size_t>(meta_method_type::meta_method_count)>;

	private:
		gc_handler gc_;

		string_table_type string_table_;

		object::mark_type current_white_;

		// for detect memory leak :(
		// todo: After stack/call_info is using a vector, size/capacity allocation is dynamic, which means we always deallocate them after leaving ~main_state, but we want to make sure free_pages is cleared before that.
		char fake_main_thread_[sizeof(child_state)];
		child_state& main_thread_;

		// head of double-linked list of all open upvalues
		object_upvalue upvalue_head_;

		// meta tables for basic types
		builtin_type_meta_table_type meta_table_{};
		// names for basic types
		builtin_type_name_table_type type_name_{};
		// names for meta method
		meta_method_name_table_type meta_method_name_{};

		// registry table
		magic_value registry_;
		// next free slot in registry
		index_type registry_free_;

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

		[[nodiscard]] child_state* create_child();

		void destroy_child(child_state& state);

		[[nodiscard]] constexpr object::mark_type another_white() const noexcept { return current_white_ ^ object::mark_white_bits_mask; }

		[[nodiscard]] constexpr object::mark_type get_white() const noexcept { return current_white_ & object::mark_white_bits_mask; }

		constexpr void make_white(object& obj) const noexcept { return obj.set_mark((obj.get_mark() & object::mask_marks) | get_white()); }

		constexpr void flip_white() noexcept { current_white_ = another_white(); }

		[[nodiscard]] constexpr bool check_is_dead(const magic_value value) const noexcept { return value.is_object() && check_is_dead(*value.as_object()); }

		[[nodiscard]] constexpr bool check_is_dead(const object& value) const noexcept
		{
			return (value.get_mark() & (object::mark_white_bits_mask | object::mark_fixed_bit_mask)) ==
			       (another_white() & object::mark_white_bits_mask);
		}

		GAL_ASSERT_CONSTEXPR void check_alive([[maybe_unused]] const magic_value value) const noexcept { gal_assert(not value.is_object() || not check_is_dead(*value.as_object())); }

		GAL_ASSERT_CONSTEXPR std::size_t remark_upvalues() { return upvalue_head_.remark(*this); }

		[[nodiscard]] constexpr gc_handler::gc_current_state_type get_gc_state() const noexcept { return gc_.gc_current_state; }

		constexpr void wake_child(child_state& child) noexcept
		{
			if (not child.is_thread_sleeping()) { return; }

			child.make_thread_wake();

			if (gc_.keep_invariant())
			{
				child.set_gc_list(gc_.exchange_gray_again(child));
				child.set_mark_black_to_gray();
			}
		}

		[[nodiscard]] constexpr child_state& get_main_state() const noexcept { return main_thread_; }

		[[nodiscard]] constexpr const object_string* get_table_mode(const object_table& table) const
		{
			return
					table.has_meta_table()
						? not table.check_flag(meta_method_type::mode)
							  ? meta_method_name_[static_cast<std::size_t>(meta_method_type::mode)]
							  : nullptr
						: nullptr;
		}

		constexpr gc_handler& get_gc_handler() noexcept { return gc_; }

		void check_gc() noexcept { gc_.check(main_thread_); }

		void check_thread() const noexcept { if (main_thread_.is_thread_sleeping()) { main_thread_.make_thread_wake(); } }

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

		GAL_ASSERT_CONSTEXPR void link_object(object& object) noexcept
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

		[[nodiscard]] constexpr auto get_registry() const noexcept { return registry_; }

		[[nodiscard]] constexpr auto* get_registry_address() noexcept { return &registry_; }

		[[nodiscard]] constexpr const auto* get_registry_address() const noexcept { return &registry_; }

		[[nodiscard]] GAL_ASSERT_CONSTEXPR const object_string& get_type_name(object_type type) const noexcept
		{
			gal_assert(utils::is_enum_between_of(type, object_type::null, object_type::tagged_value_count));
			return *type_name_[static_cast<std::size_t>(type)];
		}

		[[nodiscard]] const object_string& get_type_name(const magic_value value) const
		{
			if (value.is_user_data() && value.as_user_data()->get_tag() && value.as_user_data()->has_meta_table())
			{
				const auto* user_data = value.as_user_data();

				if (const auto type = user_data->get_meta_table()->find(meta_method_name_[static_cast<std::size_t>(meta_method_type::type)]->operator magic_value());
					type.is_string()) { return *type.as_string(); }
			}
			else if (const auto* table = meta_table_[static_cast<std::size_t>(value.get_type())]; table)
			{
				if (const auto type = table->get_meta_table()->find(meta_method_name_[static_cast<std::size_t>(meta_method_type::type)]->operator magic_value());
					type.is_string()) { return *type.as_string(); }
			}

			return get_type_name(value.get_type());
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR const object_string& get_meta_method_name(meta_method_type event) const noexcept
		{
			gal_assert(utils::is_enum_between_of(event, meta_method_type::index, meta_method_type::meta_method_count));
			return *meta_method_name_[static_cast<std::size_t>(event)];
		}

		[[nodiscard]] magic_value get_meta_method_by_object(const magic_value value, meta_method_type event) const
		{
			const object_table* table;
			if (value.is_table()) { table = value.as_table()->get_meta_table(); }
			else if (value.is_user_data()) { table = value.as_user_data()->get_meta_table(); }
			else { table = meta_table_[static_cast<std::size_t>(value.get_type())]; }

			return table ? table->find(get_meta_method_name(event).operator magic_value()) : magic_value_null;
		}
	};

	constexpr bool child_state::is_oldest_child() const noexcept { return &parent_.get_main_state() == this; }

	GAL_ASSERT_CONSTEXPR void child_state::set_current_environment(const object& env) noexcept
	{
		environment_ = env.operator magic_value();
		gal_assert(parent_.check_is_dead(environment_));
	}

	GAL_ASSERT_CONSTEXPR stack_element_type child_state::get_stack_element_address(const index_type index) noexcept
	{
		if (index > 0)
		{
			gal_assert(std::cmp_less_equal(index, call_infos_[current_call_info_].top - &stack_[base_]));
			auto* element = &stack_[base_ + (index - 1)];
			if (element >= &stack_[top_]) { return nullptr; }
			return element;
		}

		if (index > constant::registry_index)
		{
			gal_assert(index != 0 && std::cmp_less_equal(-index, get_current_stack_size()));
			return &stack_[top_ + index];
		}

		// pseudo
		gal_assert(is_pseudo(index));
		switch (index)
		{
			case constant::registry_index: { return parent_.get_registry_address(); }
			case constant::environment_index:
			{
				set_current_environment(*get_current_environment());
				return &environment_;
			}
			case constant::global_safe_index: { return &global_table_; }
			default:
			{
				gal_assert(call_infos_[current_call_info_].function->is_function());
				auto* function = call_infos_[current_call_info_].function->as_function();
				gal_assert(function->is_internal());
				const auto real_index = constant::global_safe_index - index;
				return std::cmp_less_equal(real_index, function->get_upvalue_size()) ? function->get_upvalue_address(real_index) : nullptr;
			}
		}
	}

	GAL_ASSERT_CONSTEXPR magic_value child_state::get_stack_element(const index_type index) const noexcept
	{
		if (const auto address = get_stack_element_address(index); address) { return *address; }
		return magic_value_null;
	}

	GAL_ASSERT_CONSTEXPR void child_state::remove_stack_element(const index_type index) noexcept
	{
		auto address = get_stack_element_address(index);
		gal_assert(address);
		gal_assert(address < &stack_[top_]);
		while (++address < &stack_[top_]) { (address - 1)->copy_magic_value(parent_, *address); }
		--top_;
	}

	GAL_ASSERT_CONSTEXPR void child_state::insert_stack_element(const index_type index) noexcept
	{
		const auto address = get_stack_element_address(index);
		gal_assert(address);
		for (auto t = top_; &stack_[top_] > address; --t) { stack_[t].copy_magic_value(parent_, stack_[t - 1]); }
		address->copy_magic_value(parent_, stack_[top_]);
	}

	GAL_ASSERT_CONSTEXPR void child_state::replace_stack_element(const index_type index) noexcept
	{
		// explicit test for incompatible code
		if (index == constant::environment_index && current_call_info_ == 0) { error_runtime("no calling environment"); }

		gal_assert(get_current_stack_size() >= 1);

		const auto address = get_stack_element_address(index);
		gal_assert(address);

		gal_assert(call_infos_[current_call_info_].function->is_function());
		auto* function = call_infos_[current_call_info_].function->as_function();


		if (index == constant::environment_index)
		{
			gal_assert(stack_[top_ - 1].is_table());
			function->set_environment(stack_[top_ - 1].as_table());

			parent_.barrier(*function, stack_[top_ - 1]);
		}
		else
		{
			address->copy_magic_value(parent_, stack_[top_ - 1]);
			// function upvalue?
			if (is_upvalue_index(index)) { parent_.barrier(*function, stack_[top_ - 1]); }
		}
		--top_;
	}

	constexpr void child_state::wake_me() noexcept { parent_.wake_child(*this); }

	inline void child_state::meta_method_invoke(stack_element_type result, magic_value function, magic_value lhs, magic_value rhs)
	{
		// save stack
		const auto size = result - stack_.data();

		gal_assert(top_ + 3 < stack_.capacity());
		check_stack(3);

		push_into_stack(function);
		push_into_stack(lhs);
		push_into_stack(rhs);

		call(&stack_[top_ - 1], 1);

		// restore it
		result = stack_.data() + size;

		result->copy_magic_value(parent_, stack_[--top_]);
	}

	inline magic_value child_state::meta_method_order(const magic_value lhs, const magic_value rhs, const meta_method_type event)
	{
		// no meta method?
		if (lhs.is_null()) { return magic_value_undefined; }

		const auto lhs_meta_method = parent_.get_meta_method_by_object(lhs, event);
		const auto rhs_meta_method = parent_.get_meta_method_by_object(rhs, event);

		// different meta method?
		if (not lhs_meta_method.raw_equal(rhs_meta_method)) { return magic_value_undefined; }

		meta_method_invoke(&stack_[top_], lhs_meta_method, lhs, rhs);
		return stack_[top_].is_false() ? magic_value_false : magic_value_true;
	}

	inline magic_value child_state::meta_method_compare(const magic_value lhs, const magic_value rhs, meta_method_type event)
	{
		gal_assert((lhs.is_user_data() && rhs.is_user_data()) || (lhs.is_table() && rhs.is_table()));

		auto get_meta_method = [&](object_table* l, object_table* r)
		{
			const auto lhs_meta = l && not l->check_flag(event) ? l->get_meta_method(event, parent_.get_meta_method_name(event)) : magic_value_null;

			if (lhs_meta == magic_value_null)
			{
				// no meta method
				return magic_value_undefined;
			}

			if (l == r)
			{
				// same meta tables -> same meta methods
				return lhs_meta;
			}

			const auto rhs_meta =
					r && not r->check_flag(event)
						? r->get_meta_method(event, parent_.get_meta_method_name(event))
						: magic_value_null;

			if (rhs_meta == magic_value_null)
			{
				// no meta method
				return magic_value_null;
			}

			if (lhs_meta.raw_equal(rhs_meta))
			{
				// same meta method?
				return lhs_meta;
			}

			return magic_value_null;
		};

		magic_value meta_method;

		if (lhs.is_user_data()) { meta_method = get_meta_method(lhs.as_user_data()->get_meta_table(), rhs.as_user_data()->get_meta_table()); }
		else { meta_method = get_meta_method(lhs.as_table()->get_meta_table(), rhs.as_table()->get_meta_table()); }

		meta_method_invoke(&stack_[top_], meta_method, lhs, rhs);
		return stack_[top_].is_false() ? magic_value_false : magic_value_true;
	}

	GAL_ASSERT_CONSTEXPR void object::mark(main_state& state)
	{
		gal_assert(is_mark_white() && not state.check_is_dead(*this));
		set_mark_white_to_gray();
		do_mark(state);
	}

	inline child_state* magic_value::as_thread() const noexcept
	{
		gal_assert(is_thread());
		return dynamic_cast<child_state*>(as_object());
	}

	GAL_ASSERT_CONSTEXPR void magic_value::copy_magic_value(const main_state& state, const magic_value target) noexcept
	{
		data_ = target.data_;
		state.check_alive(*this);
	}

	GAL_ASSERT_CONSTEXPR int object_prototype::get_line(const call_info& call) const noexcept { return get_line(call.saved_pc ? call.saved_pc - code_.data() - 1 : 0); }
}

#endif // GAL_LANG_VM_STATE_HPP
