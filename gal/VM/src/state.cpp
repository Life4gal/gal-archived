#include<vm/state.hpp>
#include<chrono>
#include <algorithm>
#include <utils/macro.hpp>
#include <ranges>
#include <vm/exception.hpp>
#include <source/chunk.hpp>

namespace
{
	[[nodiscard]] std::uint64_t get_time_now_milliseconds() noexcept
	{
		using namespace std::chrono;
		return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
	}

	[[nodiscard]] double get_time_duration(const std::uint64_t begin, const std::uint64_t end) noexcept
	{
		using namespace gal;
		using namespace std::chrono;
		using namespace std::chrono_literals;

		gal_assert(begin < end);
		return duration_cast<duration<double>>(steady_clock::time_point{milliseconds{end}} - steady_clock::time_point{milliseconds{begin}}).count();
	}

	[[nodiscard]] double get_time_duration(const std::uint64_t begin) noexcept
	{
		using namespace std::chrono;
		using namespace std::chrono_literals;
		return duration_cast<duration<double>>(steady_clock::now() - steady_clock::time_point{milliseconds{begin}}).count();
	}
}

namespace gal::vm
{
	bool gc_handler::traverse_table(main_state& state, object_table& table)
	{
		table.mark_meta_table(state);

		bool weak_key = false;
		bool weak_value = false;
		// is there a weak mode?
		if (const auto* mode = state.get_table_mode(table); mode)
		{
			const auto& data = mode->get_data();
			weak_key = data.contains('k');
			weak_value = data.contains('v');
			// is really weak?
			if (weak_key || weak_value)
			{
				// must be cleared after GC and put in the appropriate list
				table.set_gc_list(exchange_weak(table));
			}
		}

		if (weak_key && weak_value) { return true; }

		table.traverse(state, weak_key, weak_value);

		return weak_key || weak_value;
	}

	std::size_t gc_handler::propagate_mark(main_state& state)
	{
		gal_assert(gray->is_mark_gray());
		gray->set_mark_gray_to_black();

		switch (gray->type())
		{
			case object_type::table:
			{
				auto* table = dynamic_cast<object_table*>(gray);
				gal_assert(table);

				gray = table->get_gc_list();
				// table is weak?
				if (traverse_table(state, *table))
				{
					// keep it gray
					table->set_mark_black_to_gray();
				}
				return table->memory_usage();
			}
			case object_type::function:
			{
				auto* closure = dynamic_cast<object_closure*>(gray);
				gal_assert(closure);

				gray = closure->get_gc_list();
				closure->traverse(state);
				return closure->memory_usage();
			}
			case object_type::thread:
			{
				auto* thread = dynamic_cast<child_state*>(gray);
				gal_assert(thread);

				gray = thread->get_gc_list();

				gal_assert(not thread->is_thread_sleeping());

				// threads that are executing and the main thread are not deactivated

				if (const auto active = thread->is_thread_active() || &state.main_thread_ == thread;
					not active && gc_current_state == gc_current_state_type::propagate)
				{
					thread->traverse(state, true);

					thread->make_stack_sleep();
				}
				else
				{
					thread->set_gc_list(exchange_gray_again(*thread));

					thread->set_mark_black_to_gray();

					thread->traverse(state, false);
				}

				return thread->memory_usage();
			}
			case object_type::prototype:
			{
				auto* prototype = dynamic_cast<object_prototype*>(gray);
				gal_assert(prototype);

				gray = prototype->get_gc_list();

				prototype->traverse(state);

				return prototype->memory_usage();
			}
			case object_type::null:
			case object_type::boolean:
			case object_type::number:
			case object_type::string:
			case object_type::user_data:
			case object_type::upvalue:
			case object_type::dead_key: { break; }
		}

		UNREACHABLE();
	}

