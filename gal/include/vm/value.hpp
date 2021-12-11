#pragma once

#ifndef GAL_LANG_VALUE_HPP
	#define GAL_LANG_VALUE_HPP

	#include <gal.hpp>
	#include <allocator.hpp>
	#include <vm/common.hpp>
	#include <utility>
	#include <utils/utils.hpp>

	#include <limits>
	#include <vector>
	#include <string>
	#include <unordered_map>
	#include <map>
	#include <memory>
	#include <forward_list>

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
		object_type type_;

	protected:
		/**
		 * @brief The object's class.
		 */
		object_class* object_class_;

		object(object_type type, object_class* obj_class) noexcept;

	public:
		virtual ~object() noexcept = 0;

		[[nodiscard]] constexpr object_type type() const noexcept { return type_; }
		[[nodiscard]] object_class*			get_class() noexcept { return object_class_; }
		[[nodiscard]] const object_class*	get_class() const noexcept { return object_class_; }

		explicit							operator magic_value() const noexcept;

		/**
		 * @brief Create an object on the heap.
		 *
		 * The reason why this function is needed is to avoid explicitly allocating objects
		 * with new. Here we can use the specified allocator to get the object.
		 *
		 * If we need to allocate an object and convert it to magic_value and store it somewhere,
		 * we must perform heap allocation, otherwise magic_value will be invalidated after
		 * leaving the scope.
		 */
		template<typename T, typename... Args>
		requires std::is_base_of_v<object, T>
		static auto ctor(Args&&... args)
		{
			using allocator_type = gal_allocator<T>;
			auto  allocator		 = allocator_type{};
			auto* ptr			 = allocator.allocate(1);
			allocator.template construct(ptr, std::forward<Args>(args)...);
			return ptr;
		}

		/**
		 * @brief Destroy an object on the heap created by ctor.
		 */
		template<typename T>
		requires std::is_base_of_v<object, T>
		static void dtor(T* ptr)
		{
			using allocator_type = gal_allocator<T>;
			auto allocator		 = allocator_type{};
			ptr->destroy();
			allocator.template destroy(ptr);
			allocator.deallocate(ptr, 1);
		}

		/**
		 * @brief Try to get the amount of memory used by the object
		 */
		[[nodiscard]] virtual gal_size_type memory_usage() const noexcept = 0;

	private:
		/**
		 * @brief Destroy all dynamically allocated objects in the class, generally called before the object will be recycled (destructed)
		 *
		 * Usually, this function does not need to do anything (because the memory is managed by the STL components),
		 * but if the class contains a container for storing polymorphic objects (such as map),
		 * then you need to call this function to destroy all of them.
		 */
		virtual void destroy() {}
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
		constexpr static value_type			  tag_mask{(1 << 3) - 1};// 7

		/**
		 * @brief Tag values for the different singleton values.
		 */
		constexpr static value_type			  tag_nan{0};
		constexpr static value_type			  tag_null{1};
		constexpr static value_type			  tag_false{2};
		constexpr static value_type			  tag_true{3};
		constexpr static value_type			  tag_undefined{4};
		constexpr static value_type			  tag_reserve1{5};
		constexpr static value_type			  tag_reserve2{6};
		constexpr static value_type			  tag_reserve3{7};

		/**
		 * @brief A mask that selects the sign bit.
		 */
		constexpr static value_type			  sign_bit{value_type{1} << 63};

		/**
		 * @brief The bits that must be set to indicate a quiet NaN.
		 *
		 * note:
		 *  it's: 0 111 1111 1111 1100 000000000000000000000000000000000000000000000000
		 *  not : 0 111 1111 1111 1000 000000000000000000000000000000000000000000000000
		 */
		constexpr static value_type			  quiet_nan{0x7ffc000000000000};

		constexpr static value_type			  pointer_mask{quiet_nan | sign_bit};

		/**
		 * @brief Singleton values.
		 */
		constexpr static value_type			  null_val{quiet_nan | tag_nan};
		constexpr static value_type			  false_val{quiet_nan | tag_false};
		constexpr static value_type			  true_val{quiet_nan | tag_true};
		constexpr static value_type			  undefined_val{quiet_nan | tag_undefined};

		value_type							  data_;

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

		// todo: limit the usage or remove it.
		friend auto operator<=>(const magic_value& lhs, const magic_value& rhs)
		{
			return lhs.data_ <=> rhs.data_;
		}

		/**
		 * @brief Returns true if [this] and [other] are equivalent. Immutable values
		 * (null, booleans, numbers, ranges, and strings) are equal if they have the
		 * same data. All other values are equal if they are identical objects.
		 */
		[[nodiscard]] bool equal(const magic_value& other) const;

		/**
		 * @brief If the value stored by magic_value points to an object constructed using dynamic memory,
		 * it will be released, otherwise it will do nothing. This function is to keep magic_value as a POD.
		 *
		 * todo: It can be much simpler to change it to a destructor :)
		 */
		void			   destroy()
		{
			if (is_object())
			{
				object::dtor(as_object());
			}
			data_ = null_val;
		}
	};

	constexpr magic_value magic_value_null{magic_value::null_val};
	constexpr magic_value magic_value_false{magic_value::false_val};
	constexpr magic_value magic_value_true{magic_value::true_val};
	constexpr magic_value magic_value_undefined{magic_value::undefined_val};

	// move magic_value's constructor here so that our magic_value is a POD type
	constexpr magic_value to_magic_value(magic_value::value_type data) noexcept
	{
		return {data};
	}

	constexpr magic_value to_magic_value(bool b) noexcept
	{
		return b ? magic_value_true : magic_value_false;
	}

	constexpr magic_value to_magic_value(double data) noexcept
	{
		return {double_to_bits(data)};
	}

	object::operator magic_value() const noexcept
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
	public:
		using buffer_type	 = std::vector<magic_value, gal_allocator<magic_value>>;
		using value_type	 = buffer_type::value_type;
		using size_type		 = buffer_type::size_type;

		using pointer		 = buffer_type::pointer;
		using const_pointer	 = buffer_type::const_pointer;

		using reference		 = buffer_type::reference;

		using iterator		 = buffer_type::iterator;
		using const_iterator = buffer_type::const_iterator;

	private:
		buffer_type buffer_;

	public:
		[[nodiscard]] reference operator[](size_type index) noexcept
		{
			return buffer_[index];
		}

		[[nodiscard]] value_type operator[](size_type index) const noexcept
		{
			return buffer_[index];
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return buffer_.size();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return buffer_.begin();
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return buffer_.begin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return buffer_.end();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return buffer_.end();
		}

		void clear()
		{
			buffer_.clear();
		}

		void push(value_type data)
		{
			buffer_.push_back(data);
		}

		void fill(value_type data, size_type size)
		{
			for (size_type i = 0; i < size; ++i)
			{
				push(data);
			}
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept
		{
			return sizeof(value_type) * buffer_.capacity();
		}
	};

	/**
	 * @brief A heap-allocated string object.
	 */
	class object_string : public object
	{
	public:
		using string_type				= std::basic_string<char, std::char_traits<char>, gal_allocator<char>>;

		using value_type				= string_type::value_type;
		using size_type					= string_type::size_type;

		using pointer					= string_type::pointer;
		using const_pointer				= string_type::const_pointer;

		using reference					= string_type::reference;
		using const_reference			= string_type::const_reference;

		using iterator					= string_type::iterator;
		using const_iterator			= string_type::const_iterator;

		constexpr static size_type npos = string_type::npos;

	private:
		string_type string_;

	public:
		/**
		 * @brief Creates a new empty string object
		 */
		explicit object_string(gal_virtual_machine_state& state);

		/**
		 * @brief Creates a new string object of [length] and init with [c].
		 */
		object_string(gal_virtual_machine_state& state, size_type length, value_type c = 0);

		/**
		 * @brief Creates a new string object of [length] and copies [text] into it.
		 */
		object_string(gal_virtual_machine_state& state, const_pointer text, size_type length);

		/**
		 * @brief Move an exist string into [this]
		 */
		object_string(gal_virtual_machine_state& state, string_type string);

		/**
		 * @brief Creates a new string object by taking a range of characters from [source].
		* The range starts at [begin], contains [count] bytes, and increments by
		* [step].
		 */
		object_string(gal_virtual_machine_state& state, const object_string& source, size_type begin, size_type count, size_type step = 1);

		/**
		 * @brief Produces a string representation of [value].
		 */
		object_string(gal_virtual_machine_state& state, double value);

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
		[[deprecated("use get_appender instead")]] object_string(gal_virtual_machine_state& state, const char* format, ...);

		/**
		 * @brief Creates a new string containing the UTF-8 encoding of [value].
		 */
		object_string(gal_virtual_machine_state& state, int value);

		/**
		 * @brief Creates a new string from the integer representation of a byte
		 */
		object_string(gal_virtual_machine_state& state, std::uint8_t value);

		/**
		 * @brief Creates a new string containing the code point in [string] starting at byte
		 * [index]. If [index] points into the middle of a UTF-8 sequence, returns an
		 * empty string.
		 */
		object_string(gal_virtual_machine_state& state, object_string& string, size_type index);

		[[nodiscard]] bool empty() const noexcept
		{
			return string_.empty();
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return string_.size();
		}

		[[nodiscard]] pointer data() noexcept
		{
			return string_.data();
		}

		[[nodiscard]] const_pointer data() const noexcept
		{
			return string_.data();
		}

		[[nodiscard]] const string_type& str() const noexcept
		{
			return string_;
		}

		[[nodiscard]] reference operator[](size_type index) noexcept
		{
			return string_[index];
		}

		[[nodiscard]] const_reference operator[](size_type index) const noexcept
		{
			return string_[index];
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return string_.begin();
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return string_.begin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return string_.end();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return string_.end();
		}

		void clear()
		{
			string_.clear();
		}

		/**
		 * @brief Search for the first occurrence of [needle] and returns its
		 * zero-based offset. Returns `npos` if string does not contain [needle].
		 */
		size_type	   find(object_string& needle, size_type start);

		object_string& append(const char* text, size_type length)
		{
			string_.append(text, length);
			return *this;
		}

		object_string& append(const char* text)
		{
			string_.append(text);
			return *this;
		}

		object_string& append(size_type count, value_type c)
		{
			string_.append(count, c);
			return *this;
		}

		object_string& append(const object_string& string)
		{
			string_.append(string.string_);
			return *this;
		}

		auto get_appender()
		{
			return std::back_inserter(string_);
		}

		object_string& operator+=(const object_string& string)
		{
			string_ += string.string_;
			return *this;
		}

		void push_back(value_type c)
		{
			string_.push_back(c);
		}

		/**
		 * @brief Returns true if [text] and [string] represent the same string.
		 */
		friend bool operator==(const object_string& string, const_pointer text)
		{
			return string.string_ == text;
		}

		/**
		 * @brief Returns true if [text] and [string] represent the same string.
		 */
		friend bool operator==(const_pointer text, const object_string& string)
		{
			return string.string_ == text;
		}

		/**
		 * @brief Returns true if [text] and [this] represent the same string.
		 */
		bool equal(size_type begin, size_type length, const_pointer text) const
		{
			return string_.compare(begin, length, text);
		}

		/**
		 * @brief Returns true if [text] and [this] represent the same string.
		 */
		bool equal(size_type length, const_pointer text) const
		{
			return equal(0, length, text);
		}

		friend bool operator==(const object_string& lhs, const object_string& rhs)
		{
			return lhs.string_ == rhs.string_;
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}
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
		[[nodiscard]] reference operator[](size_type index) noexcept
		{
			return table_[index];
		}

		[[nodiscard]] const_reference operator[](size_type index) const noexcept
		{
			return table_[index];
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return table_.size();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return table_.begin();
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return table_.begin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return table_.end();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return table_.end();
		}

		void clear()
		{
			table_.clear();
		}

		void push(gal_virtual_machine_state& state, const char* name, object_string::size_type length)
		{
			table_.emplace_back(state, name, length);
		}

		void push(object_string string)
		{
			table_.push_back(std::move(string));
		}

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 */
		[[nodiscard]] gal_index_type add(gal_virtual_machine_state& state, const char* name, object_string::size_type length)
		{
			table_.emplace_back(state, name, length);
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
		[[nodiscard]] gal_index_type find(const char* name, object_string::size_type length) const;

		/**
		 * @brief Looks up name in the symbol table. Returns its index if found or gal_index_not_exist if not.
		 */
		[[nodiscard]] gal_index_type find(const object_string& string) const;

		/**
		 * @brief Adds name to the symbol table. Returns the index of it in the table.
		 * Will use an existing symbol if already present.
		 */
		[[nodiscard]] gal_index_type ensure(gal_virtual_machine_state& state, const char* name, object_string::size_type length)
		{
			// See if the symbol is already defined.
			if (auto index = find(name, length); index == gal_index_not_exist)
			{
				// New symbol, so add it.
				return add(state, name, length);
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
		explicit object_upvalue(magic_value& value)
			// Upvalues are never used as first-class objects, so don't need a class.
			: object{object_type::UPVALUE_TYPE, nullptr},
			  value_{&value},
			  closed_{magic_value_null}
		{
		}

		[[nodiscard]] const magic_value* get_value() const noexcept
		{
			return value_;
		}

		void reset_value(magic_value* value) noexcept
		{
			value_ = value;
		}

		void close() noexcept
		{
			closed_ = *value_;
			value_	= &closed_;
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}
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
		using source_line_buffer_type = std::vector<int, gal_allocator<int>>;
		using line_type				  = source_line_buffer_type::value_type;

		/**
		  * @brief The name of the function.
		  */
		std::string					name;

		/**
		  * @brief An array of line numbers. There is one element in this array for each
		  * bytecode in the function's bytecode array. The value of that element is
		  * the line in the source code that generated that instruction.
		  */
		source_line_buffer_type		source_lines;

		[[nodiscard]] gal_size_type memory_usage() const noexcept
		{
			// todo: What about the function name?
			return sizeof(line_type) * source_lines.capacity();
		}
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
		using code_buffer_type			  = std::vector<std::uint8_t, gal_allocator<std::uint8_t>>;
		using code_buffer_value_type	  = code_buffer_type::value_type;
		using code_buffer_size_type		  = code_buffer_type::size_type;
		using code_buffer_pointer		  = code_buffer_type::pointer;
		using code_buffer_const_pointer	  = code_buffer_type::const_pointer;

		using constants_buffer_type		  = magic_value_buffer;
		using constants_buffer_value_type = magic_value_buffer::value_type;
		using constants_buffer_size_type  = magic_value_buffer::size_type;

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
		debug_function		  debug_;

	public:
		/**
		 * @brief Creates a new empty function. Before being used, it must have code,
		 * constants, etc. added to it.
		 */
		object_function(gal_virtual_machine_state& state, object_module& module, gal_slot_type max_slots);

		[[nodiscard]] const auto& get_name() const noexcept
		{
			return debug_.name;
		}

		void set_name(const char* name)
		{
			debug_.name = name;
		}

		[[nodiscard]] gal_slot_type get_slots_size() const noexcept
		{
			return max_slots_;
		}

		[[nodiscard]] gal_size_type get_upvalues_size() const noexcept
		{
			return num_upvalues_;
		}

		[[nodiscard]] code_buffer_const_pointer get_code_data() const noexcept
		{
			return code_.data();
		}

		[[nodiscard]] code_buffer_size_type get_code_size() const noexcept
		{
			return code_.size();
		}

		object_function& append_code(code_buffer_value_type data)
		{
			code_.push_back(data);
			return *this;
		}

		[[nodiscard]] decltype(auto) get_constant(constants_buffer_size_type index) noexcept
		{
			return constants_[index];
		}

		[[nodiscard]] decltype(auto) get_constant(constants_buffer_size_type index) const noexcept
		{
			return constants_[index];
		}

		[[nodiscard]] gal_size_type get_parameters_arity() const noexcept
		{
			return arity_;
		}

		[[nodiscard]] bool check_parameters_arity(gal_size_type num_args) const noexcept
		{
			return get_parameters_arity() <= num_args;
		}

		[[nodiscard]] object_module& get_module() noexcept
		{
			return module_;
		}

		[[nodiscard]] const object_module& get_module() const noexcept
		{
			return module_;
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}
	};

	/**
	 * @brief An instance of a first-class function and the environment it has closed over.
	 * Unlike [object_function], this has captured the upvalues that the function accesses.
	 */
	class object_closure : public object
	{
	public:
		using upvalue_container			   = std::vector<object_upvalue*, gal_allocator<object_upvalue*>>;
		using upvalue_container_value_type = upvalue_container::value_type;
		using upvalue_container_size_type  = upvalue_container::size_type;

	private:
		/**
		 * @brief The function that this closure is an instance of.
		 *
		 * @note The function should be heap allocated, and we are responsible for releasing it.
		 *
		 * Use a reference to make sure we have a valid function.
		 *
		 * todo: Heap allocation maybe is not necessary.
		 */
		object_function&  function_;
		upvalue_container upvalues_;

	public:
		/**
		 * @brief Creates a new closure object that invokes [function]. Allocates room for its
		 * upvalues, but assumes outside code will populate it.
		 */
		object_closure(gal_virtual_machine_state& state, object_function& function);

		[[nodiscard]] object_function& get_function() noexcept
		{
			return function_;
		}

		[[nodiscard]] const object_function& get_function() const noexcept
		{
			return function_;
		}

		[[nodiscard]] upvalue_container_value_type get_upvalue(upvalue_container_size_type index) const noexcept
		{
			return upvalues_[index];
		}

		void push_upvalue(upvalue_container_value_type value)
		{
			upvalues_.push_back(value);
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
			object::dtor(&function_);
		}
	};

	struct call_frame
	{
		/**
		 * @brief Pointer to the current (really next-to-be-executed) instruction in the
		 * function's bytecode.
		 */
		object_function::code_buffer_const_pointer ip;

		/**
		 * @brief The closure being executed.
		 */
		object_closure*							   closure;

		/**
		 * @brief Pointer to the first stack slot used by this call frame. This will contain
		 * the receiver, followed by the function's parameters, then local variables
		 * and temporaries.
		 */
		magic_value*							   stack_start;
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
		using frames_buffer_type			= std::vector<call_frame, gal_allocator<call_frame>>;
		using frames_buffer_value_type		= frames_buffer_type::value_type;
		using frames_buffer_size_type		= frames_buffer_type::size_type;
		using frames_buffer_reference		= frames_buffer_type::reference;
		using frames_buffer_const_reference = frames_buffer_type::const_reference;

	private:
		/**
		 * @brief The stack of value slots. This is used for holding local variables and
		 * temporaries while the fiber is executing. It is heap-allocated and grown
		 * as needed.
		 */
		std::unique_ptr<magic_value[]>									 stack_;

		/**
		 * @brief A pointer to one past the top-most value on the stack.
		 */
		magic_value*													 stack_top_;

		/**
		 * @brief The number of allocated slots in the stack array.
		 */
		gal_size_type													 stack_capacity_;

		/**
		 * @brief The stack of call frames. This is a dynamic array that grows as needed but
		 * never shrinks.
		 */
		frames_buffer_type												 frames_;

		/**
		 * @brief The linked list of open upvalues that are pointing to values
		 * still on the stack. The tail of the list will be the upvalue closest
		 * to the top of the stack, and then the list works upwards.
		 */
		std::forward_list<object_upvalue, gal_allocator<object_upvalue>> open_upvalues_;

		/**
		 * @brief The fiber that ran this one. If this fiber is yielded, control will resume
		 * to this one. May be `nullptr`.
		 */
		object_fiber*													 caller_;

		/**
		 * @brief If the fiber failed because of a runtime error, this will contain the
		 * error object. Otherwise, it will be nullptr.
		 */
		std::shared_ptr<magic_value>									 error_;

		fiber_state														 state_;

	public:
		/**
		 * @brief Creates a new fiber object that will invoke [closure].
		 */
		object_fiber(gal_virtual_machine_state& state, object_closure* closure);

		[[nodiscard]] bool has_frame() const noexcept
		{
			return frames_.empty();
		}

		/**
		 * @brief Adds a new [call_frame] to [fiber] invoking [closure] whose stack starts at [stack_start].
		 */
		void								  add_call_frame(object_closure& closure, magic_value& stack_start);

		[[nodiscard]] frames_buffer_reference get_recent_frame() noexcept
		{
			return frames_.back();
		}

		[[nodiscard]] frames_buffer_const_reference get_recent_frame() const noexcept
		{
			return frames_.back();
		}

		void pop_recent_frame() noexcept
		{
			frames_.pop_back();
		}

		/**
		 * @brief Pushes [closure] onto [this]'s callstack to invoke it. Expects [num_args]
		 * arguments (including the receiver) to be on the top of the stack already.
		 */
		void call_function(gal_virtual_machine_state& state, object_closure& closure, gal_size_type num_args)
		{
			// Grow the stack if needed.
			const auto needed = get_current_stack_size() + closure.get_function().get_slots_size();
			ensure_stack(state, needed);

			add_call_frame(closure, *get_stack_point(num_args));
		}

		/**
		 * @brief Ensures [fiber]'s stack has at least [needed] slots.
		 */
		void						ensure_stack(gal_virtual_machine_state& state, gal_size_type needed);

		[[nodiscard]] gal_size_type get_current_stack_size(magic_value* bottom) const noexcept
		{
			return stack_top_ - bottom;
		}

		[[nodiscard]] gal_size_type get_current_stack_size() const noexcept
		{
			return stack_top_ - stack_.get();
		}

		[[nodiscard]] magic_value* get_stack_bottom() const noexcept
		{
			return stack_.get();
		}

		[[nodiscard]] magic_value* get_stack_point(gal_size_type offset) noexcept
		{
			return stack_top_ - offset;
		}

		[[nodiscard]] const magic_value* get_stack_point(gal_size_type offset) const noexcept
		{
			return stack_top_ - offset;
		}

		void set_stack_point(gal_size_type offset, magic_value value) noexcept
		{
			stack_top_[-offset] = value;
		}

		void set_stack_top(magic_value* new_top) noexcept
		{
			stack_top_ = new_top;
		}

		void stack_top_rebase(gal_size_type offset) noexcept
		{
			stack_top_ = stack_.get() + offset;
		}

		void pop_stack(gal_size_type offset) noexcept
		{
			stack_top_ -= offset;
		}

		/**
		 * @brief Captures the local variable [local] into an [object_upvalue]. If that local is
		 * already in an upvalue, the existing one will be used. (This is important to
		 * ensure that multiple closures closing over the same variable actually see
		 * the same variable.) Otherwise, it will create a new open upvalue and add it
		 * the fiber's list of upvalues.
		 */
		object_upvalue&	   capature_upvalue(magic_value& local);

		/**
		 * @brief Closes any open upvalues that have been created for stack slots at [last]
		 * and above.
		 */
		void			   close_upvalue(magic_value& last);

		/**
		 * @brief Creates a new class.
		 *
		 * If [num_fields] is -1, the class is a outer class. The name and superclass
		 * should be on top of the fiber's stack. After calling this, the top of the
		 * stack will contain the new class.
		 *
		 * Aborts the current fiber if an error occurs.
		 */
		void			   create_class(gal_virtual_machine_state& state, gal_size_type num_fields, object_module* module);

		/**
		 * @brief Completes the process for creating a new class.
		 *
		 * The class attributes instance and the class itself should be on the
		 * top of the fiber's stack.
		 *
		 * This process handles moving the attribute data for a class from
		 * compile time to runtime, since it now has all the attributes associated
		 * with a class, including for methods.
		 */
		void			   end_class();

		[[nodiscard]] bool has_caller() const noexcept
		{
			return not caller_;
		}

		[[nodiscard]] object_fiber* get_caller() const noexcept
		{
			return caller_;
		}

		void set_caller(object_fiber* caller) noexcept
		{
			// todo: What should we do with the previously existing caller?
			caller_ = caller;
		}

		void set_error(magic_value error)
		{
			// error_ = std::make_shared<magic_value>(error);

			using allocator_type = gal_allocator<magic_value>;
			allocator_type allocator{};
			auto*		   ptr = allocator.allocate(1);
			allocator.construct(ptr, error);
			error_ = std::shared_ptr<magic_value>(ptr,
												  [](magic_value* value)
												  {
													  value->destroy();
													  allocator_type allocator{};
													  allocator.destroy(value);
													  allocator.deallocate(value, 1);
												  });
		}

		void clear_error()
		{
			error_.reset();
		}

		[[nodiscard]] bool has_error() const noexcept
		{
			return error_.operator bool();
		}

		[[nodiscard]] fiber_state get_state() const noexcept
		{
			return state_;
		}

		void set_state(fiber_state new_state) noexcept
		{
			state_ = new_state;
		}

		[[nodiscard]] object_fiber* raise_error();

		void						stack_push(magic_value value) { *stack_top_++ = value; }
		magic_value*				stack_pop() { return --stack_top_; }
		void						stack_drop() { --stack_top_; }
		[[nodiscard]] magic_value*	stack_peek() const { return stack_top_ - 1; }
		[[nodiscard]] magic_value*	stack_peek2() const { return stack_top_ - 2; }

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
			// Stack functions.
			// Stack variables.
			// Open upvalues.
			// The caller.
		}
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
		using method_buffer_type								   = std::vector<method, gal_allocator<method>>;
		using method_buffer_value_type							   = method_buffer_type::value_type;
		using method_buffer_size_type							   = method_buffer_type::size_type;
		using method_buffer_reference							   = method_buffer_type::reference;
		using method_buffer_const_reference						   = method_buffer_type::const_reference;

		constexpr static gal_size_type outer_class_fields_size	   = -1;
		constexpr static gal_size_type interface_class_fields_size = 0;

	private:
		// todo: Do we need to support multiple inheritance?
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
		object_string	   name_;

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
		object_class(gal_size_type num_fields, object_string name)
			: object{object_type::CLASS_TYPE, nullptr},
			  superclass_{nullptr},
			  num_fields_{num_fields},
			  methods_{},
			  name_{std::move(name)},
			  attributes_{magic_value_null} {}

		/**
		 * @brief Makes [superclass] the superclass of [subclass], and causes subclass to
		 * inherit its methods. This should be called before any methods are defined
		 * on subclass.
		 */
		void							  bind_super_class(object_class& superclass);

		[[nodiscard]] const object_class* get_super_class() const noexcept
		{
			return superclass_;
		}

		/**
		 * @brief Creates a new class object as well as its associated metaclass.
		 */
		std::shared_ptr<object_class>		  create_derived_class(gal_virtual_machine_state& state, gal_size_type num_fields, object_string& name);

		[[nodiscard]] method_buffer_size_type get_methods_size() const noexcept
		{
			return methods_.size();
		}

		[[nodiscard]] method_buffer_reference get_method(method_buffer_size_type index) noexcept
		{
			return methods_[index];
		}

		[[nodiscard]] method_buffer_const_reference get_method(method_buffer_size_type index) const noexcept
		{
			return methods_[index];
		}

		void set_method(method_buffer_size_type symbol, method m)
		{
			// Make sure the buffer is big enough to contain the symbol's index.
			if (symbol >= methods_.size())
			{
				method none{.type = method_type::none_type, .as{.closure = nullptr}};
				for (auto i = symbol - methods_.size(); i > 0; --i)
				{
					methods_.push_back(none);
				}
			}
			methods_.push_back(m);
		}

		[[nodiscard]] gal_size_type get_field_size() const noexcept
		{
			return num_fields_;
		}

		[[nodiscard]] gal_size_type get_remain_field_size() const noexcept
		{
			return max_fields - num_fields_;
		}

		[[nodiscard]] const object_string& get_class_name() const noexcept
		{
			return name_;
		}

		[[nodiscard]] magic_value get_attributes() const noexcept
		{
			return attributes_;
		}

		void set_attributes(magic_value attributes) noexcept
		{
			// todo: How to deal with existing attributes?
			attributes_ = attributes;
		}

		[[nodiscard]] bool			is_outer_class() const noexcept { return num_fields_ == outer_class_fields_size; }
		[[nodiscard]] bool			is_interface_class() const noexcept { return num_fields_ == interface_class_fields_size; }

		[[nodiscard]] static bool	is_outer_class_fields(gal_size_type num_fields) noexcept { return num_fields == outer_class_fields_size; };
		[[nodiscard]] static bool	is_interface_class_fields(gal_size_type num_fields) noexcept { return num_fields == interface_class_fields_size; }

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
			// The metaclass.
			// The superclass.
			// Method function objects.
			// attributes
		}
	};

	class object_outer : public object
	{
	public:
		using data_buffer_type			= std::vector<std::uint8_t, gal_allocator<std::uint8_t>>;
		using data_buffer_value_type	= data_buffer_type::value_type;
		using data_buffer_size_type		= data_buffer_type::size_type;
		using data_buffer_pointer		= data_buffer_type::pointer;
		using data_buffer_const_pointer = data_buffer_type::const_pointer;

	private:
		data_buffer_type data_;

	public:
		explicit object_outer(object_class* obj_class, gal_size_type size)
			: object{object_type::OUTER_TYPE, obj_class},
			  data_(size) {}

		[[nodiscard]] data_buffer_pointer get_data() noexcept
		{
			return data_.data();
		}

		[[nodiscard]] data_buffer_const_pointer get_data() const noexcept
		{
			return data_.data();
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo: Keep track of how much memory the outer object uses. We can store
			// this in each outer object, but it will balloon the size. We may not want
			// that much overhead. One option would be to let the outer class register
			// a C++ function that returns a size for the object. That way the VM doesn't
			// always have to explicitly store it.
			return 0;
		}
	};

	class object_instance : public object
	{
	public:
		using field_buffer_type			  = std::vector<magic_value, gal_allocator<magic_value>>;
		using field_buffer_value_type	  = field_buffer_type::value_type;
		using field_buffer_size_type	  = field_buffer_type::size_type;
		using field_buffer_type_reference = field_buffer_type::reference;

	private:
		field_buffer_type fields_;

	public:
		explicit object_instance(object_class* obj_class)
			: object{object_type::OUTER_TYPE, obj_class},
			  fields_{}
		{
			// Initialize fields to null.
			const auto size = object_class_->get_field_size();
			fields_.reserve(size);
			for (auto i = size; i > 0; --i)
			{
				fields_.push_back(magic_value_null);
			}
		}

		[[nodiscard]] auto get_field_size() const noexcept
		{
			return object_class_->get_field_size();
		}

		[[nodiscard]] field_buffer_type_reference get_field(field_buffer_size_type index) noexcept
		{
			return fields_[index];
		}

		[[nodiscard]] field_buffer_value_type get_field(field_buffer_size_type index) const noexcept
		{
			return fields_[index];
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
		}
	};

	class object_list : public object
	{
	public:
		// todo: use vector or list?
		using list_buffer_type			  = std::vector<magic_value, gal_allocator<magic_value>>;
		using list_buffer_value_type	  = list_buffer_type::value_type;
		using list_buffer_size_type		  = list_buffer_type::size_type;
		using list_buffer_difference_type = list_buffer_type::difference_type;
		using list_buffer_value_pointer	  = list_buffer_type::pointer;

	private:
		list_buffer_type elements_;

	public:
		explicit object_list(gal_virtual_machine_state& state);

		[[nodiscard]] list_buffer_size_type size() const noexcept
		{
			return elements_.size();
		}

		/**
		 * @brief Inserts [value] in [list] at [index].
		 */
		void insert(list_buffer_size_type index, list_buffer_value_type value)
		{
			// Store the new element.
			elements_.insert(std::next(elements_.begin(), static_cast<list_buffer_difference_type>(index)), value);
		}

		/**
		 * @brief Removes and returns the item at [index] from [list].
		 */
		list_buffer_value_type remove(list_buffer_size_type index)
		{
			auto removed = elements_[index];
			elements_.erase(std::next(elements_.begin(), static_cast<list_buffer_difference_type>(index)));
			return removed;
		}

		[[nodiscard]] list_buffer_value_type get(list_buffer_size_type index) const noexcept
		{
			return elements_[index];
		}

		void set(list_buffer_size_type index, list_buffer_value_type value) noexcept
		{
			elements_[index] = value;
		}

		/**
		 * @brief Searches for [value] in [list], returns the index or gal_index_not_exist if not found.
		 */
		[[nodiscard]] gal_index_type index_of(magic_value value) const;

		[[nodiscard]] gal_size_type	 memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
			// elements
		}
	};

}// namespace gal

