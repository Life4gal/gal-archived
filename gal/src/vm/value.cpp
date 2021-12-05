#include <algorithm>
#include <cstdarg>
#include <utility>
#include <utils/assert.hpp>
#include <utils/utils.hpp>
#include <vm/debugger.hpp>
#include <vm/value.hpp>
#include <vm/vm.hpp>

namespace gal
{
	object::~object() noexcept = default;

	object::object(gal_virtual_machine_state& state, object_type type, std::shared_ptr<object_class> object_class) noexcept
		: type_{type},
		  dark_{false},
		  object_class_{std::move(object_class)}
	{
		state.objects_.push_front(this);
	}

	void object::gray(gal_virtual_machine_state& state)
	{
		// Stop if the object is already darkened, so we don't get stuck in a cycle.
		if (dark_)
		{
			return;
		}

		// It's been reached.
		dark_ = true;

		state.gray_.push(this);
	}

	void object::blacken_all(gal_virtual_machine_state& state)
	{
		while (not state.gray_.empty())
		{
			// Pop an item from the gray stack.
			auto* obj = state.gray_.top();
			state.gray_.pop();
			obj->blacken(state);
		}
	}

	void magic_value::gray(gal_virtual_machine_state& state) const
	{
		if (not is_object())
		{
			return;
		}
		as_object()->gray(state);
	}

	bool magic_value::equal(const magic_value& other) const
	{
		if (*this == other)
		{
			return true;
		}

		// If we get here, it's only possible for two heap-allocated immutable objects
		// to be equal.
		if (not is_object() || not other.is_object())
		{
			return false;
		}

		auto* lhs = as_object();
		auto* rhs = other.as_object();

		// Must be the same type.
		if (lhs->type() != rhs->type())
		{
			return false;
		}

		switch (lhs->type())
		{
			case object_type::STRING_TYPE:
			{
				return dynamic_cast<object_string&>(*lhs) == dynamic_cast<object_string&>(*rhs);
			}
			default:
			{
				// All other types are only equal if they are same, which they aren't if
				// we get here.
				return false;
			}
		}
	}

	void magic_value_buffer::gray(gal_virtual_machine_state& state)
	{
		std::ranges::for_each(buffer_, [&](value_type& value)
							  { value.gray(state); });
	}

