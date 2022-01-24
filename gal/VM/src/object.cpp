#include<vm/object.hpp>
#include <algorithm>
#include <vm/state.hpp>
#include <charconv>
#include <utils/function.hpp>

namespace gal::vm
{
	object::~object() noexcept = default;

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

	object_string::object_string(main_state& state, const data_type::value_type* data)
		: object_string{state, data, data_type::traits_type::length(data)} { }

	object_string::object_string(main_state& state, const data_type::value_type* data, const data_type::size_type size)
		: object{object_type::string},
		  atomic_{0},
		  data_{data, size, {state}} { state.add_string_into_table(*this); }

	object_string::object_string(main_state& state, data_type&& data)
		: object{object_type::string},
		  atomic_{0},
		  data_{std::move(data)} { state.add_string_into_table(*this); }

	object_prototype::object_prototype(main_state& state)
		: object{object_type::prototype},
		  constants_{{state}},
		  code_{{state}},
		  children_{{state}},
		  line_info_{{state}},
		  abs_line_info_{nullptr},
		  line_gap_log2_{0},
		  local_variables_{{state}},
		  upvalue_names_{{state}},
		  source_{nullptr},
		  debug_name_{nullptr},
		  debug_instructions_{{state}},
		  gc_list_{nullptr},
		  num_upvalues_{0},
		  num_params_{0},
		  is_vararg_{0},
		  max_stack_size_{0} { state.link_object(*this); }

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

	const object_prototype::local_variable* object_prototype::get_local(int local_number, const compiler::debug_pc_type pc)
	{
		const auto it = std::ranges::find_if(local_variables_,
		                                     [local_number, pc](const auto& local) mutable
		                                     {
			                                     if (pc >= local.begin_pc && pc <= local.end_pc)
			                                     {
				                                     // is variable active?
				                                     --local_number;
				                                     if (local_number == 0) { return true; }
			                                     }
			                                     return false;
		                                     });
		return it != local_variables_.end() ? &*it : nullptr;
	}