	std::size_t gc_handler::atomic(child_state& state)
	{
		gal_assert(gc_current_state == gc_current_state_type::atomic);

		auto& parent = state.parent_;

		std::size_t work = 0;
		// remark occasional upvalues of (maybe) dead threads
		work += parent.remark_upvalues();
		// traverse objects caught by write barrier and by 'remark_upvalues'
		work += propagate_all(parent);
		// remark weak tables
		gray = std::exchange(weak, nullptr);

		gal_assert(not parent.main_thread_.is_mark_white());

		// mark running thread
		state.try_mark(parent);
		// mark basic meta tables (again)
		parent.mark_meta_table();

		work += propagate_all(parent);

		// remark gray again
		gray = std::exchange(gray_again, nullptr);

		work += propagate_all(parent);
		// remove collected objects from weak tables
		work += weak ? dynamic_cast<object_table*>(weak)->clear_dead_node() : 0;
		weak = nullptr;

		// flip current white
		parent.flip_white();

		sweep_string_gc = 0;
		sweep_gc = root_gc;
		gc_current_state = gc_current_state_type::sweep_string;

		return work;
	}

	object* gc_handler::sweep_list(main_state& state, object* begin, std::size_t* traversed_count, std::size_t count)
	{
		const auto dead_mask = state.another_white();
		const auto begin_count = count;

		// make sure we never sweep fixed objects
		gal_assert(dead_mask & object::mark_fixed_bit_mask);

		auto* current = begin;
		while (current && count-- > 0)
		{
			const auto alive = (current->get_mark() ^ object::mark_white_bits_mask) & dead_mask;

			if (current->type() == object_type::thread)
			{
				auto* thread = dynamic_cast<child_state*>(current);
				gal_assert(thread);
				// sweep open upvalues
				sweep_list(state, thread->open_upvalue_, traversed_count);

				if (alive) { thread->make_stack_wake(); }
			}

			if (alive)
			{
				// not dead?
				gal_assert(not state.check_is_dead(*current));

				// make it white (for next cycle)
				state.make_white(*current);

				begin = current;
				current = current->get_next();
			}
			else
			{
				// must erase current
				gal_assert(state.check_is_dead(*current));

				auto* next = current->get_next();

				begin->reset_next(next);
				// is the first element of the list?
				if (current == root_gc)
				{
					// adjust first
					root_gc = next;
				}

				object::destroy(state, current);

				current = next;
			}
		}

		// if we did not reach the end of the list it means that we've stopped because the count dropped below zero
		if (traversed_count) { *traversed_count += begin_count - (count + (current ? 1 : 0)); }

		return begin;
	}

	std::size_t gc_handler::step(main_state& state, const std::size_t limit)
	{
		std::size_t cost = 0;
		switch (gc_current_state)
		{
			case gc_current_state_type::pause:
			{
				// start a new collection
				state.mark_root();
				gal_assert(gc_current_state == gc_current_state_type::propagate);
				break;
			}
			case gc_current_state_type::propagate:
			case gc_current_state_type::propagate_again:
			{
				while (gray && cost < limit)
				{
					++gc_states.current_cycle.mark_items;

					cost += propagate_mark(state);
				}

				if (not gray)
				{
					if (gc_current_state == gc_current_state_type::propagate)
					{
						// perform one iteration over 'gray again' list
						gray = std::exchange(gray_again, nullptr);

						gc_current_state = gc_current_state_type::propagate_again;
					}
					else
					{
						// no more `gray' objects
						gc_current_state = gc_current_state_type::atomic;
					}
				}
				break;
			}
			case gc_current_state_type::atomic:
			{
				gc_states.current_cycle.atomic_begin_time_stamp = std::chrono::steady_clock::now().time_since_epoch().count();
				gc_states.current_cycle.atomic_begin_total_size_bytes = total_bytes;

				// finish mark phase
				cost = atomic(state.main_thread_);
				gal_assert(gc_current_state == gc_current_state_type::sweep_string);
				break;
			}
			case gc_current_state_type::sweep_string:
			{
				while (sweep_string_gc < state.string_table_.size() && cost < limit)
				{
					std::size_t traversed_count = 0;
					sweep_list(state, state.string_table_[sweep_string_gc++], &traversed_count);

					gc_states.current_cycle.sweep_items += traversed_count;
					cost += sweep_cost;
				}

				// nothing more to sweep?
				if (sweep_string_gc >= state.string_table_.size())
				{
					// sweep string buffer list
					std::size_t traversed_count = 0;
					sweep_list(state, string_buffer_gc, &traversed_count);

					gc_states.current_cycle.sweep_items += traversed_count;
					// end sweep-string phase
					gc_current_state = gc_current_state_type::sweep;
				}
				break;
			}
			case gc_current_state_type::sweep:
			{
				while (sweep_gc && cost < limit)
				{
					std::size_t traversed_count = 0;
					sweep_gc = sweep_list(state, sweep_gc, &traversed_count, sweep_max_count);

					gc_states.current_cycle.sweep_items += traversed_count;
					cost += sweep_max_count * sweep_cost;
				}

				if (not sweep_gc)
				{
					// nothing more to sweep?
					// end collection
					gc_current_state = gc_current_state_type::pause;
				}
				break;
			}
		}

		return cost;
	}