namespace std
{
	template<>
	struct hash<::gal::object_string>
	{
		using is_transparent = void;
		std::size_t operator()(const ::gal::object_string& str) const
		{
			// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
			constexpr std::uint64_t hash_init{14695981039346656037ull};
			constexpr std::uint64_t hash_prime{1099511628211ull};

			auto					hash = hash_init;
			for (auto c: str.str())
			{
				hash ^= c;
				hash *= hash_prime;
			}
			return hash;
		}

		std::size_t operator()(::gal::object_string::const_pointer str) const
		{
			// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
			constexpr std::uint64_t hash_init{14695981039346656037ull};
			constexpr std::uint64_t hash_prime{1099511628211ull};

			auto					hash = hash_init;
			for (auto c = *str; c != '\0'; ++str, c = *str)
			{
				hash ^= c;
				hash *= hash_prime;
			}
			return hash;
		}
	};

	template<>
	struct hash<::gal::magic_value>
	{
		std::size_t operator()(const ::gal::magic_value& value) const
		{
			constexpr static auto hash_bits = [](std::size_t hash) constexpr noexcept
			{
				hash = ~hash + (hash << 18);// hash = (hash << 18) - hash - 1;
				hash = hash ^ (hash >> 31);
				hash = hash * 21;// hash = (hash + (hash << 2)) + (hash << 4);
				hash = hash ^ (hash >> 11);
				hash = hash + (hash << 6);
				hash = hash ^ (hash >> 22);
				return hash & 0x3fffffff;
			};

			if (value.is_object())
			{
				const auto* obj = value.as_object();
				switch (obj->type())
				{
					case ::gal::object_type::CLASS_TYPE:
					{
						// Classes just use their name.
						return hash<::gal::object_string>{}(dynamic_cast<const ::gal::object_class&>(*obj).get_class_name());
					}
					case ::gal::object_type::FUNCTION_TYPE:
					{
						// Allow bare (non-closure) functions so that we can use a map to find
						// existing constants in a function's constant table. This is only used
						// internally. Since user code never sees a non-closure function, they
						// cannot use them as map keys.
						const auto& function = dynamic_cast<const ::gal::object_function&>(*obj);
						return hash_bits(function.get_parameters_arity()) ^ hash_bits(function.get_code_size());
					}
					case ::gal::object_type::STRING_TYPE:
					{
						return hash<::gal::object_string>{}(dynamic_cast<const ::gal::object_string&>(*obj));
					}
					default:
					{
						// "Only immutable objects can be hashed."
						UNREACHABLE();
					}
				}
			}
			else
			{
				return hash_bits(value.data_);
			}
		}
	};
}// namespace std

