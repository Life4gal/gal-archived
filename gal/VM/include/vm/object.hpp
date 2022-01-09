#pragma once

#include <gal.hpp>
#include <cstdint>
#include <variant>
#include <array>
#include <string>
#include <vector>
#include <utils/assert.hpp>
#include <utils/concept.hpp>
#include <utils/macro.hpp>
#include <vm/allocator.hpp>

#ifndef GAL_LANG_VM_OBJECT_HPP
#define GAL_LANG_VM_OBJECT_HPP

namespace gal::vm
{
	class magic_value;

	/**
	 * @brief Collectable objects
	 */
	class gal_string;
	class gal_user_data;
	class gal_prototype;
	class gal_upvalue;
	class gal_closure;
	class gal_table;
	class thread_state;
	struct global_state;

	/**
	 * @brief Base struct for all heap-allocated objects.
	 */
	class object
	{
		friend struct memory_manager;
	public:
		using mark_type = std::uint8_t;
		/**
		 * @brief Layout for bit use in `marked' field:
		 * bit 0 - object is white (type 0)
		 * bit 1 - object is white (type 1)
		 * bit 2 - object is black
		 * bit 3 - object is fixed (should not be collected)
		 */
		constexpr static mark_type mark_white_bit0 = 0;
		constexpr static mark_type mark_white_bit1 = 1;
		constexpr static mark_type mark_black_bit = 2;
		constexpr static mark_type mark_fixed_bit = 3;

		constexpr static mark_type mark_white_bits_mask = (1 << mark_white_bit0) | (1 << mark_white_bit1);
		constexpr static mark_type mark_black_bit_mask = 1 << mark_black_bit;
		constexpr static mark_type mark_fixed_bit_mask = 1 << mark_fixed_bit;

		constexpr static mark_type mask_marks = ~(mark_white_bits_mask | mark_black_bit_mask);

	private:
		object* next_;
		object_type type_;
		mark_type marked_;
		memory_categories_type memory_category_;

		constexpr virtual void do_mark(global_state& state) = 0;
	protected:
		constexpr explicit object(const object_type type, const memory_categories_type category = default_memory_category) noexcept
			: next_{nullptr},
			  type_{type},
			  marked_{0},
			  memory_category_{category} {}

		/**
		 * @brief Destroy all dynamically allocated objects in the class, generally called before the object will be recycled (destructed)
		 *
		 * Usually, this function does not need to do anything (because the memory is managed by the STL components),
		 * but if the class contains a container for storing polymorphic objects (such as map),
		 * then you need to call this function to destroy all of them.
		 */
		virtual ~object() noexcept = 0;

		object(const object&) = default;
		object& operator=(const object&) = default;
		object(object&&) = default;
		object& operator=(object&&) = default;

	public:
		[[nodiscard]] constexpr bool has_next() const noexcept { return next_; }

		[[nodiscard]] constexpr object* get_next() noexcept { return next_; }

		[[nodiscard]] constexpr object* get_next() const noexcept { return next_; }

		constexpr void link_next(object* next) noexcept { next_ = next; }

		[[nodiscard]] constexpr object_type type() const noexcept { return type_; }

		[[nodiscard]] constexpr mark_type get_mark() const noexcept { return marked_; }

		constexpr void set_mark(const mark_type mark) noexcept { marked_ = mark; }

		[[nodiscard]] constexpr bool is_mark_white() const noexcept { return marked_ & mark_white_bits_mask; }

		[[nodiscard]] constexpr bool is_mark_black() const noexcept { return marked_ & mark_black_bit_mask; }

		[[nodiscard]] constexpr bool is_mark_gray() const noexcept { return not(is_mark_white() || is_mark_black()); }

		[[nodiscard]] constexpr bool is_mark_fixed() const noexcept { return marked_ & mark_fixed_bit_mask; }

		constexpr void set_mark_another_white() noexcept { marked_ ^= mark_white_bits_mask; }

		constexpr void set_mark_gray_to_black() noexcept { marked_ |= mark_black_bit_mask; }

		constexpr void set_mark_white_to_gray() noexcept { marked_ &= ~mark_white_bits_mask; }