	void gc_handler::begin_gc_cycle()
	{
		gc_states.current_cycle.begin_time_stamp = get_time_now_milliseconds();
		gc_states.current_cycle.wait_time = get_time_duration(gc_states.current_cycle.begin_time_stamp, gc_states.current_cycle.end_time_stamp);
	}

	void gc_handler::end_gc_cycle()
	{
		gc_states.current_cycle.end_time_stamp = get_time_now_milliseconds();
		gc_states.current_cycle.end_total_size_bytes = total_bytes;

		++gc_states.completed_cycles;
		gc_states.last_cycle = gc_states.current_cycle;
		gc_states.current_cycle = {};

		gc_states.cycle_state_accumulate.mark_time += gc_states.last_cycle.mark_time;
		gc_states.cycle_state_accumulate.mark_items += gc_states.last_cycle.mark_items;
		gc_states.cycle_state_accumulate.atomic_time += gc_states.last_cycle.atomic_time;
		gc_states.cycle_state_accumulate.sweep_time += gc_states.last_cycle.sweep_time;
		gc_states.cycle_state_accumulate.sweep_items += gc_states.last_cycle.sweep_items;
	}


	void gc_handler::record_state_time(const gc_current_state_type current_state, const double second, const bool assist)
	{
		switch (current_state)
		{
			case gc_current_state_type::pause:
			{
				// record root mark time if we have switched to next state
				if (gc_current_state == gc_current_state_type::propagate) { gc_states.current_cycle.mark_time += second; }
				break;
			}
			case gc_current_state_type::propagate:
			case gc_current_state_type::propagate_again:
			{
				gc_states.current_cycle.mark_time += second;
				break;
			}
			case gc_current_state_type::atomic:
			{
				gc_states.current_cycle.atomic_time += second;
				break;
			}
			case gc_current_state_type::sweep_string:
			case gc_current_state_type::sweep:
			{
				gc_states.current_cycle.sweep_time += second;
				break;
			}
		}

		if (assist) { gc_states.step_assist_time_accumulate += second; }
		else { gc_states.step_explicit_time_accumulate += second; }
	}

	std::size_t gc_handler::get_heap_trigger(std::size_t heap_goal)
	{
		auto get_heap_trigger_error_offset = [this](const gc_cycle_state& cycle_state)
		{
			// adjust for error using Proportional-Integral controller
			// https://en.wikipedia.org/wiki/PID_controller
			const auto error_kb = static_cast<gc_heap_trigger_state::term_type>((cycle_state.atomic_begin_total_size_bytes - cycle_state.heap_goal_size_bytes) / 1024);

			// we use sliding window for the error integral to avoid error sum 'windup' when the desired target cannot be reached
			auto* slot = &gc_states.trigger_state.terms[gc_states.trigger_state.term_pos % gc_heap_trigger_state::term_count];
			const auto prev = *slot;
			*slot = error_kb;
			gc_states.trigger_state.integral += error_kb - prev;
			++gc_states.trigger_state.term_pos;

			// controller tuning
			// https://en.wikipedia.org/wiki/Ziegler%E2%80%93Nichols_method
			constexpr double ku = 0.9;// ultimate gain (measured)
			constexpr double tu = 2.5;// oscillation period (measured)

			constexpr double kp = 0.45 * ku;// proportional gain
			constexpr double ti = 0.8 * tu;
			constexpr double ki = 0.54 * ku / ti;// integral gain

			return static_cast<std::size_t>((kp * error_kb + ki * gc_states.trigger_state.integral) * 1024);
		};

		const auto& last_cycle = gc_states.last_cycle;
		const auto& current_cycle = gc_states.current_cycle;

		// adjust threshold based on a guess of how many bytes will be allocated between the cycle start and sweep phase
		// our goal is to begin the sweep when used memory has reached the heap goal
		const auto allocation_duration = get_time_duration(last_cycle.end_time_stamp, current_cycle.atomic_begin_time_stamp);

		// avoid measuring intervals smaller than 1ms
		if (constexpr double duration_threshold = 1e-3;
			allocation_duration < duration_threshold) { return heap_goal; }

		const auto allocation_rate = static_cast<double>(current_cycle.atomic_begin_total_size_bytes - last_cycle.end_total_size_bytes) / allocation_duration;
		const auto mark_duration = get_time_duration(current_cycle.begin_time_stamp, current_cycle.atomic_begin_time_stamp);

		const auto expected_growth = static_cast<std::size_t>(mark_duration * allocation_rate);
		const auto offset = get_heap_trigger_error_offset(current_cycle);
		const auto heap_trigger = heap_goal - (expected_growth + offset);

		// clamp the trigger between memory use at the end of the cycle and the heap goal
		return std::clamp(heap_trigger, total_bytes, heap_goal);
	}