namespace gal
{
	/**
	  * @brief A loaded module and the top-level variables it defines.
	  *
	  * While this is an object and is managed by the GC, it never appears as a
	  * first-class object in GAL.
	  */
	class object_module : public object
	{
	private:
		// using variables_buffer_type							   = std::unordered_map<gal_size_type, std::pair<object_string, magic_value>, std::hash<gal_size_type>, std::equal_to<>, gal_allocator<std::pair<const gal_size_type, std::pair<object_string, magic_value>>>>;
		using variables_buffer_type							   = std::map<gal_size_type, std::pair<object_string, magic_value>, std::less<>, gal_allocator<std::pair<const gal_size_type, std::pair<object_string, magic_value>>>>;

		using value_type									   = variables_buffer_type::value_type;
		// variable index
		using key_type										   = variables_buffer_type::key_type;
		// pair of variable_name-variable
		using mapped_type									   = variables_buffer_type::mapped_type;
		using size_type										   = variables_buffer_type::size_type;

		using pointer										   = variables_buffer_type::pointer;
		using const_poiner									   = variables_buffer_type::const_pointer;

		using reference										   = variables_buffer_type::reference;
		using const_reference								   = variables_buffer_type::const_reference;

		using iterator										   = variables_buffer_type::iterator;
		using const_iterator								   = variables_buffer_type::const_iterator;