	[[nodiscard]] object* object_upvalue::close_until(main_state& state, std::add_const_t<stack_element_type> level)
	{
		auto* current = this;
		auto* next = current->get_next();

		while (current && current->value_ >= level)
		{
			gal_assert(not current->is_mark_black() && not current->is_closed());

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

	object_upvalue* object_upvalue::find_upvalue(main_state& state, stack_element_type level, object_upvalue& link)
	{
		auto* current = this;
		auto* next = current->get_next();

		while (current && current->value_ >= level)
		{
			gal_assert(not current->is_closed());

			// found a corresponding upvalue?
			if (current->value_ == level)
			{
				// is it dead?
				if (state.check_is_dead(*current))
				{
					// resurrect it
					current->set_mark_another_white();
					return current;
				}
			}

			if (next)
			{
				current = dynamic_cast<object_upvalue*>(next);
				gal_assert(current);
				next = current->get_next();
			}
			else { current = nullptr; }
		}

		auto* ret = CREATE_OBJECT(
				object_upvalue,
				state,
				// current value lives in the stack
				level,
				nullptr)		;

		ret->set_mark(state.get_white());
		// chain it in the proper position
		ret->link_next(current);

		// double link it in `link` list
		link.link_behind(*ret);

		return ret;
	}

	object_closure::object_closure(main_state& state, const size_type num_elements, object_table* environment, object_prototype& prototype)
		: object{object_type::function},
		  is_internal_{0},
		  stack_size_{prototype.get_stack_capacity()},
		  is_preload_{0},
		  gc_list_{nullptr},
		  environment_{environment},
		  function_{num_elements}
	// function_{.gal = {
	// 		  .prototype = &prototype,
	// 		  .upreferences = {num_elements, magic_value_null, {state}
	// 		  }}}
	{
		state.link_object(*this);
	}

	object_closure::object_closure(
			main_state& state,
			size_type num_elements,
			object_table* environment,
			internal_function_type function,
			continuation_function_type continuation,
			string_type debug_name)
		: object{object_type::function},
		  is_internal_{1},
		  stack_size_{min_stack_size},
		  is_preload_{0},
		  gc_list_{nullptr},
		  environment_{environment},
		  function_{.num = num_elements}
	// function_{.internal = {
	//   .function = function,
	//   .continuation = continuation,
	//   .debug_name = debug_name,
	//   .upvalues = internal_type::upvalue_container_type{internal_type::upvalue_container_type::allocator_type{state}}
	// }}
	{
		(void)function;
		(void)continuation;
		(void)debug_name;
		// function_.internal.upvalues.reserve(num_elements);
		state.link_object(*this);
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

	std::size_t object_table::clear_dead_node()
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

		return work;
	}

	bool magic_value::raw_equal(const magic_value other) const noexcept
	{
		if (is_null()) { return other.is_null(); }
		if (is_boolean()) { return as_boolean() == other.as_boolean(); }
		if (is_number())
		{
			constexpr auto float_eq = [](const number_type a, const number_type b) constexpr noexcept { return a - b < std::numeric_limits<number_type>::epsilon() && b - a < std::numeric_limits<number_type>::epsilon(); };

			return float_eq(as_number(), other.as_number());
		}

		gal_assert(is_object() && other.is_object());
		return as_object() == other.as_object();
	}

	bool magic_value::equal(child_state& state, const magic_value other) const
	{
		if (not is_object() || not(is_user_data() && other.is_user_data()) || not(is_table() && other.is_table())) { return raw_equal(other); }

		return state.meta_method_compare(*this, other, meta_method_type::equal).as_boolean();
	}

	namespace
	{
		template<bool HasEqual>
		bool magic_value_compare(child_state& state, const magic_value lhs, const magic_value rhs)
		{
			if (lhs.get_type() != rhs.get_type()) { state.error_order(lhs, rhs, meta_method_type::less_than); }

			if (lhs.is_number())
			{
				if constexpr (HasEqual) { return lhs.as_number() <= rhs.as_number(); }
				else { return lhs.as_number() < rhs.as_number(); }
			}

			if (lhs.is_string()) { return *lhs.as_string() < *rhs.as_string(); }

			if (auto result = state.meta_method_order(lhs, rhs, meta_method_type::less_than);
				result != magic_value_undefined) { return result.as_boolean(); }
			else if constexpr (HasEqual)
			{
				if (result = state.meta_method_order(lhs, rhs, meta_method_type::less_equal);
					result != magic_value_undefined) { return result.as_boolean(); }
			}

			state.error_order(lhs, rhs, meta_method_type::less_equal);
		}
	}

	bool magic_value::less_than(child_state& state, const magic_value other) const { return magic_value_compare<false>(state, *this, other); }

	bool magic_value::less_equal(child_state& state, const magic_value other) const { return magic_value_compare<true>(state, *this, other); }

	number_type magic_value::to_number() const noexcept
	{
		if (is_number()) { return as_number(); }

		if (is_string())
		{
			const auto& data = as_string()->get_data();
			number_type num;
			const auto [ptr, err] = std::from_chars(data.data(), data.data() + data.size(), num);
			if (ptr == data.data() + data.size() && err == std::errc{}) { return num; }
		}
		return magic_value_null.as_number();
	}

	object_string* magic_value::to_string(main_state& state) const
	{
		if (is_string()) { return as_string(); }

		if (is_number())
		{
			constexpr auto log10_ceil = utils::y_combinator{
					[]<std::integral T>(auto&& self, const T num) constexpr noexcept -> int { return num < 10 ? 1 : 1 + self(num / 10); }};

			constexpr auto length =
					5 +
					std::numeric_limits<number_type>::max_digits10 +
					std::ranges::max(2, log10_ceil(std::numeric_limits<number_type>::max_exponent10));

			// object_string::data_type str{length, 0, {state}};
			object_string::data_type str{{state}};
			str.resize(length);
			std::to_chars(str.data(), str.data() + str.size(), as_number(), std::chars_format::scientific);
			return CREATE_OBJECT(
					object_string,
					state,
					state,
					std::move(str))			;
		}

		return nullptr;
	}
}