	GAL_ASSERT_CONSTEXPR void gc_handler::validate_gray_list(const object& list) const
	{
		if (not keep_invariant()) { return; }

		const auto* current = &list;
		while (current)
		{
			gal_assert(current->is_mark_gray());

			switch (current->type())
			{
				case object_type::prototype:
				{
					gal_assert(dynamic_cast<const object_prototype*>(current));
					current = dynamic_cast<const object_prototype*>(current)->get_gc_list();
					break;
				}
				case object_type::function:
				{
					gal_assert(dynamic_cast<const object_closure*>(current));
					current = dynamic_cast<const object_closure*>(current)->get_gc_list();
					break;
				}
				case object_type::table:
				{
					gal_assert(dynamic_cast<const object_table*>(current));
					current = dynamic_cast<const object_table*>(current)->get_gc_list();
					break;
				}
				case object_type::thread:
				{
					gal_assert(dynamic_cast<const child_state*>(current));
					current = dynamic_cast<const child_state*>(current)->get_gc_list();
					break;
				}
				case object_type::null:
				case object_type::boolean:
				case object_type::number:
				case object_type::string:
				case object_type::user_data:
				case object_type::upvalue:
				case object_type::dead_key: { UNREACHABLE(); }
			}
		}
	}

	gc_handler::gc_handler(object* root)
		: root_gc{root},
		  sweep_gc{root_gc},
		  total_bytes{sizeof(main_state)},
		  free_pages{} { std::ranges::fill(free_pages, nullptr); }

	void gc_handler::step(child_state& state, const bool assist)
	{
		const auto& callback = state.parent_.get_callback_info();

		// how much to work
		const auto limit = gc_step_size / 100 * gc_step_multiple;
		gal_assert(total_bytes >= gc_threshold);

		const auto debt = total_bytes - gc_threshold;

		if (callback.interrupt) { [[unlikely]] callback.interrupt(state, 0); }

		// at the start of the new cycle
		if (gc_current_state == gc_current_state_type::pause) { begin_gc_cycle(); }

		const auto last_gc_state = gc_current_state;
		const auto last_time_stamp = get_time_now_milliseconds();

		const auto work = step(state.parent_, limit);

		if (assist) { gc_states.current_cycle.assist_work += work; }
		else { gc_states.current_cycle.explicit_work += work; }

		record_state_time(last_gc_state, get_time_duration(last_time_stamp), assist);

		// at the end of the last cycle
		if (gc_current_state == gc_current_state_type::pause)
		{
			// at the end of a collection cycle, set goal based on gc_goal setting
			const auto heap_goal = total_bytes / 100 * gc_goal;
			const auto heap_trigger = get_heap_trigger(heap_goal);

			gc_threshold = heap_trigger;

			end_gc_cycle();

			gc_states.current_cycle.heap_goal_size_bytes = heap_goal;
			gc_states.current_cycle.heap_trigger_size_bytes = heap_trigger;
		}
		else
		{
			gc_threshold = total_bytes + gc_step_size;

			// compensate if GC is "behind schedule" (has some debt to pay)
			if (gc_threshold > debt) { gc_threshold -= debt; }
		}

		if (callback.interrupt) { [[unlikely]] callback.interrupt(state, static_cast<int>(last_gc_state)); }
	}

