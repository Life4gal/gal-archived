#include <vm/state.hpp>
#include <vm/memory.hpp>
#include <vm/gc.hpp>
#include <vm/allocator.hpp>

namespace gal::vm
{
	void global_state::destroy_all(thread_state& state)
	{
		gal_assert(main_thread == &state);
		// main thread is at the end of root_gc list
		gal_assert(not state.has_next());
		// todo
	}

	void thread_state::new_stack()
	{
		gal_assert(top_ == 0);
		gal_assert(base_ == 0);
		gal_assert(current_call_info_ = 0);

		// initialize first call info
		base_call_info_[current_call_info_].function = &stack_[top_];
		// function entry for this call info
		++top_;

		base_ = top_;
		base_call_info_[current_call_info_].base = &stack_[top_];
		base_call_info_[current_call_info_].top = &stack_[top_ + min_stack_size];
	}

	void thread_state::destroy_stack()
	{
		// todo: destruct magic_value ?
	}

	void thread_state::close_stack(stack_index_type level)
	{
		for (
			gal_upvalue* upvalue = nullptr;
			open_upvalue_ &&
			(gal_assert(open_upvalue_->type() == object_type::upvalue), (upvalue = dynamic_cast<gal_upvalue*>(open_upvalue_))->get_index() >= level);
		)
		{
			gal_assert(not upvalue->is_mark_black() && *upvalue->get_index() != upvalue->get_close_value());
			// remove from open list
			open_upvalue_ = upvalue->get_next();

			if (global_.is_dead(*upvalue)) { upvalue->destroy(); }
			else
			{
				upvalue->unlink();
				upvalue->close(global_);
				// link upvalue into gc_root list
				global_.link_upvalue(*upvalue);
			}
		}
	}


	void thread_state::destroy_list(object** begin, object* end)
	{
		for (auto* current = *begin; current != end; current = *begin)
		{
			if (current->type() == object_type::thread)
			{
				// delete open upvalues of each thread
				auto* t = dynamic_cast<thread_state*>(current);
				gal_assert(t);
				destroy_list(&t->open_upvalue_, nullptr);
			}

			*begin = current->get_next();
			current->destroy();
		}
	}


	thread_state::thread_state(
			global_state& global,
			const memory_categories_type active_memory_category)
		: object{object_type::thread, active_memory_category},
		  status_{0},
		  active_memory_category_{active_memory_category},
		  stack_state_{0},
		  single_step_{false},
		  global_{global},
		  top_{0},
		  base_{0},
		  base_call_info_{},
		  current_call_info_{0},
		  num_internal_calls_{0},
		  base_internal_calls_{0},
		  cached_slot_{0},
		  named_call_{nullptr},
		  user_data_{nullptr} { new_stack(); }

	thread_state* thread_state::new_thread()
	{
		vm_allocator<thread_state> allocator{*this};

		auto* child = allocator.allocate(1);
		allocator.construct(child, global_, active_memory_category_);

		global_.link_object(*child);
		// share table of globals
		child->global_table_.copy_magic_value(global_, global_table_);
		child->single_step_ = single_step_;

		gal_assert(child->is_mark_white());

		return child;
	}
}