		constexpr static key_type variable_already_defined	   = -1;
		constexpr static key_type variable_too_many_defined	   = -2;
		constexpr static key_type variable_used_before_defined = -3;

	private:
		/**
		  * @brief The currently defined top-level variables.
		  */
		variables_buffer_type variables_;

		/**
		  * @brief The name of the module.
		  */
		object_string		  name_;

	public:
		/**
		 * @brief Creates a new module.
		 */
		explicit object_module(object_string name)
			// Modules are never used as first-class objects, so don't need a class.
			: object{object_type::MODULE_TYPE, nullptr},
			  name_{std::move(name)}
		{
		}

		[[nodiscard]] const object_string& get_name() const noexcept
		{
			return name_;
		}

		[[nodiscard]] size_type size() const noexcept
		{
			return variables_.size();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return variables_.begin();
		}

		[[nodiscard]] const_iterator begin() const noexcept
		{
			return variables_.begin();
		}

		[[nodiscard]] iterator end() noexcept
		{
			return variables_.end();
		}

		[[nodiscard]] const_iterator end() const noexcept
		{
			return variables_.end();
		}

		void clear()
		{
			variables_.clear();
		}

		/**
		  * @brief Looks up a variable from the module.
		  *
		  * Returns `magic_value_undefined` if not found.
		  */
		[[nodiscard]] magic_value get_variable(const object_string& name) const;