		constexpr void set_mark_black_to_gray() noexcept { marked_ &= ~mark_black_bit_mask; }

		[[nodiscard]] constexpr memory_categories_type get_category() const noexcept { return memory_category_; }

		[[nodiscard]] explicit operator magic_value() const noexcept;

		constexpr void mark(global_state& state);

		constexpr void try_mark(global_state& state) { if (is_mark_white()) { mark(state); } }

		constexpr virtual void destroy() = 0;
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
	 * The details of how these are used to represent numbers are not really
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

	class magic_value final
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
		[[maybe_unused]] constexpr static value_type tag_reserve1{5};
		[[maybe_unused]] constexpr static value_type tag_reserve2{6};
		[[maybe_unused]] constexpr static value_type tag_reserve3{7};

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
		 * Intel¡¯s "QNaN Floating-Point Indefinite" value:
		 *      For the floating-point data type encodings (single-precision, double-precision, and double-extended-precision),
		 *      one unique encoding (a QNaN) is reserved for representing the special value QNaN floating-point indefinite.
		 *      The x87 FPU and the SSE/SSE2/SSE3/SSE4.1/AVX extensions return these indefinite values as responses to some
		 *      masked floating-point exceptions.
		 *
		 *      https://software.intel.com/content/dam/develop/external/us/en/documents/floating-point-reference-sheet-v2-13.pdf
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
		constexpr explicit magic_value() noexcept
			: data_{null_val} {}

		constexpr explicit magic_value(const value_type data) noexcept
			: data_{data} {}

		constexpr explicit magic_value(const boolean_type b) noexcept
			: data_{b ? true_val : false_val} {}

		constexpr explicit magic_value(const number_type d) noexcept
			: data_{std::bit_cast<value_type>(d)} {}

		explicit magic_value(const object* obj) noexcept
			: data_{reinterpret_cast<std::uintptr_t>(obj)} {}

		[[nodiscard]] constexpr value_type get_data() const noexcept { return data_; }

		/**
		 * @brief Gets the singleton type tag for a magic_value (which must be a singleton).
		 */
		[[nodiscard]] constexpr value_type get_tag() const noexcept { return data_ & tag_mask; }

		/**
		 * @brief If the NaN bits are set, it's not a number.
		 */
		[[nodiscard]] constexpr bool is_number() const noexcept { return (data_ & quiet_nan) != quiet_nan; }

		/**
		 * @brief An object pointer is a NaN with a set sign bit.
		 */
		[[nodiscard]] constexpr bool is_object() const noexcept { return (data_ & pointer_mask) == pointer_mask; }

		[[nodiscard]] constexpr bool is_null() const noexcept { return data_ == null_val; }
		[[nodiscard]] constexpr bool is_false() const noexcept { return data_ == false_val; }
		[[nodiscard]] constexpr bool is_true() const noexcept { return data_ == true_val; }
		[[nodiscard]] constexpr bool is_undefined() const noexcept { return data_ == undefined_val; }

		[[nodiscard]] constexpr bool is_boolean() const noexcept { return is_true() || is_false(); }
		[[nodiscard]] constexpr bool is_falsy() const noexcept { return is_false() || is_null(); }
		[[nodiscard]] constexpr bool is_empty() const noexcept { return is_null(); }

		/**
		 * @brief Value -> 0 or 1.
		 */
		[[nodiscard]] constexpr bool as_boolean() const noexcept { return data_ == true_val; }
		[[nodiscard]] constexpr number_type as_number() const noexcept { return std::bit_cast<number_type>(data_); }

		/**
		 * @brief Value -> object*.
		 */
		[[nodiscard]] object* as_object() const noexcept
		{
			gal_assert(is_object());
			return reinterpret_cast<object*>((data_ & ~pointer_mask));
		}

		/**
		 * @brief Returns true if [value] is an object of type [type].
		 */
		[[nodiscard]] bool is_object(const object_type type) const noexcept { return is_object() && as_object()->type() == type; }

		// todo
		[[nodiscard]] constexpr bool is_light_user_data() const noexcept { return is_object(object_type::light_user_data); }