	void gc_handler::full_gc(main_state& state)
	{
		if (gc_current_state == gc_current_state_type::pause) { begin_gc_cycle(); }

		if (utils::is_enum_between_of(gc_current_state, gc_current_state_type::propagate, gc_current_state_type::atomic))
		{
			// reset sweep marks to sweep all elements (returning them to white)
			sweep_string_gc = 0;
			sweep_gc = root_gc;
			// reset other collector lists
			gray = nullptr;
			gray_again = nullptr;
			weak = nullptr;
			gc_current_state = gc_current_state_type::sweep_string;
		}

		gal_assert(utils::is_any_enum_of(gc_current_state, gc_current_state_type::sweep_string, gc_current_state_type::sweep));
		// finish any pending sweep phase
		while (gc_current_state != gc_current_state_type::pause)
		{
			gal_assert(utils::is_any_enum_of(gc_current_state, gc_current_state_type::sweep_string, gc_current_state_type::sweep));
			step(state);
		}

		end_gc_cycle();

		// run a full collection cycle
		begin_gc_cycle();

		state.mark_root();
		while (gc_current_state != gc_current_state_type::pause) { step(state); }

		const auto heap_goal_size_bytes = total_bytes / 100 * gc_goal;

		// trigger cannot be correctly adjusted after a forced full GC.
		// we will try to place it so that we can reach the goal based on
		// the rate at which we run the GC relative to allocation rate
		// and on amount of bytes we need to traverse in propagation stage.
		// goal and step_multiple are defined in percents
		gc_threshold = total_bytes * (gc_goal * gc_step_multiple / 100 - 100) / gc_step_multiple;

		// but it might be impossible to satisfy that directly
		gc_threshold = std::ranges::max(gc_threshold, total_bytes);

		end_gc_cycle();

		gc_states.current_cycle.heap_goal_size_bytes = heap_goal_size_bytes;
		gc_states.current_cycle.heap_trigger_size_bytes = gc_threshold;
	}

	void gc_handler::validate(main_state& state) const
	{
		gal_assert(not state.check_is_dead(state.main_thread_));
		gal_assert(not state.registry_.is_object() || not state.check_is_dead(*state.registry_.as_object()));

		std::ranges::for_each(state.meta_table_,
		                      [&state](const auto* table) { if (table) { gal_assert(not state.check_is_dead(*table)); } });

		validate_gray_list(*weak);
		validate_gray_list(*gray);
		validate_gray_list(*gray_again);

		std::ranges::for_each(state.string_table_,
		                      [this](const auto* string) { if (string) { validate_gray_list(*string); } });

		validate_gray_list(*root_gc);
		validate_gray_list(*string_buffer_gc);

		state.upvalue_head_.check_list();
	}

	void child_state::do_destroy(main_state& state)
	{
		gal_assert(&parent_ == &state);

		// close all upvalues for this thread
		gal_assert(dynamic_cast<object_upvalue*>(open_upvalue_));
		open_upvalue_ = dynamic_cast<object_upvalue*>(open_upvalue_)->close_until(parent_, stack_.data());
		gal_assert(open_upvalue_ == nullptr);

		if (const auto& callback = parent_.get_callback_info();
			callback.user_thread) { callback.user_thread(nullptr, *this); }

		destroy(parent_, this);

		(void)state;
	}

	child_state::child_state(main_state& parent)
		: object{object_type::thread, mark_white_bit0_mask | mark_fixed_bit_mask},
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
		  global_table_(
				  create<object_table>(
						  parent,
						  #ifndef GAL_ALLOCATOR_NO_TRACE
						  std_source_location::current(),
						  #endif
						  parent
						  )->operator magic_value()),
		  open_upvalue_{nullptr},
		  gc_list_{nullptr},
		  named_call_{nullptr},
		  user_data_{nullptr}
	{
		// initialize first call info
		call_infos_[0] = {
				.base = &stack_[0],
				.function = &stack_[0],
				.top = &stack_[min_stack_size],
				.saved_pc = 0,
				.num_returns = 0,
				.flags = 0};

		// main thread only
		gal_assert(this == &parent_.get_main_state());
	}