		[[nodiscard]] magic_value get_variable(object_string::const_pointer name) const;

		/**
		  * @brief Looks up a variable from the module.
		  *
		  * Returns `magic_value_undefined` if not found.
		  */
		[[nodiscard]] magic_value get_variable(key_type index) const;

		/**
		  * @brief Looks up a variable from the module.
		  *
		  * Returns `gal_size_not_exist` if not found.
		  */
		[[nodiscard]] key_type	  get_variable_index(const object_string& name) const;

		[[nodiscard]] key_type	  get_variable_index(object_string::const_pointer name) const;

		/**
		 * @brief Change a variable in Module to another, and do nothing if the target variable does not exist.
		 */
		void					  set_variable(const object_string& name, magic_value value);

		void					  set_variable(object_string::const_pointer name, magic_value value);

		/**
		 * @brief Change a variable in Module to another, and do nothing if the target variable does not exist.
		 */
		void					  set_variable(key_type index, magic_value value);

		/**
		  * @brief Adds a new implicitly declared top-level variable named [name] to module
		  * based on a use site occurring on [line].
		  *
		  * Does not check to see if a variable with that name is already declared or
		  * defined. Returns the symbol for the new variable or `variable_too_many_defined`
		  * if there are too many variables defined.
		  */
		key_type				  declare_variable(const object_string& name, int line)
		{
			if (variables_.size() > max_module_variables)
			{
				return variable_too_many_defined;
			}

			// Implicitly defined variables get a "value" that is the line where the
			// variable is first used. We'll use that later to report an error on the
			// right line.
			return variables_.emplace(variables_.size(), std::make_pair(name, magic_value{static_cast<magic_value::value_type>(line)})).first->first;
		}