		[[nodiscard]] constexpr bool is_string() const noexcept { return is_object(object_type::string); }
		[[nodiscard]] constexpr bool is_table() const noexcept { return is_object(object_type::table); }
		[[nodiscard]] constexpr bool is_function() const noexcept { return is_object(object_type::function); }
		[[nodiscard]] constexpr bool is_user_data() const noexcept { return is_object(object_type::user_data); }
		[[nodiscard]] constexpr bool is_thread() const noexcept { return is_object(object_type::thread); }

		// todo
		[[nodiscard]] inline user_data_type as_light_user_data() const noexcept;

		[[nodiscard]] inline gal_string* as_string() const noexcept;
		[[nodiscard]] inline gal_table* as_table() const noexcept;
		[[nodiscard]] inline gal_closure* as_function() const noexcept;
		[[nodiscard]] inline gal_user_data* as_user_data() const noexcept;
		[[nodiscard]] inline thread_state* as_thread() const noexcept;

		constexpr void copy_magic_value(const global_state& state, magic_value target) noexcept;

		constexpr void mark(global_state& state) const noexcept { if (is_object()) { as_object()->try_mark(state); } }

		/**
		 * @brief Returns true if [lhs] and [rhs] are strictly the same value. This is identity
		 * for object values, and value equality for un-boxed values.
		 */
		friend bool operator==(const magic_value& lhs, const magic_value& rhs)
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

	inline object::operator magic_value() const noexcept { return magic_value{this}; }

	/**
	 * @brief index to stack elements
	 */
	using stack_index_type = magic_value*;

	/**
	 * @brief String headers for string table
	 */
	class gal_string final : public object
	{
	private:
		std::int16_t atomic_;
		std::uint32_t hash_;
		std::string data_;

		constexpr void do_mark(global_state& state) override
		{
			// do nothing
		}

	public:
		// todo interface
	};

	class gal_user_data final : public object
	{
	private:
		user_data_tag_type tag_;
		gal_table* meta_table_;
		std::vector<char> data_;

		constexpr void do_mark(global_state& state) override;

	public:
		// todo: interface
	};

	struct gal_local_var
	{
		gal_string* name;
		// first point where variable is active
		compiler::debug_pc_type begin_pc;
		// first point where variable is dead
		compiler::debug_pc_type end_pc;
		// register slot, relative to base, where variable is stored
		compiler::register_type reg;
	};

	/**
	 * @brief Function Prototypes
	 */
	class gal_prototype final : public object
	{
	private:
		// constants used by the function
		magic_value key_;
		// function bytecode
		compiler::operand_underlying_type* code_;
		// functions defined inside the function
		gal_prototype** parent_;
		// for each instruction, line number as a delta from baseline
		compiler::baseline_delta_type* line_info_;
		// baseline line info, one entry for each 1 << line_gap_log2 instructions; allocated after line_info
		int* abs_line_info_;
		// information about local variables
		gal_local_var* local_var_;
		// upvalue names
		gal_string** upvalues_;
		gal_string* source_;

		gal_string* debug_name_;
		// a copy of code[] array with just operands
		compiler::operand_abc_underlying_type* debug_instruction_;

		object* gc_list_;

		compiler::operand_abc_underlying_type num_upvalues_;
		compiler::operand_abc_underlying_type num_params_;
		compiler::operand_abc_underlying_type is_vararg_;
		compiler::operand_abc_underlying_type max_stack_size_;

		constexpr void do_mark(global_state& state) override;

	public:
		// todo: interface
	};

	class gal_upvalue final : public object
	{
	private:
		// points to stack or to its own value
		stack_index_type value_;

		union upvalue_state
		{
			// the value (when closed)
			magic_value closed{};

			struct upvalue_link
			{
				// double linked list (when open)
				gal_upvalue* prev{nullptr};
				gal_upvalue* next{nullptr};
			} link;
		} upvalue_;

		constexpr void do_mark(global_state& state) override
		{
			value_->mark(state);
			// closed?
			if (*value_ == upvalue_.closed)
			{
				// open upvalues are never black
				set_mark_gray_to_black();
			}
		}

