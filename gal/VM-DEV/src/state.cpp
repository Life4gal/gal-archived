#include<vm/state.hpp>
#include<chrono>
#include <algorithm>
#include <utils/macro.hpp>

namespace
{
	struct time_guardian
	{
		std::chrono::system_clock::time_point begin;

		time_guardian()
			: begin{std::chrono::system_clock::now()} {}

		[[nodiscard]] double get() const noexcept { return std::chrono::duration<double>(std::chrono::system_clock::now() - begin).count(); }
	};
}

namespace gal::vm_dev
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
		return 0;
	}

	void child_state::traverse(main_state& state, bool clear_stack)
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

	void main_state::mark_meta_table()
	{
		std::ranges::for_each(
				meta_table_,
				[this](object_table* table) { if (table) { table->try_mark(*this); } });
	}
}
