#pragma once

#ifndef GAL_LANG_VALUE_HPP
	#define GAL_LANG_VALUE_HPP

	#include <gal.hpp>
	#include <allocator.hpp>
	#include <vm/common.hpp>
	#include <utils/utils.hpp>

	#include <limits>
	#include <vector>
	#include <string>
	#include <unordered_map>

/**
 * @brief
 * This defines the built-in types and their core representations in memory.
 * Since GAL is dynamically typed, any variable can hold a value of any type,
 * and the type can change at runtime. Implementing this efficiently is
 * critical for performance.
 *
 * The main type exposed by this is [magic_value]. A C++ variable of that type is a
 * storage location that can hold any GAL value. The stack, module variables,
 * and instance fields are all implemented in C++ as variables of type [magic_value].
 * The built-in types for booleans, numbers, and null are unboxed: their value
 * is stored directly in the [magic_value], and copying a [magic_value] copies the
 * value. Other types--classes, instances of classes, functions, lists, and strings--are
 * all reference types. They are stored on the heap and the [magic_value] just stores a
 * pointer to it. Copying the [magic_value] copies a reference to the same object. The
 * GAL implementation calls these "object", or objects, though to a user, all values
 * are objects.
 *
 * There is also a special singleton value "undefined". It is used internally
 * but never appears as a real value to a user. It has two uses:
 *
 * - It is used to identify module variables that have been implicitly declared
 *   by use in a forward reference but not yet explicitly declared. These only
 *   exist during compilation and do not appear at runtime.
 *
 * - It is used to represent unused map entries in an object_map.
 */

namespace gal
{
	enum class object_type
	{
		STRING_TYPE,
		UPVALUE_TYPE,
		MODULE_TYPE,
		FUNCTION_TYPE,
		CLOSURE_TYPE,
		FIBER_TYPE,
		CLASS_TYPE,
		OUTER_TYPE,
		INSTANCE_TYPE,
		LIST_TYPE,
		MAP_TYPE,
	};

	class magic_value;
	class object_string;
	class object_upvalue;
	class object_module;
	class object_function;
	class object_closure;
	class object_fiber;
	class object_class;
	class object_outer;
	class object_instance;
	class object_list;
	class object_map;

	/**
	 * @brief Base struct for all heap-allocated objects.
	 */
	class object
	{
	private:
		object_type	  type_;
		bool		  dark_;

		/**
		 * @brief The object's class.
		 */
		object_class* object_class_;

	public:
		virtual ~object() noexcept = 0;

		constexpr object(object_type type, object_class* object_class) noexcept
			: type_(type),
			  dark_(false),
			  object_class_(object_class) {}

		[[nodiscard]] constexpr object_type type() const noexcept { return type_; }

		/**
		 * @brief Mark [this] as reachable and still in use. This should only be called
		 * during the sweep phase of a garbage collection.
		 */
		void								gray(gal_virtual_machine_state& state);

		/**
		 * @brief Releases all memory owned by [object], including [object] itself.
		 */
		void								free(gal_virtual_machine_state& state)
		{
			destroy(state);
		}

		/**
		 * @brief Processes every object in the gray stack until all reachable objects have
		 * been marked. After that, all objects are either white (free-able) or black
		 * (in use and fully traversed).
		  *
		  * todo: move it to state
		 */
		static void blacken_all(gal_virtual_machine_state& state);

		explicit	operator magic_value() const noexcept;

	private:
		virtual void blacken(gal_virtual_machine_state& state) = 0;
		virtual void destroy(gal_virtual_machine_state& state) = 0;
	};