	public:
		[[nodiscard]] constexpr magic_value* get_index() noexcept { return value_; }

		[[nodiscard]] constexpr const magic_value* get_index() const noexcept { return value_; }

		[[nodiscard]] constexpr magic_value get_close_value() const noexcept { return upvalue_.closed; }

		constexpr void close(const global_state& state)
		{
			upvalue_.closed.copy_magic_value(state, *value_);
			// now current value lives here
			value_ = &upvalue_.closed;
		}

		GAL_ASSERT_CONSTEXPR void unlink()
		{
			gal_assert(upvalue_.link.next->upvalue_.link.prev == this);
			gal_assert(upvalue_.link.prev->upvalue_.link.next = this);

			upvalue_.link.next->upvalue_.link.prev = upvalue_.link.prev;
			upvalue_.link.prev->upvalue_.link.next = upvalue_.link.next;
		}

		void destroy() override
		{
			// is it open?
			if (*value_ != upvalue_.closed)
			{
				// remove from open list
				unlink();
			}

			// free upvalue
			vm_allocator<gal_upvalue> allocator{*this};
			allocator.deallocate(this, 1);
		}
	};

	class gal_closure final : public object
	{
	private:
		compiler::operand_abc_underlying_type is_internal_;
		compiler::operand_abc_underlying_type num_upvalues_;
		compiler::operand_abc_underlying_type stack_size_;
		compiler::operand_abc_underlying_type is_preload_;

		object* gc_list_;
		gal_table* environment_;

		struct internal_type
		{
			internal_function_type function;
			continuation_type continuation;
			const char* debug_name;
			magic_value upvalues[1];
		};

		struct gal_type
		{
			gal_prototype* proto;
			magic_value upreferences[1];
		};

		union function_type
		{
			internal_type internal;
			gal_type gal;
		} function_;

		constexpr void do_mark(global_state& state) override;
	public:
		[[nodiscard]] constexpr bool is_internal() const noexcept { return is_internal_; }

		// todo: interface
	};

	class gal_table final : public object
	{
	public:
		struct table_node
		{
			struct node_key
			{
				magic_value value;
				// for chaining
				index_type next;
			};

			using node_value = magic_value;
		};

	private:
		// 1 << p means tag_method(p) is not present
		compiler::operand_abc_underlying_type flags_;
		// sand-box feature to prohibit writes to table
		compiler::operand_abc_underlying_type immutable_;
		// environment does not share globals with other scripts
		compiler::operand_abc_underlying_type sharable_;
		// log2 of size of `node' array
		compiler::operand_abc_underlying_type node_size_;
		// (1 << node_size)-1, truncated to 8 bits
		compiler::operand_abc_underlying_type node_mask8_;

		int array_size_;

		union
		{
			// any free position is before this position
			int last_free;
			// negated 'boundary' of `array' array; if array_boundary < 0
			int array_boundary;
		};

		gal_table* meta_table_;
		magic_value* array_;
		table_node* node_;
		object* gc_list_;

		constexpr void do_mark(global_state& state) override;

	public:
		// todo: interface
		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }
	};

	constexpr void gal_user_data::do_mark(global_state& state)
	{
		// user data are never gray
		set_mark_gray_to_black();
		if (meta_table_) { meta_table_->try_mark(state); }
	}

	inline gal_string* magic_value::as_string() const noexcept
	{
		gal_assert(is_string());
		return dynamic_cast<gal_string*>(as_object());
	}

	inline gal_table* magic_value::as_table() const noexcept
	{
		gal_assert(is_table());
		return dynamic_cast<gal_table*>(as_object());
	}

	inline gal_closure* magic_value::as_function() const noexcept
	{
		gal_assert(is_function());
		return dynamic_cast<gal_closure*>(as_object());
	}

	inline gal_user_data* magic_value::as_user_data() const noexcept
	{
		gal_assert(is_user_data());
		return dynamic_cast<gal_user_data*>(as_object());
	}
}

#endif // GAL_LANG_VM_OBJECT_HPP