	object_string::object_string(gal_virtual_machine_state& state)
		: object{state, object_type::STRING_TYPE, state.string_class_}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, size_type length, char c)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_(length, c)
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, const char* text, object_string::size_type length)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_{text, length}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, string_type string)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_{std::move(string)}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, std_string_type string)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_{std::move(string)}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, const object_string& source, object_string::size_type begin, object_string::size_type count, object_string::size_type step)
		: object{state, object_type::STRING_TYPE, state.string_class_}
	{
		auto*	  from	 = reinterpret_cast<const std::uint8_t*>(source.data());
		size_type length = 0;
		for (size_type i = 0; i < count; ++i)
		{
			length += utf8_decode_num_bytes(from[begin + i * step]);
		}

		string_.resize(length);

		auto* to = reinterpret_cast<std::uint8_t*>(data());
		for (size_type i = 0; i < count; ++i)
		{
			auto index		= begin + i * step;
			auto code_point = utf8_decode(from + index, source.size() - index);

			if (code_point != -1)
			{
				to += utf8_encode(code_point, to);
			}
		}
	}

	object_string::object_string(gal_virtual_machine_state& state, double value)
		: object{state, object_type::STRING_TYPE, state.string_class_}
	{
		// todo: different allocator?
		// string_ = std_format::format("{:<-.14f}", value);

		std_format::format_to(get_appender(), "{:<-.14f}", value);
	}

	object_string::object_string(gal_virtual_machine_state& state, const char* format, ...)
		: object{state, object_type::STRING_TYPE, state.string_class_}
	{
		va_list arg_list;
		va_start(arg_list, format);
		for (auto* c = format; *c != '\0'; ++c)
		{
			switch (*c)
			{
				case '$':
				{
					auto* str = va_arg(arg_list, const char*);
					append(str);
					break;
				}
				case '@':
				{
					auto* str = va_arg(arg_list, magic_value).as_string();
					append(*str);
					break;
				}
				default:
				{
					// Any other character is interpreted literally.
					push_back(*c);
				}
			}
		}
		va_end(arg_list);
	}

	object_string::object_string(gal_virtual_machine_state& state, int value)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_(static_cast<size_type>(utf8_encode_num_bytes(value)), 0)
	{
		utf8_encode(value, reinterpret_cast<std::uint8_t*>(data()));
	}

	object_string::object_string(gal_virtual_machine_state& state, std::uint8_t value)
		: object{state, object_type::STRING_TYPE, state.string_class_},
		  string_(static_cast<size_type>(1), static_cast<char>(value))
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, object_string& string, object_string::size_type index)
		: object{state, object_type::STRING_TYPE, state.string_class_}
	{
		gal_assert(index < string.size(), "Index out of bounds.");

		if (const auto code_point = utf8_decode(reinterpret_cast<const std::uint8_t*>(string.data() + index), string.size() - index); code_point == -1)
		{
			// If it isn't a valid UTF-8 sequence, treat it as a single raw byte.
			push_back(string.string_[index]);
		}
		else
		{
			string.string_.reserve(utf8_encode_num_bytes(code_point));
			utf8_encode(code_point, reinterpret_cast<std::uint8_t*>(data()));
		}
	}

	object_string::size_type object_string::find(object_string& needle, size_type start)
	{
		// Uses the Boyer-Moore-Horspool string matching algorithm.

		// Edge case: An empty needle is always found.
		if (needle.size() == 0)
		{
			return start;
		}

		// If the needle goes past the [this] it won't be found.
		if (start + needle.size() > size())
		{
			return npos;
		}

		// If the start is too far it also won't be found.
		if (start >= size())
		{
			return npos;
		}

		// Pre-calculate the shift table. For each character (8-bit value), we
		// determine how far the search window can be advanced if that character is
		// the last character in the [this] where we are searching for the [needle]
		// and the [needle] doesn't match there.
		size_type shift[std::numeric_limits<std::uint8_t>::max()];
		size_type needle_end = needle.size() - 1;

		// By default, we assume the character is not the [needle] at all. In that
		// case, if a match fails on that character, we can advance one whole [needle]
		// width since.
		std::fill(std::begin(shift), std::end(shift), needle.size());

		// Then, for every character in the [needle], determine how far it is from the
		// end. If a match fails on that character, we can advance the window such
		// that is the last character in it lines up with the last place we could
		// find it in the [needle].
		for (size_type i = 0; i < needle_end; ++i)
		{
			auto c								= needle[i];
			shift[static_cast<std::uint8_t>(c)] = needle_end - i;
		}

		// Slide the [needle] across the haystack, looking for the first match or
		// stopping if the [needle] goes off the end.
		auto last_char = needle[needle_end];
		auto range	   = size() - needle.size();

		for (size_type i = start; i <= range;)
		{
			// Compare the last character in the [this]'s window to the last character
			// in the [needle]. If it matches, see if the whole [needle] matches.
			auto c = this->operator[](i + needle_end);
			if (last_char == c && std::memcmp(data() + i, needle.data(), needle_end) == 0)
			{
				// Found a match.
				return i;
			}

			// Otherwise, slide the needle forward.
			i += shift[static_cast<std::uint8_t>(c)];
		}

		// Not found.
		return npos;
	}

	void object_string::blacken(gal_virtual_machine_state& state)
	{
		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_string);
	}

	gal_index_type symbol_table::find(const char* name, object_string::size_type length)
	{
		const auto it = std::ranges::find_if(
				table_,
				[&](const value_type& str)
				{ return str.equal(length, name); });
		if (it != table_.end())
		{
			return std::distance(table_.begin(), it);
		}
		return gal_index_not_exist;
	}

	gal_index_type symbol_table::find(const object_string& string)
	{
		const auto it = std::ranges::find(
				table_,
				string);
		if (it != table_.end())
		{
			return std::distance(table_.begin(), it);
		}
		return gal_index_not_exist;
	}

	void symbol_table::blacken(gal_virtual_machine_state& state)
	{
		std::ranges::for_each(table_, [&](value_type& str)
							  { str.gray(state); });
	}

	void object_upvalue::blacken(gal_virtual_machine_state& state)
	{
		// Mark the closed-over object (in case it is closed).
		closed_.gray(state);

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_upvalue);
	}

	void object_module::blacken(gal_virtual_machine_state& state)
	{
		// Top-level variables.
		variables_.gray(state);

		variable_names_.blacken(state);

		name_.gray(state);

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_module);
	}

	object_function::object_function(gal_virtual_machine_state& state, object_module& module, gal_slot_type max_slots)
		: object{state, object_type::FUNCTION_TYPE, state.function_class_},
		  module_{module},
		  max_slots_{max_slots},
		  num_upvalues_{0},
		  arity_{0}
	{
	}

	void object_function::blacken(gal_virtual_machine_state& state)
	{
		// Mark the constants.
		constants_.gray(state);

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_function);
		state.bytes_allocated_ += sizeof(code_buffer_type::value_type) * code_.capacity();
		state.bytes_allocated_ += constants_.memory_usage();

		// The debug line number buffer.
		state.bytes_allocated_ += debug_.memory_usage();
	}

	object_closure::object_closure(gal_virtual_machine_state& state, object_function&& function)
		: object{state, object_type::CLOSURE_TYPE, state.function_class_},
		  function_{std::move(function)}
	{
		upvalues_.reserve(function.get_upvalues_size());
	}

	void object_closure::blacken(gal_virtual_machine_state& state)
	{
		// Mark the function.
		function_.gray(state);

		// Mark the upvalues.
		std::ranges::for_each(upvalues_, [&](upvalue_type& upvalue)
							  { upvalue.get().gray(state); });

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_closure);
		state.bytes_allocated_ += sizeof(upvalue_type) * upvalues_.capacity();
	}

	object_fiber::object_fiber(gal_virtual_machine_state& state, object_closure* closure)
		: object{state, object_type::FIBER_TYPE, state.fiber_class_},
		  // Add one slot for the unused implicit receiver slot that the compiler assumes all functions have.
		  stack_{std::make_unique<magic_value[]>(closure ? bit_ceil(closure->get_function().get_slots_size() + 1) : 1)},
		  stack_top_{stack_.get()},
		  stack_capacity_{closure ? bit_ceil(closure->get_function().get_slots_size() + 1) : 1},
		  frames_{initial_call_frames},
		  open_upvalues_{},
		  caller_{nullptr},
		  error_message_{nullptr},
		  state_{fiber_state::other_state}
	{
		if (closure)
		{
			// Initialize the first call frame.
			add_call_frame(*closure, stack_[0]);

			// The first slot always holds the closure.
			stack_top_[0] = closure->operator magic_value();
			++stack_top_;
		}
	}

	void object_fiber::add_call_frame(object_closure& closure, magic_value& stack_start)
	{
		call_frame a{closure.get_function().get_code_data(), &closure, &stack_start};
		frames_.push_back(a);

		// frames_.emplace_back(closure.get_function().get_code_data(), &closure, &stack_start);
	}

	void object_fiber::ensure_stack(gal_virtual_machine_state& state, gal_size_type needed)
	{
		if (stack_capacity_ >= needed) return;

		const auto new_capacity = bit_ceil(needed);

		const auto old_stack	= std::move(stack_);

		stack_					= std::make_unique<magic_value[]>(new_capacity);
		stack_capacity_			= new_capacity;

		/**
		 * @brief we need to recalculate every pointer that points into the old stack
		 * to into the same relative distance in the new stack. We have to be a little
		 * careful about how these are calculated because pointer subtraction is only
		 * well-defined within a single array, hence the slightly redundant-looking
		 * arithmetic below.
		 */

		// Top of the stack.
		if (const auto* bottom = state.get_stack_bottom(); bottom >= old_stack.get() && bottom <= stack_top_)
		{
			state.set_stack_bottom(stack_.get() + (bottom - old_stack.get()));
		}

		// Stack pointer for each call frame.
		for (auto& frame: frames_)
		{
			frame.stack_start = stack_.get() + (frame.stack_start - old_stack.get());
		}

		// Open upvalues.
		for (auto& upvalue: open_upvalues_)
		{
			upvalue.reset_value(stack_.get() + (upvalue.get_value() - old_stack.get()));
		}

		stack_top_ = stack_.get() + (stack_top_ - old_stack.get());

		// old_stack discard in here
	}

	object_upvalue& object_fiber::capature_upvalue(gal_virtual_machine_state& state, magic_value& local)
	{
		// If there are no open upvalues at all, we must need a new one.
		if (open_upvalues_.empty())
		{
			return open_upvalues_.emplace_front(state, local);
		}

		// Walk towards the bottom of the stack until we find a previously existing
		// upvalue or pass where it should be.
		auto prev_upvalue = open_upvalues_.before_begin();
		auto upvalue	  = open_upvalues_.begin();

		// Walk towards the top of the stack until we find a previously existing
		// upvalue or pass where it should be.
		while (upvalue != open_upvalues_.end() && upvalue->get_value() < &local)
		{
			prev_upvalue = upvalue;
			++upvalue;
		}

		// Found an existing upvalue for this local.
		if (upvalue != open_upvalues_.end() && upvalue->get_value() == &local)
		{
			return *upvalue;
		}

		// We've walked past this local on the stack, so there must not be an
		// upvalue for it already. Make a new one and link it in the right
		// place to keep the list sorted.
		return *open_upvalues_.emplace_after(prev_upvalue, state, local);
	}

	void object_fiber::close_upvalue(magic_value& last)
	{
		if (not open_upvalues_.empty())
		{
			auto prev	 = open_upvalues_.before_begin();
			auto current = open_upvalues_.begin();
			for (
					;
					current != open_upvalues_.end() && current->get_value() < &last;
					++current, ++prev)
			{
			}

			for (auto i = current; i != open_upvalues_.end(); ++i)
			{
				// Move the value into the upvalue itself and point the upvalue to it.
				i->close();
			}

			open_upvalues_.erase_after(prev, open_upvalues_.end());
		}
	}

	object_fiber* object_fiber::raise_error()
	{
		gal_assert(has_error(), "Should only call this after an error.");

		auto* current = this;
		auto  error	  = error_message_;

		do {
			// Every fiber along the call chain gets aborted with the same error.
			current->set_error(error);

			// If the caller ran this fiber using "try", give it the error and stop.
			if (current->state_ == fiber_state::try_state)
			{
				// Make the caller's try method return the error message.
				current->caller_->set_stack_point(1, error->operator magic_value());
				return current->caller_;
			}

			// Otherwise, unhook the caller since we will never resume and return to it.
			auto* caller	 = current->caller_;
			current->caller_ = nullptr;
			current			 = caller;
		} while (current);

		return nullptr;
	}

	void object_fiber::blacken(gal_virtual_machine_state& state)
	{
		// Stack functions.
		for (auto& frame: frames_)
		{
			frame.closure->gray(state);
		}

		// Stack variables.
		for (auto* slot = stack_.get(); slot < stack_top_; ++slot)
		{
			slot->gray(state);
		}

		// Open upvalues.
		for (auto& upvalue: open_upvalues_)
		{
			upvalue.gray(state);
		}

		// The caller.
		gal_assert(caller_, "Caller should not be nullptr.");
		caller_->gray(state);
		error_message_.gray(state);

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_fiber);
		state.bytes_allocated_ += sizeof(frames_buffer_value_type) * frames_.capacity();
		state.bytes_allocated_ += sizeof(magic_value) * stack_capacity_;
	}

	void object_class::bind_super_class(object_class& superclass)
	{
		superclass_ = &superclass;

		// Include the superclass in the total number of fields.
		if (not is_outer_class())
		{
			num_fields_ += superclass.num_fields_;
		}
		else
		{
			gal_assert(superclass.is_interface_class(), "A outer class cannot inherit from a class with fields.");
		}

		// Inherit methods from its superclass.
		// todo: Do we need to support multiple inheritance?
		methods_ = superclass_->methods_;
	}

	std::shared_ptr<object_class> object_class::create_derived_class(gal_virtual_machine_state& state, gal_size_type num_fields, object_string& name)
	{
		// Create the metaclass.
		auto meta_class_name = name;
		meta_class_name.append("@metaclass@");

		auto meta_class			  = std::make_shared<object_class>(state, 0, std::move(meta_class_name));
		meta_class->object_class_ = state.class_class_;

		// Meta-classes always inherit Class and do not parallel the non-metaclass
		// hierarchy.
		meta_class->bind_super_class(*state.class_class_);

		auto ret		   = std::make_shared<object_class>(state, num_fields, name);
		ret->object_class_ = std::move(meta_class);
		ret->bind_super_class(*this);

		return ret;
	}

	void object_class::blacken(gal_virtual_machine_state& state)
	{
		// The metaclass.
		object_class_->gray(state);

		// The superclass.
		superclass_->gray(state);

		// Method function objects.
		for (auto& m: methods_)
		{
			if (m.type == method_type::block_type)
			{
				m.as.closure->gray(state);
			}
		}

		name_.gray(state);

		if (not attributes_.is_null())
		{
			attributes_.as_object()->gray(state);
		}

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_class);
		state.bytes_allocated_ += sizeof(method_buffer_value_type) * methods_.capacity();
	}

	void object_instance::blacken(gal_virtual_machine_state& state)
	{
		object_class_->gray(state);

		// Mark the fields.
		for (auto& field: fields_)
		{
			field.gray(state);
		}

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_instance);
		state.bytes_allocated_ += sizeof(field_buffer_value_type) * fields_.capacity();
	}

	object_list::object_list(gal_virtual_machine_state& state)
		: object{state, object_type::LIST_TYPE, state.list_class_},
		  elements_{}
	{
	}

	void object_list::insert(gal_virtual_machine_state& state, magic_value value, list_buffer_size_type index)
	{
		if (value.is_object())
		{
			state.push_root(*value.as_object());
		}

		// Store the new element.
		elements_.insert(std::next(elements_.begin(), static_cast<list_buffer_difference_type>(index)), value);

		if (value.is_object())
		{
			state.pop_root();
		}
	}

	magic_value object_list::remove(gal_virtual_machine_state& state, list_buffer_size_type index)
	{
		auto removed = elements_[index];

		if (removed.is_object())
		{
			state.push_root(*removed.as_object());
		}

		elements_.erase(std::next(elements_.begin(), static_cast<list_buffer_difference_type>(index)));

		if (removed.is_object())
		{
			state.pop_root();
		}

		return removed;
	}

	gal_index_type object_list::index_of(magic_value value) const
	{
		const auto it = std::ranges::find(elements_, value);
		if (it != elements_.end())
		{
			return std::distance(elements_.begin(), it);
		}
		return gal_index_not_exist;
	}

	void object_list::blacken(gal_virtual_machine_state& state)
	{
		// Mark the elements.
		for (auto& value: elements_)
		{
			value.gray(state);
		}

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_list);
		state.bytes_allocated_ += sizeof(list_buffer_value_type) * elements_.capacity();
	}

	object_map::object_map(gal_virtual_machine_state& state)
		: object{state, object_type::MAP_TYPE, state.map_class_},
		  entries_{}
	{
	}

	magic_value object_map::remove(gal_virtual_machine_state& state, magic_value key)
	{
		auto it = find(key);
		if (it == entries_.end())
		{
			return magic_value_null;
		}

		auto value = it->second;

		if (value.is_object())
		{
			state.push_root(*value.as_object());
		}

		entries_.erase(it);

		if (value.is_object())
		{
			state.pop_root();
		}
		return value;
	}

	void object_map::blacken(gal_virtual_machine_state& state)
	{
		// Mark the entries.
		for (auto& [k, v]: entries_)
		{
			k.gray(state);
			v.gray(state);
		}

		// Keep track of how much memory is still in use.
		state.bytes_allocated_ += sizeof(object_map);
		state.bytes_allocated_ += sizeof(value_type) * entries_.size();
	}
}// namespace gal