	/**
	 * @brief
	 * An IEEE 754 double-precision float is a 64-bit value with bits laid out like:
	 *
	 * 1 Sign bit
	 * | 11 Exponent bits
	 * | |          52 Mantissa (i.e. fraction) bits
	 * | |          |
	 * S[Exponent-][Mantissa------------------------------------------]
	 *
	 * The details of how these are used to represent numbers aren't really
	 * relevant here as long we don't interfere with them. The important bit is NaN.
	 *
	 * An IEEE double can represent a few magical values like NaN ("not a number"),
	 * Infinity, and -Infinity. A NaN is any value where all exponent bits are set:
	 *
	 *  v--NaN bits
	 * -11111111111----------------------------------------------------
	 *
	 * Here, "-" means "doesn't matter". Any bit sequence that matches the above is
	 * a NaN. With all of those "-", it obvious there are a *lot* of different
	 * bit patterns that all mean the same thing. NaN tagging takes advantage of
	 * this. We'll use those available bit patterns to represent things other than
	 * numbers without giving up any valid numeric values.
	 *
	 * NaN values come in two flavors: "signalling" and "quiet". The former are
	 * intended to halt execution, while the latter just flow through arithmetic
	 * operations silently. We want the latter. Quiet NaNs are indicated by setting
	 * the highest mantissa bit:
	 *
	 *             v--Highest mantissa bit
	 * -[NaN      ]1---------------------------------------------------
	 *
	 * If all of the NaN bits are set, it's not a number. Otherwise, it is.
	 * That leaves all the remaining bits as available for us to play with. We
	 * stuff a few different kinds of things here: special singleton values like
	 * "true", "false", and "null", and pointers to objects allocated on the heap.
	 * We'll use the sign bit to distinguish singleton values from pointers. If
	 * it's set, it's a pointer.
	 *
	 * v--Pointer or singleton?
	 * S[NaN      ]1---------------------------------------------------
	 *
	 * For singleton values, we just enumerate the different values. We'll use the
	 * low bits of the mantissa for that, and only need a few:
	 *
	 *                                                 3 Type bits--v
	 * 0[NaN      ]1------------------------------------------------[T]
	 *
	 * For pointers, we are left with 51 bits of mantissa to store an address.
	 * That's more than enough room for a 32-bit address. Even 64-bit machines
	 * only actually use 48 bits for addresses, so we've got plenty. We just stuff
	 * the address right into the mantissa.
	 *
	 * double precision numbers, pointers, and a bunch of singleton values,
	 * all stuffed into a single 64-bit sequence. Even better, we don't have to
	 * do any masking or work to extract number values: they are unmodified. This
	 * means math on numbers is fast.
	 */

	static_assert(std::numeric_limits<double>::is_iec559);
	class magic_value
	{
	public:
		using value_type = std::uint64_t;

		/**
		 * @brief Masks out the tag bits used to identify the singleton value.
		 */
		constexpr static value_type tag_mask{(1 << 3) - 1};// 7

		/**
		 * @brief Tag values for the different singleton values.
		 */
		constexpr static value_type tag_nan{0};
		constexpr static value_type tag_null{1};
		constexpr static value_type tag_false{2};
		constexpr static value_type tag_true{3};
		constexpr static value_type tag_undefined{4};
		constexpr static value_type tag_reserve1{5};
		constexpr static value_type tag_reserve2{6};
		constexpr static value_type tag_reserve3{7};

		/**
		 * @brief A mask that selects the sign bit.
		 */
		constexpr static value_type sign_bit{value_type{1} << 63};

		/**
		 * @brief The bits that must be set to indicate a quiet NaN.
		 *
		 * note:
		 *  it's: 0 111 1111 1111 1100 000000000000000000000000000000000000000000000000
		 *  not : 0 111 1111 1111 1000 000000000000000000000000000000000000000000000000
		 */
		constexpr static value_type quiet_nan{0x7ffc000000000000};

		constexpr static value_type pointer_mask{quiet_nan | sign_bit};

		/**
		 * @brief Singleton values.
		 */
		constexpr static value_type null_val{quiet_nan | tag_nan};
		constexpr static value_type false_val{quiet_nan | tag_false};
		constexpr static value_type true_val{quiet_nan | tag_true};
		constexpr static value_type undefined_val{quiet_nan | tag_undefined};

	private:
		value_type data_;

	public:
		constexpr explicit magic_value() noexcept : magic_value(null_val) {}
		constexpr explicit magic_value(value_type d) noexcept : data_(d) {}
		constexpr explicit magic_value(bool b) noexcept : data_(b ? true_val : false_val) {}
		constexpr explicit magic_value(double d) noexcept : magic_value(double_to_bits(d)) {}

		/**
		 * @brief Gets the singleton type tag for a magic_value (which must be a singleton).
		 */
		[[nodiscard]] constexpr value_type	  get_tag() const noexcept { return data_ & tag_mask; }

		/**
		 * @brief If the NaN bits are set, it's not a number.
		 */
		[[nodiscard]] constexpr bool		  is_number() const noexcept { return (data_ & quiet_nan) != quiet_nan; }

		/**
		 * @brief An object pointer is a NaN with a set sign bit.
		 */
		[[nodiscard]] constexpr bool		  is_object() const noexcept { return (data_ & pointer_mask) == pointer_mask; }

		[[nodiscard]] constexpr bool		  is_true() const noexcept { return data_ == true_val; }
		[[nodiscard]] constexpr bool		  is_false() const noexcept { return data_ == false_val; }

		[[nodiscard]] constexpr bool		  is_null() const noexcept { return data_ == null_val; }
		[[nodiscard]] constexpr bool		  is_undefined() const noexcept { return data_ == undefined_val; }
		[[nodiscard]] constexpr bool		  is_falsy() const noexcept { return is_false() || is_null(); }

