#include <algorithm>
#include <cstdarg>
#include <utility>
#include <utils/assert.hpp>
#include <utils/utils.hpp>
#include <vm/compiler.hpp>
#include <vm/debugger.hpp>
#include <vm/value.hpp>
#include <vm/vm.hpp>

namespace gal
{
	object::~object() noexcept = default;

	object::object(object_type type, std::shared_ptr<object_class> object_class) noexcept
		: type_{type},
		  object_class_{std::move(object_class)}
	{
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

	object_string::object_string(gal_virtual_machine_state& state)
		: object{object_type::STRING_TYPE, state.string_class_}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, size_type length, char c)
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_(length, c)
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, const char* text, object_string::size_type length)
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_{text, length}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, string_type string)
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_{std::move(string)}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, std_string_type string)
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_{std::move(string)}
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, const object_string& source, object_string::size_type begin, object_string::size_type count, object_string::size_type step)
		: object{object_type::STRING_TYPE, state.string_class_}
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
		: object{object_type::STRING_TYPE, state.string_class_}
	{
		// todo: different allocator?
		// string_ = std_format::format("{:<-.14f}", value);

		std_format::format_to(get_appender(), "{:<-.14f}", value);
	}

	object_string::object_string(gal_virtual_machine_state& state, const char* format, ...)
		: object{object_type::STRING_TYPE, state.string_class_}
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
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_(static_cast<size_type>(utf8_encode_num_bytes(value)), 0)
	{
		utf8_encode(value, reinterpret_cast<std::uint8_t*>(data()));
	}

	object_string::object_string(gal_virtual_machine_state& state, std::uint8_t value)
		: object{object_type::STRING_TYPE, state.string_class_},
		  string_(static_cast<size_type>(1), static_cast<char>(value))
	{
	}

	object_string::object_string(gal_virtual_machine_state& state, object_string& string, object_string::size_type index)
		: object{object_type::STRING_TYPE, state.string_class_}
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

	object_function::object_function(gal_virtual_machine_state& state, object_module& module, gal_slot_type max_slots)
		: object{object_type::FUNCTION_TYPE, state.function_class_},
		  module_{module},
		  max_slots_{max_slots},
		  num_upvalues_{0},
		  arity_{0}
	{
	}

	object_closure::object_closure(gal_virtual_machine_state& state, object_function& function)
		: object{object_type::CLOSURE_TYPE, state.function_class_},
		  function_{function}
	{
		upvalues_.reserve(function.get_upvalues_size());
	}

	object_fiber::object_fiber(gal_virtual_machine_state& state, object_closure* closure)
		: object{object_type::FIBER_TYPE, state.fiber_class_},
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

	object_upvalue& object_fiber::capature_upvalue(magic_value& local)
	{
		// If there are no open upvalues at all, we must need a new one.
		if (open_upvalues_.empty())
		{
			return open_upvalues_.emplace_front(local);
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
		return *open_upvalues_.emplace_after(prev_upvalue, local);
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

		auto meta_class			  = std::make_shared<object_class>(0, std::move(meta_class_name));
		meta_class->object_class_ = state.class_class_;

		// Meta-classes always inherit Class and do not parallel the non-metaclass
		// hierarchy.
		meta_class->bind_super_class(*state.class_class_);

		auto ret		   = std::make_shared<object_class>(num_fields, name);
		ret->object_class_ = std::move(meta_class);
		ret->bind_super_class(*this);

		return ret;
	}

	object_list::object_list(gal_virtual_machine_state& state)
		: object{object_type::LIST_TYPE, state.list_class_},
		  elements_{}
	{
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

	gal_index_type object_module::define_variable(const object_string& name, magic_value value, int* line)
	{
		if (variables_.size() > max_module_variables)
		{
			return variable_too_many_defined;
		}

		// See if the variable is already explicitly or implicitly declared.
		auto it = variables_.find(name);

		if (it == variables_.end())
		{
			// Brand-new variable.
			return static_cast<gal_index_type>(std::distance(variables_.begin(), variables_.emplace(name, value).first));
		}
		else if (auto& v = it->second; v.is_number())
		{
			// An implicitly declared variable's value will always be a number.
			// Now we have a real definition.
			if (line)
			{
				*line = static_cast<int>(v.as_number());
			}
			v = value;

			// If this was a local-name we want to error if it was
			// referenced before this definition.
			if (is_local_name(name))
			{
				return variable_used_before_defined;
			}
			return static_cast<gal_index_type>(std::distance(variables_.begin(), it));
		}
		else
		{
			// Already explicitly declared.
			return variable_already_defined;
		}
	}

	object_map::object_map(gal_virtual_machine_state& state)
		: object{object_type::MAP_TYPE, state.map_class_},
		  entries_{}
	{
	}

}// namespace gal