	child_state::child_state(child_state& brother)
		: object{object_type::thread},
		  parent_{brother.parent_},
		  status_{0},
		  stack_state_{0},
		  single_step_{brother.single_step_},
		  // function entry for this call info
		  top_{1},
		  base_{1},
		  call_infos_{},
		  current_call_info_{0},
		  num_internal_calls_{0},
		  base_internal_calls_{0},
		  cached_slot_{0},
		  open_upvalue_{nullptr},
		  gc_list_{nullptr},
		  named_call_{nullptr},
		  user_data_{nullptr}
	{
		// initialize first call info
		call_infos_[0] = {
				.base = &stack_[0],
				.function = &stack_[0],
				.top = &stack_[min_stack_size],
				.saved_pc = 0,
				.num_returns = 0,
				.flags = 0};

		// brother is main thread only
		gal_assert(&brother == &parent_.get_main_state());

		parent_.link_object(*this);

		// share table of globals
		global_table_.copy_magic_value(parent_, brother.global_table_);

		brother.push_into_stack(this->operator magic_value());

		gal_assert(is_mark_white());
	}

	void child_state::reset()
	{
		close_upvalue();
		// clear call frames
		auto& base = call_infos_.front();
		base.base = stack_.data() + 1;
		base.function = stack_.data();
		base.top = base.base + min_stack_size;

		current_call_info_ = 0;

		// clear thread state
		status_ = thread_status::ok;
		top_ = 0;
		base_ = 0;
		num_internal_calls_ = 0;
		base_internal_calls_ = 0;
		// clear thread stack
		std::ranges::fill(stack_, magic_value_null);
	}

	void child_state::traverse(main_state& state, const bool clear_stack)
	{
		global_table_.mark(state);

		if (named_call_) { named_call_->mark(); }

		for (stack_slot_type i = 0; i < top_; ++i) { stack_[i].mark(state); }

		// final traversal?
		if (state.get_gc_state() == gc_handler::gc_current_state_type::atomic || clear_stack)
		{
			// clear not-marked stack slice
			std::ranges::for_each(stack_,
			                      [](auto& value)
			                      {
				                      if (value.is_object())
				                      {
					                      // todo: just set?
					                      value = magic_value_null;
				                      }
			                      });
		}
	}

	void child_state::push_error(object_string::data_type&& data)
	{
		const auto& call = call_infos_[current_call_info_];
		if (call.function->is_function() && not call.function->as_function()->is_internal())
		{
			constexpr auto reserve_size = max_id_size + sizeof(" (FROM: :)") + 2;

			const auto* prototype = call.function->as_function()->get_prototype();

			object_string::data_type str{std::move(data)};
			str.reserve(str.size() + reserve_size);

			str.append(" (FROM: ");
			// add file:line information
			get_chunk_id(str.data() + str.size(), prototype->get_source()->get_raw_data(), max_id_size);
			std_format::format_to(std::back_inserter(str), ":{})", prototype->get_line(call));
			push_string(std::move(str));
		}
		else { push_string(std::move(data)); }
	}

	void child_state::runtime_error(object_string::data_type&& data)
	{
		push_error(std::move(data));
		throw vm_exception{parent_, thread_status::error_run};
	}

	void main_state::mark_meta_table()
	{
		std::ranges::for_each(
				meta_table_,
				[this](object_table* table) { if (table) { table->try_mark(*this); } });
	}