		/**
		  * @brief Adds a new top-level variable named [name] to [this], and optionally
		  * populates line with the line of the implicit first use (line can be nullptr).
		  *
		  * Returns the symbol for the new variable, `variable_already_defined` if a variable
		  * with the given name is already defined, or `variable_too_many_defined` if there
		  * are too many variables defined. Returns `variable_used_before_defined` if this is
		  * a top-level lowercase variable (local name) that was used before being defined.
		  */
		key_type define_variable(const object_string& name, magic_value value, int* line = nullptr);

		/**
		  * @brief Adds a new top-level variable named [name] to [this], and optionally
		  * populates line with the line of the implicit first use (line can be nullptr).
		  *
		  * Returns the symbol for the new variable, `variable_already_defined` if a variable
		  * with the given name is already defined, or `variable_too_many_defined` if there
		  * are too many variables defined. Returns `variable_used_before_defined` if this is
		  * a top-level lowercase variable (local name) that was used before being defined.
		  *
		  * C++20 should support it
		  */
		// gal_index_type define_variable(const char* name, magic_value value, int* line = nullptr);

		/**
		 * @brief Import all variables from another module.
		 *
		 * todo: Should we use insert? Can we ensure that the target module does not contain any variables defined in this module?
		 */
		void	 copy_variables(const object_module& other)
		{
			for (const auto& [index, kv]: other.variables_)
			{
				define_variable(kv.first, kv.second);
			}
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
			// variables
		}
	};