		/**
		 * @brief Value -> 0 or 1.
		 */
		[[nodiscard]] constexpr bool		  as_boolean() const noexcept { return data_ == true_val; }

		/**
		 * @brief Value -> object*.
		 */
		[[nodiscard]] inline object*		  as_object() const noexcept;

		/**
		 * @brief Returns true if [value] is an object of type [type].
		 */
		[[nodiscard]] inline bool			  is_object(object_type type) const noexcept;

		[[nodiscard]] constexpr bool		  is_boolean() const noexcept { return is_true() || is_false(); }
		[[nodiscard]] inline bool			  is_class() const noexcept { return is_object(object_type::CLASS_TYPE); }
		[[nodiscard]] inline bool			  is_closure() const noexcept { return is_object(object_type::CLOSURE_TYPE); }
		[[nodiscard]] inline bool			  is_fiber() const noexcept { return is_object(object_type::FIBER_TYPE); }
		[[nodiscard]] inline bool			  is_function() const noexcept { return is_object(object_type::FUNCTION_TYPE); }
		[[nodiscard]] inline bool			  is_outer() const noexcept { return is_object(object_type::OUTER_TYPE); }
		[[nodiscard]] inline bool			  is_instance() const noexcept { return is_object(object_type::INSTANCE_TYPE); }
		[[nodiscard]] inline bool			  is_list() const noexcept { return is_object(object_type::LIST_TYPE); }
		[[nodiscard]] inline bool			  is_map() const noexcept { return is_object(object_type::MAP_TYPE); }
		[[nodiscard]] inline bool			  is_string() const noexcept { return is_object(object_type::STRING_TYPE); }

		[[nodiscard]] constexpr double		  as_number() const noexcept;
		[[nodiscard]] inline object_string*	  as_string() const noexcept;
		[[nodiscard]] inline object_module*	  as_module() const noexcept;
		[[nodiscard]] inline object_function* as_function() const noexcept;
		[[nodiscard]] inline object_closure*  as_closure() const noexcept;
		[[nodiscard]] inline object_fiber*	  as_fiber() const noexcept;
		[[nodiscard]] inline object_class*	  as_class() const noexcept;
		[[nodiscard]] inline object_outer*	  as_outer() const noexcept;
		[[nodiscard]] inline object_instance* as_instance() const noexcept;
		[[nodiscard]] inline object_list*	  as_list() const noexcept;
		[[nodiscard]] inline object_map*	  as_map() const noexcept;

		/**
		 * @brief Mark [this] as reachable and still in use. This should only be called
		 * during the sweep phase of a garbage collection.
		 */
		void								  gray(gal_virtual_machine_state& state) const;

		/**
		 * @brief Returns true if [lhs] and [rhs] are strictly the same value. This is identity
		 * for object values, and value equality for unboxed values.
		 */
		friend bool							  operator==(const magic_value& lhs, const magic_value& rhs)
		{
			/**
			 * @brief Value types have unique bit representations and we compare object types
			 * by identity (i.e. pointer), so all we need to do is compare the bits.
			 */
			return lhs.data_ == rhs.data_;
		}

		/**
		 * @brief Returns true if [this] and [other] are equivalent. Immutable values
		 * (null, booleans, numbers, ranges, and strings) are equal if they have the
		 * same data. All other values are equal if they are identical objects.
		 */
		[[nodiscard]] bool equal(const magic_value& other) const;
	};

	constexpr magic_value magic_value_null{magic_value::null_val};
	constexpr magic_value magic_value_false{magic_value::false_val};
	constexpr magic_value magic_value_true{magic_value::true_val};
	constexpr magic_value magic_value_undefined{magic_value::undefined_val};

	object::			  operator magic_value() const noexcept
	{
		/**
		 * @brief The triple casting is necessary here to satisfy some compilers:
		 * 1. (uintptr_t) Convert the pointer to a number of the right size.
		 * 2. (value::value_type)  Pad it up in the bits to make a tagged NaN.
		 * 3. Cast to value.
		 */
		return magic_value{magic_value::pointer_mask | static_cast<magic_value::value_type>(reinterpret_cast<std::uintptr_t>(this))};
	}

	class magic_value_buffer
	{
		using buffer_type	  = std::vector<magic_value, gal_allocator<magic_value>>;
		using value_type	  = buffer_type::value_type;
		using size_type		  = buffer_type::size_type;

		using pointer		  = buffer_type::pointer;
		using const_pointer	  = buffer_type::const_pointer;

