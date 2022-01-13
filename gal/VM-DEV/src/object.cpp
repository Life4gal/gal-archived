#include<vm/object.hpp>
#include <algorithm>
#include <vm/state.hpp>

namespace gal::vm_dev
{
	void object_string::do_destroy(main_state& state) { state.remove_string_from_table(*this); }

	void object_user_data::do_destroy(main_state& state)
	{
		gal_assert(tag_ < user_data_tag_limit || tag_ == user_data_tag_inline_destructor);

		gc_handler::user_data_gc_handler gc;

		if (tag_ == user_data_tag_inline_destructor)
		{
			std::memcpy(
					&gc,
					data_.data() + data_.size() - sizeof(gc_handler::user_data_gc_handler),
					sizeof(gc_handler::user_data_gc_handler));
		}
		else { gc = state.get_user_data_gc_handler(tag_); }

		if (gc) { gc(data_.data()); }

		data_.clear();
	}

	void object_prototype::traverse(main_state& state)
	{
		if (source_) { source_->mark(); }
		if (debug_name_) { debug_name_->mark(); }

		// mark literals
		std::ranges::for_each(constants_,
		                      [&state](const auto value) { value.mark(state); });

		// mark upvalue names
		std::ranges::for_each(upvalue_names_,
		                      [](auto* name) { if (name) { name->mark(); } });

		// mark nested prototypes
		std::ranges::for_each(children_,
		                      [&state](auto* proto) { if (proto) { proto->mark(state); } });

		// mark local-variable names
		std::ranges::for_each(
				local_variables_,
				[](auto* name) { if (name) { name->mark(); } },
				[](auto& var) { return var.name; });
	}

	[[nodiscard]] object* object_upvalue::close_until(main_state& state, stack_element_type level)
	{
		auto* current = this;
		auto* next = current->get_next();

		while (current && current->value_ >= level)
		{
			gal_assert(not current->is_mark_black() && current->value_ != &upvalue_.closed);

			if (state.check_is_dead(*current))
			{
				// free upvalue
				destroy(state, current);
			}
			else
			{
				current->unlink();
				current->close(state);
				// link upvalue into gc_root list
				state.link_upvalue(*current);
			}

			if (next)
			{
				current = dynamic_cast<object_upvalue*>(next);
				gal_assert(current);
				next = current->get_next();
			}
			else { current = nullptr; }
		}

		return next;
	}

	void object_closure::traverse(main_state& state)
	{
		environment_->mark(state);

		if (is_internal())
		{
			// mark its upvalues
			std::ranges::for_each(function_.internal.upvalues,
			                      [&state](const auto value) { value.mark(state); });
		}
		else
		{
			function_.gal.prototype->mark(state);
			// mark its upvalues
			std::ranges::for_each(function_.gal.upreferences,
			                      [&state](const auto value) { value.mark(state); });
		}
	}

	void object_table::traverse(main_state& state, const bool weak_key, const bool weak_value)
	{
		std::ranges::for_each(nodes_,
		                      [&state, weak_key, weak_value](const auto& pair)
		                      {
			                      const auto [key, value] = pair;
			                      gal_assert((not key.is_object() || key.as_object()->type() != object_type::dead_key) || value.is_null());

			                      if (value.is_null())
			                      {
				                      // remove empty entries
				                      if (key.is_object()) { key.as_object()->set_type(object_type::dead_key); }
			                      }
			                      else
			                      {
				                      gal_assert(not key.is_null());
				                      if (not weak_key) { key.mark(state); }
				                      if (not weak_value) { value.mark(state); }
			                      }
		                      });
	}

	std::size_t object_table::clear_dead_node(main_state& state)
	{
		std::size_t work = 0;

		for (auto* current = this; current; current = dynamic_cast<object_table*>(current->gc_list_))
		{
			work += current->memory_usage();

			std::ranges::for_each(nodes_,
			                      [](auto& pair)
			                      {
				                      auto& [key, value] = pair;

				                      // non-empty entry?
				                      if (not value.is_null())
				                      {
					                      // can we clear key or value?
					                      if ((key.is_object() && key.as_object()->is_object_cleared()) || (value.is_object() && value.as_object()->is_object_cleared()))
					                      {
						                      // remove value
						                      // todo: just assign?
						                      value = magic_value_null;
						                      // remove entry from table
						                      if (key.is_object()) { key.as_object()->set_type(object_type::dead_key); }
					                      }
				                      }
			                      });
		}
	}
}