	/**
	 * @brief A hash table mapping keys to values.
	 */
	class object_map : public object
	{
	public:
		// using map_buffer_type = std::unordered_map<magic_value, magic_value, std::hash<magic_value>, std::equal_to<>, gal_allocator<std::pair<const magic_value, magic_value>>>;
		using map_buffer_type = std::map<magic_value, magic_value, std::less<>, gal_allocator<std::pair<const magic_value, magic_value>>>;

		using value_type	  = map_buffer_type::value_type;
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

		[[nodiscard]] size_type size() const noexcept
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

		void clear()
		{
			entries_.clear();
		}

		/**
		 * @brief Looks up [key] in [map]. If found, returns the value. Otherwise, returns
		 * `magic_value_undefined`.
		 */
		[[nodiscard]] magic_value get(magic_value key) const
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

		[[nodiscard]] const_iterator find(magic_value key) const
		{
			return entries_.find(key);
		}

		/**
		 * @brief Removes [key] from [map], if present. Returns the value for the key if found
		 * or `magic_value_null` otherwise.
		 */
		magic_value remove(magic_value key)
		{
			auto it = find(key);
			if (it == entries_.end())
			{
				return magic_value_null;
			}

			auto value = it->second;

			entries_.erase(it);

			return value;
		}

		[[nodiscard]] gal_size_type memory_usage() const noexcept override
		{
			// todo
			return 0;
		}

	private:
		void destroy() override
		{
			// todo
		}
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