		using reference		  = buffer_type::reference;
		using const_reference = buffer_type::const_reference;

		using iterator		  = buffer_type::iterator;
		using const_iterator  = buffer_type::const_iterator;

	private:
		buffer_type						  buffer_;

		[[nodiscard]] constexpr reference operator[](size_type index) noexcept
		{
			return buffer_[index];
		}

		[[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept
		{
			return buffer_[index];
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return buffer_.size();
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return buffer_.begin();
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return buffer_.begin();
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return buffer_.end();
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return buffer_.end();
		}

		constexpr void clear()
		{
			buffer_.clear();
		}

		constexpr void push(value_type data)
		{
			buffer_.push_back(data);
		}

		constexpr void fill(value_type data, size_type size)
		{
			for (size_type i = 0; i < size; ++i)
			{
				push(data);
			}
		}

		/**
		 * @brief Mark the values in [this] as reachable and still in use. This should only
		 * be called during the sweep phase of a garbage collection.
		 */
		void gray(gal_virtual_machine_state& state);
	};

}// namespace gal

namespace std
{
	template<>
	struct hash<::gal::magic_value>
	{
		std::size_t operator()(const ::gal::magic_value& value) const
		{
			// todo
			(void)value;
			return 0;
		}
	};
}// namespace std

namespace gal
{
	/**
	 * @brief A heap-allocated string object.
	 */
	class object_string : public object
	{
	public:
		using string_type				= std::basic_string<char, std::char_traits<char>, gal_allocator<char>>;
		using size_type					= string_type::size_type;

		constexpr static size_type npos = string_type::npos;

	private:
		string_type string_;

	public:
		/**
		 * @brief Creates a new string object of [length] and copies [text] into it.
		 */
		object_string(gal_virtual_machine_state& state, const char* text, size_type length);

		/**
		 * @brief Creates a new string object by taking a range of characters from [source].
		* The range starts at [begin], contains [count] bytes, and increments by
		* [step].
		 */
		object_string(const object_string& source, size_type begin, size_type count, size_type step = 1);

		/**
		 * @brief Produces a string representation of [value].
		 */
		object_string(double value);

		/**
		 * @brief Creates a new formatted string from [format] and any additional arguments
		 * used in the format string.
		 *
		 * This is a very restricted flavor of formatting, intended only for internal
		 * use by the VM. Two formatting characters are supported, each of which reads
		 * the next argument as a certain type:
		 *
		 * $ - A const char* string.
		 * @ - A GAL string object.
		 */
		object_string(const char* format, ...);

		/**
		 * @brief Creates a new string containing the UTF-8 encoding of [value].
		 */
		object_string(int value);

		/**
		 * @brief Creates a new string from the integer representation of a byte
		 */
		object_string(std::uint8_t value);

		/**
		 * @brief Creates a new string containing the code point in [string] starting at byte
		 * [index]. If [index] points into the middle of a UTF-8 sequence, returns an
		 * empty string.
		 */
		object_string(object_string& string, size_type index);

		/**
		 * @brief Search for the first occurrence of [needle] and returns its
		 * zero-based offset. Returns `npos` if string does not contain [needle].
		 */
		size_type	find(object_string& needle, size_type start);

		/**
		 * @brief Returns true if [text] and [this] represent the same string.
		 */
		bool		operator==(const char* text) const;

		/**
		 * @brief Returns true if [text] and [this] represent the same string.
		 */
		bool		equal(const char* text, size_type length) const;

