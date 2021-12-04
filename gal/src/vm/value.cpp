#include <algorithm>
#include <cstdarg>
#include <utils/assert.hpp>
#include <utils/utils.hpp>
#include <vm/value.hpp>
#include <vm/vm.hpp>

namespace gal
{
	object::~object() noexcept = default;

	object::object(gal_virtual_machine_state& state, object_type type, object_class* object_class) noexcept
		: type_{type},
		  dark_{false},
		  object_class_{object_class}
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
		string_ = std_format::format("{:<-.14f}", value);
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
}// namespace gal