	main_state::main_state()
		: gc_{&main_thread_},
		  current_white_{object::mark_white_bits_mask},
		  main_thread_{*this},
		  registry_(object::create<object_table>(
				  *this,
				  #ifndef GAL_ALLOCATOR_NO_TRACE
				  std_source_location::current(),
				  #endif
				  *this
				  )->operator magic_value()),
		  registry_free_{0},
		  callback_{}
	{
		for (decltype(type_name_.size()) i = 0; i < type_name_.size(); ++i)
		{
			type_name_[i] = object::create<object_string>(
					*this,
					#ifndef GAL_ALLOCATOR_NO_TRACE
					std_source_location::current(),
					#endif
					*this,
					gal_typename[i].data(),
					gal_typename[i].size());
			// never collect these names
			type_name_[i]->set_mark_fix();
		}

		for (decltype(tagged_method_name_.size()) i = 0; i < tagged_method_name_.size(); ++i)
		{
			tagged_method_name_[i] = object::create<object_string>(
					*this,
					#ifndef GAL_ALLOCATOR_NO_TRACE
					std_source_location::current(),
					#endif
					*this,
					gal_event_name[i].data(),
					gal_event_name[i].size());
			// never collect these names
			tagged_method_name_[i]->set_mark_fix();
		}

		// pin to make sure we can always throw these error
		(void)object::create<object_string>(
				*this,
				#ifndef GAL_ALLOCATOR_NO_TRACE
				std_source_location::current(),
				#endif
				*this,
				"Out of memory");
		(void)object::create<object_string>(
				*this,
				#ifndef GAL_ALLOCATOR_NO_TRACE
				std_source_location::current(),
				#endif
				*this,
				"Error in error handling");

		gc_.gc_threshold = 4 * gc_.total_bytes;
	}

	main_state::~main_state() noexcept
	{
		main_thread_.close_upvalue();

		// collect all objects
		gal_assert(not main_thread_.has_next());
		gc_.root_gc->delete_chain(*this, &main_thread_);

		// free all string lists
		// todo: deleting strings from string_table_ will cause the vector to be altered and the iterator to be invalidated.
		// remove from back
		// for (auto* string: string_table_ | std::views::reverse)
		// {
		// 	string->delete_chain(*this);
		// }
		// for (auto it = string_table_.rbegin(), next = it + 1; it != string_table_.rend(); it = next, next = it + 1)
		// {
		// 	(*it)->delete_chain(*this);
		// }
		while (not string_table_.empty()) { string_table_.back()->delete_chain(*this); }
		string_table_.shrink_to_fit();

		gal_assert(string_table_.empty());

		if (gc_.string_buffer_gc) { gc_.string_buffer_gc->delete_chain(*this); }
		// unfortunately, when string objects are freed, the string table use count is decremented
		// even when the string is a buffer that wasn't placed into the table
		gal_assert(string_table_.empty());

		gal_assert(gc_.root_gc == &main_thread_);
		gal_assert(gc_.string_buffer_gc == nullptr);

		gal_assert(registry_.is_table());
		object::destroy(*this, registry_.as_table());

		gal_assert(main_thread_.global_table_.is_table());
		object::destroy(*this, main_thread_.global_table_.as_table());

		gal_assert(std::ranges::all_of(gc_.free_pages, [](const auto* page) { return page == nullptr; }));

		gal_assert(gc_.total_bytes == sizeof(main_state));

		#ifndef GAL_ALLOCATOR_NO_TRACE
		raw_memory::print_trace_log();
		#endif
	}

	child_state* main_state::create_child()
	{
		vm_allocator<child_state> allocator{*this};

		auto* child = allocator.allocate(1);
		allocator.construct(child, main_thread_);

		if (callback_.user_thread) { callback_.user_thread(this, *child); }

		return child;
	}

	void main_state::destroy_child(child_state& state)
	{
		state.close_upvalue();
		gal_assert(state.open_upvalue_ == nullptr);
		if (callback_.user_thread) { callback_.user_thread(nullptr, state); }

		object::destroy(*this, &state);
	}

	void main_state::add_string_into_table(object_string& string) { string_table_.push_back(&string); }

	void main_state::remove_string_from_table(object_string& string)
	{
		const auto it = std::ranges::find(string_table_, &string);

		gal_assert(it != string_table_.end());

		string_table_.erase(it);
	}

	namespace state
	{
		[[nodiscard]] main_state* new_state() { return new main_state{}; }

		void destroy_state(main_state& state) { delete &state; }

		[[nodiscard]] child_state* new_thread(main_state& state)
		{
			state.check_gc();
			state.check_thread();
			return state.create_child();
		}

		[[nodiscard]] main_state& main_thread(const child_state& state) { return state.get_parent(); }

		void reset_thread(child_state& state) { state.reset(); }

		[[nodiscard]] boolean_type is_thread_reset(const child_state& state) { return state.is_reset(); }
	}
}