		friend bool operator==(const object_string& lhs, const object_string& rhs)
		{
			return lhs.string_ == rhs.string_;
		}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	class symbol_table
	{
	public:
		using table_type	  = std::vector<object_string, gal_allocator<object_string>>;
		using value_type	  = table_type::value_type;
		using size_type		  = table_type::size_type;

		using pointer		  = table_type::pointer;
		using const_pointer	  = table_type::const_pointer;

		using reference		  = table_type::reference;
		using const_reference = table_type::const_reference;

		using iterator		  = table_type::iterator;
		using const_iterator  = table_type::const_iterator;

	private:
		table_type table_;

	public:
		[[nodiscard]] constexpr reference operator[](size_type index) noexcept
		{
			return table_[index];
		}

		[[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept
		{
			return table_[index];
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return table_.size();
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return table_.begin();
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return table_.begin();
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return table_.end();
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return table_.end();
		}

		constexpr void clear()
		{
			table_.clear();
		}

		void push(const char* name, object_string::size_type length)
		{
			table_.emplace_back(name, length);
		}

		void push(object_string string)
		{
			table_.push_back(std::move(string));
		}

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 */
		[[nodiscard]] gal_index_type add(const char* name, object_string::size_type length)
		{
			table_.emplace_back(name, length);
			return static_cast<gal_index_type>(table_.size() - 1);
		}

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 */
		[[nodiscard]] gal_index_type add(object_string string)
		{
			table_.push_back(std::move(string));
			return static_cast<gal_index_type>(table_.size() - 1);
		}

		/**
		 * @brief Looks up name in the symbol table. Returns its index if found or gal_index_not_exist if not.
		 */
		[[nodiscard]] gal_index_type find(const char* name, object_string::size_type length);

		/**
		 * @brief Looks up name in the symbol table. Returns its index if found or gal_index_not_exist if not.
		 */
		[[nodiscard]] gal_index_type find(const object_string& string);

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 * Will use an existing symbol if already present.
		 */
		[[nodiscard]] gal_index_type ensure(const char* name, object_string::size_type length)
		{
			// See if the symbol is already defined.
			if (auto index = find(name, length); index == gal_index_not_exist)
			{
				// New symbol, so add it.
				return add(name, length);
			}
			else
			{
				return index;
			}
		}

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 * Will use an existing symbol if already present.
		 */
		[[nodiscard]] gal_index_type ensure(const object_string& string)
		{
			// See if the symbol is already defined.
			if (auto index = find(string); index == gal_index_not_exist)
			{
				// New symbol, so add it.
				return add(string);
			}
			else
			{
				return index;
			}
		}

		void blacken(gal_virtual_machine_state& state);
	};

	/**
	 * @brief Note that upvalues have this because they are garbage
	 * collected, but they are not first class GAL objects.
	 */
	class object_upvalue : public object
	{
	private:
		/**
		 * @brief Pointer to the variable this upvalue is referencing.
		 */
		magic_value* value_;

		/**
		 * @brief If the upvalue is closed (i.e. the local variable it was pointing to have
		 * been popped off the stack) then the closed-over value will be hoisted out
		 * of the stack into here. [value] will then be changed to point to this.
		 */
		magic_value	 closed_;

	public:
		/**
		 * @brief Creates a new open upvalue pointing to [value] on the stack.
		 */
		constexpr explicit object_upvalue(magic_value& value)
			: object(object_type::UPVALUE_TYPE, nullptr),
			  value_(&value),
			  closed_(magic_value_null) {}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	/**
	  * @brief The type of primitive function.
	  *
	  * Primitives are similar to outer functions, but have more direct access to
	  * VM internals. It is passed the arguments in [args]. If it returns a value,
	  * it places it in `args[0]` and returns `true`. If it causes a runtime error
	  * or modifies the running fiber, it returns `false`.
	  */
	using primitive_function_type = bool (*)(gal_virtual_machine_state& state, magic_value* args);

	/**
	  * @brief Stores debugging information for a function used for things like stack traces.
	  */
	struct debug_function
	{
		using source_line_buffer_type = std::vector<int>;

		/**
		  * @brief The name of the function.
		  */
		std::string				name;

		/**
		  * @brief An array of line numbers. There is one element in this array for each
		  * bytecode in the function's bytecode array. The value of that element is
		  * the line in the source code that generated that instruction.
		  */
		source_line_buffer_type source_lines;
	};

	/**
	  * @brief A loaded module and the top-level variables it defines.
	  *
	  * While this is an object and is managed by the GC, it never appears as a
	  * first-class object in GAL.
	  */
	class object_module : public object
	{
	private:
		/**
		  * @brief The currently defined top-level variables.
		  */
		magic_value_buffer variables_;

		/**
		  * @brief Symbol table for the names of all module variables. Indexes here directly
		  * correspond to entries in [variables].
		  */
		symbol_table	   variable_names_;

		/**
		  * @brief The name of the module.
		  */
		object_string&	   name_;

	public:
		/**
		 * @brief Creates a new module.
		 */
		explicit object_module(object_string& name)
			// Modules are never used as first-class objects, so don't need a class.
			: object{object_type::MODULE_TYPE, nullptr},
			  name_{name}
		{
		}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	/**
	  * @brief A function object. It wraps and owns the bytecode and other debug information
	  * for a callable chunk of code.
	  *
	  * Function objects are not passed around and invoked directly. Instead, they
	  * are always referenced by an [object_closure] which is the real first-class
	  * representation of a function. This isn't strictly necessary if they function
	  * has no upvalues, but lets the rest of the VM assume all called objects will
	  * be closures.
	  */
	class object_function : public object
	{
	public:
		using code_buffer_type		= std::vector<std::uint8_t, gal_allocator<std::uint8_t>>;
		using constants_buffer_type = magic_value_buffer;

	private:
		code_buffer_type	  code_;
		constants_buffer_type constants_;

		/**
		 * @brief The module where this function was defined.
		 */
		object_module&		  module_;

		/**
		 * @brief The maximum number of stack slots this function may use.
		 */
		gal_slot_type		  max_slots_;

		/**
		 * @brief The number of upvalues this function closes over.
		 */
		gal_size_type		  num_upvalues_;

		/**
		 * @brief The number of parameters this function expects. Used to ensure that.
		 * call handles a mismatch between number of parameters and arguments.
		 * This will only be set for functions, and not object_functions that represent
		 * methods or scripts.
		 */
		gal_size_type		  arity_;
		debug_function&		  debug_;

	public:
		/**
		 * @brief Creates a new empty function. Before being used, it must have code,
		 * constants, etc. added to it.
		 */
		object_function(gal_virtual_machine_state& state, object_module& module, gal_slot_type max_slots, debug_function& debug);

		void set_name(const char* name)
		{
			debug_.name = name;
		}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	/**
	 * @brief An instance of a first-class function and the environment it has closed over.
	 * Unlike [object_function], this has captured the upvalues that the function accesses.
	 */
	class object_closure : public object
	{
	private:
		/**
		 * @brief The function that this closure is an instance of.
		 */
		object_function&											 function_;
		std::vector<object_upvalue*, gal_allocator<object_upvalue*>> upvalues_;

	public:
		/**
		 * @brief Creates a new closure object that invokes [function]. Allocates room for its
		 * upvalues, but assumes outside code will populate it.
		 */
		object_closure(gal_virtual_machine_state& state, object_function& function);

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	struct call_frame
	{
		/**
		 * @brief Pointer to the current (really next-to-be-executed) instruction in the
		 * function's bytecode.
		 */
		std::uint8_t*	ip;

		/**
		 * @brief The closure being executed.
		 */
		object_closure* closure;

		/**
		 * @brief Pointer to the first stack slot used by this call frame. This will contain
		 * the receiver, followed by the function's parameters, then local variables
		 * and temporaries.
		 */
		magic_value*	stack_start;
	};

	enum class fiber_state
	{
		/**
		 * @brief The fiber is being run from another fiber using a call to `try()`.
		 */
		try_state,

		/**
		 * @brief The fiber was directly invoked by `run_interpreter()`. This means it's the
		 * initial fiber used by a call to `call()` or `interpret()`.
		 */
		root_state,

		/**
		 * @brief The fiber is invoked some other way. If [caller] is `nullptr` then the fiber
		 * was invoked using `call()`. If [num_frames] is zero, then the fiber has
		 * finished running and is done. If [num_frames] is one and that frame's `ip`
		 * points to the first byte of code, the fiber has not been started yet.
		 */
		other_state
	};

	class object_fiber : public object
	{
	public:
		using frames_buffer_type									 = std::vector<call_frame, gal_allocator<call_frame>>;
		using frames_buffer_size_type								 = frames_buffer_type::size_type;

		/**
		 * @brief The number of call frames initially allocated when a fiber is created. Making
		 * this smaller makes fibers use less memory (at first) but spends more time
		 * reallocating when the call stack grows.
		 */
		constexpr static frames_buffer_size_type initial_call_frames = 4;

	private:
		/**
		 * @brief The stack of value slots. This is used for holding local variables and
		 * temporaries while the fiber is executing. It is heap-allocated and grown
		 * as needed.
		 */
		magic_value*	   stack_;

		/**
		 * @brief A pointer to one past the top-most value on the stack.
		 */
		magic_value*	   stack_top_;

		/**
		 * @brief The number of allocated slots in the stack array.
		 */
		gal_size_type	   stack_capacity_;

		/**
		 * @brief The stack of call frames. This is a dynamic array that grows as needed but
		 * never shrinks.
		 */
		frames_buffer_type frames_;

		/**
		 * @brief Pointer to the first node in the linked list of open upvalues that are
		 * pointing to values still on the stack. The head of the list will be the
		 * upvalue closest to the top of the stack, and then the list works downwards.
		 */
		object_upvalue*	   open_upvalues_;

		/**
		 * @brief The fiber that ran this one. If this fiber is yielded, control will resume
		 * to this one. May be `nullptr`.
		 */
		object_fiber*	   caller_;

		/**
		 * @brief If the fiber failed because of a runtime error, this will contain the
		 * error object. Otherwise, it will be null.
		 */
		magic_value		   error_;

		fiber_state		   state_;

	public:
		/**
		 * @brief Creates a new fiber object that will invoke [closure].
		 */
		object_fiber(gal_virtual_machine_state& state, object_closure* closure);

		/**
		 * @brief Adds a new [call_frame] to [fiber] invoking [closure] whose stack starts at [stack_start].
		 */
		void					  add_call_frame(object_closure& closure, magic_value& stack_start);

		/**
		 * @brief Ensures [fiber]'s stack has at least [needed] slots.
		 */
		void					  ensure_stack(gal_virtual_machine_state& state, gal_size_type needed);

		[[nodiscard]] bool		  has_error() const;

		void					  push(magic_value value) { *stack_top_++ = value; }
		magic_value				  pop() { return *--stack_top_; }
		void					  drop() { --stack_top_; }
		[[nodiscard]] magic_value peek() const { return *(stack_top_ - 1); }
		[[nodiscard]] magic_value peek2() const { return *(stack_top_ - 2); }

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	enum class method_type
	{
		/**
		 * @brief A primitive method implemented in C++ in the VM.
		 * Unlike outer methods, this can directly manipulate the fiber's stack.
		 */
		primitive_type,

		/**
		 * @brief A primitive that handles .call on function.
		 */
		function_call_type,

		/**
		 * @brief an externally-defined C++ method.
		 */
		outer_type,

		/**
		 * @brief A normal user-defined method.
		 */
		block_type,

		/**
		 * @brief No method for the given symbol.
		 */
		none_type
	};

	struct method
	{
		method_type type;

		/**
		 * @brief The method function itself. The [type] determines which field of the union
		 * is used.
		 */
		union
		{
			primitive_function_type		   primitive_function;
			gal_outer_method_function_type outer_method_function;
			object_closure*				   closure;
		} as;
	};

	class object_class : public object
	{
	public:
		using method_buffer_type = std::vector<method, gal_allocator<method>>;

	private:
		object_class*	   superclass_;

		/**
		 * @brief The number of fields needed for an instance of this class, including all
		 * of its superclass fields.
		 */
		gal_size_type	   num_fields_;

		/**
		 * @brief The table of methods that are defined in or inherited by this class.
		 * Methods are called by symbol, and the symbol directly maps to an index in
		 * this table. This makes method calls fast at the expense of empty cells in
		 * the list for methods the class doesn't support.
		 *
		 * You can think of it as a hash table that never has collisions but has a
		 * really low load factor. Since methods are pretty small (just a type and a
		 * pointer), this should be a worthwhile trade-off.
		 */
		method_buffer_type methods_;

		/**
		 * @brief The name of the class.
		 */
		object_string&	   name_;

		/**
		 * @brief The ClassAttribute for the class, if any
		 */
		magic_value		   attributes_;

	public:
		/**
		 * @brief Creates a new "raw" class. It has no metaclass or superclass whatsoever.
		 * This is only used for bootstrapping the initial Object and Class classes,
		 * which are a little special.
		 */
		object_class(gal_virtual_machine_state& state, gal_size_type num_fields, object_string& name);

		/**
		 * @brief Makes [superclass] the superclass of [subclass], and causes subclass to
		 * inherit its methods. This should be called before any methods are defined
		 * on subclass.
		 */
		void			   bind_super_class(gal_virtual_machine_state& state, object_class& superclass);

		/**
		 * @brief Creates a new class object as well as its associated metaclass.
		 */
		object_class*	   create_derived_class(gal_virtual_machine_state& state, gal_size_type num_fields, object_string& name);

		void			   set_method(gal_virtual_machine_state& state, gal_index_type symbol, method m);

		[[nodiscard]] bool is_outer_class() const noexcept { return num_fields_ == gal_size_not_exist; }

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	class object_outer : public object
	{
	public:
		using data_buffer_type = std::vector<std::uint8_t, gal_allocator<std::uint8_t>>;

	private:
		data_buffer_type data_;

	public:
		explicit object_outer(object_class& obj_class)
			: object{object_type::OUTER_TYPE, &obj_class} {}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	class object_instance : public object
	{
	public:
		using field_buffer_type = std::vector<magic_value, gal_allocator<magic_value>>;

	private:
		field_buffer_type fields_;

	public:
		explicit object_instance(object_class& obj_class)
			: object(object_type::INSTANCE_TYPE, &obj_class)
		{
		}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	class object_list : public object
	{
	public:
		using list_buffer_type	 = std::vector<magic_value, gal_allocator<magic_value>>;
		using list_value_type	 = list_buffer_type::value_type;
		using list_size_type	 = list_buffer_type::size_type;
		using list_value_pointer = list_buffer_type::pointer;

	private:
		list_buffer_type elements_;

	public:
		explicit object_list(gal_virtual_machine_state& state);

		/**
		 * @brief Inserts [value] in [list] at [index], shifting down the other elements.
		 */
		void						 insert(gal_virtual_machine_state& state, magic_value value, list_size_type index);

		/**
		 * @brief Removes and returns the item at [index] from [list].
		 */
		magic_value					 remove(gal_virtual_machine_state& state, list_size_type index);

		/**
		 * @brief Searches for [value] in [list], returns the index or gal_index_not_exist if not found.
		 */
		[[nodiscard]] gal_index_type index_of(magic_value value) const;

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	/**
	 * @brief A hash table mapping keys to values.
	 */
	class object_map : public object
	{
	public:
		using map_buffer_type = std::unordered_map<magic_value, magic_value, std::hash<magic_value>, std::equal_to<>, gal_allocator<std::pair<const magic_value, magic_value>>>;

		using key_type		  = map_buffer_type::key_type;
		using mapped_type	  = map_buffer_type::mapped_type;
		using size_type		  = map_buffer_type::size_type;

		using pointer		  = map_buffer_type::pointer;
		using const_poiner	  = map_buffer_type::const_pointer;

		using reference		  = map_buffer_type::reference;
		using const_reference = map_buffer_type::const_reference;

		using iterator		  = map_buffer_type::iterator;
		using const_iterator  = map_buffer_type::const_iterator;

	private:
		map_buffer_type entries_;

	public:
		explicit object_map(gal_virtual_machine_state& state);

		/**
		 * @brief Validates that [arg] is a valid object for use as a map key. Returns true if
		 * it is and returns false otherwise. Use validate_key usually, for a runtime error.
		 * This separation exists to aid the API in surfacing errors to the developer as well.
		 */
		static bool is_valid_key(magic_value arg)
		{
			return arg.is_boolean() ||
				   arg.is_class() ||
				   arg.is_null() ||
				   arg.is_number() ||
				   arg.is_string();
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return entries_.size();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return entries_.begin();
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return entries_.begin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return entries_.end();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return entries_.end();
		}

		constexpr void clear()
		{
			entries_.clear();
		}

		/**
		 * @brief Looks up [key] in [map]. If found, returns the value. Otherwise, returns
		 * `magic_value_undefined`.
		 */
		magic_value get(magic_value key)
		{
			const auto ret = entries_.find(key);
			if (ret != entries_.end())
			{
				return ret->second;
			}
			return magic_value_undefined;
		}

		/**
		 * @brief Associates [key] with [value] in [map].
		 */
		void set(magic_value key, magic_value value)
		{
			entries_.insert_or_assign(key, value);
		}

		iterator find(magic_value key)
		{
			return entries_.find(key);
		}

		const_iterator find(magic_value key) const
		{
			return entries_.find(key);
		}

	private:
		void blacken(gal_virtual_machine_state& state) override;
		void destroy(gal_virtual_machine_state& state) override;
	};

	inline object* magic_value::as_object() const noexcept
	{
		return reinterpret_cast<object*>(static_cast<std::uintptr_t>(data_ & ~pointer_mask));
	}

	inline bool magic_value::is_object(object_type type) const noexcept
	{
		return is_object() && as_object()->type() == type;
	}

	constexpr double magic_value::as_number() const noexcept
	{
		return bits_to_double(data_);
	}

	inline object_string* magic_value::as_string() const noexcept
	{
		return dynamic_cast<object_string*>(as_object());
	}

	inline object_module* magic_value::as_module() const noexcept
	{
		return dynamic_cast<object_module*>(as_object());
	}

	inline object_function* magic_value::as_function() const noexcept
	{
		return dynamic_cast<object_function*>(as_object());
	}

	inline object_closure* magic_value::as_closure() const noexcept
	{
		return dynamic_cast<object_closure*>(as_object());
	}

	inline object_fiber* magic_value::as_fiber() const noexcept
	{
		return dynamic_cast<object_fiber*>(as_object());
	}

	inline object_class* magic_value::as_class() const noexcept
	{
		return dynamic_cast<object_class*>(as_object());
	}

	inline object_outer* magic_value::as_outer() const noexcept
	{
		return dynamic_cast<object_outer*>(as_object());
	}

	inline object_instance* magic_value::as_instance() const noexcept
	{
		return dynamic_cast<object_instance*>(as_object());
	}

	inline object_list* magic_value::as_list() const noexcept
	{
		return dynamic_cast<object_list*>(as_object());
	}

	inline object_map* magic_value::as_map() const noexcept
	{
		return dynamic_cast<object_map*>(as_object());
	}
}// namespace gal

#endif//GAL_LANG_VALUE_HPP
