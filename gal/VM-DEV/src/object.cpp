#include<vm/object.hpp>
#include <algorithm>

namespace gal::vm_dev
{
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
}
