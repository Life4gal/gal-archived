#pragma once

#ifndef GAL_LANG_VM_OBJECT_HPP
#define GAL_LANG_VM_OBJECT_HPP

#include <gal.hpp>
#include <vm/allocator.hpp>
#include <utils/assert.hpp>
#include <utils/hash_container.hpp>
#include <vector>
#include <vm/tagged_method.hpp>
#include <utils/enum_utils.hpp>

namespace gal::vm_dev
{
	class magic_value;

	class object_string;
	class object_user_data;
	class object_prototype;
	class object_upvalue;
	class object_closure;
	class object_table;

	/**
	 * @brief Base struct for all heap-allocated objects.
	 */
	class object
	{
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

		constexpr virtual void do_mark([[maybe_unused]] main_state& state) = 0;

		constexpr virtual void do_destroy([[maybe_unused]] main_state& state) = 0;

	protected:
		constexpr explicit object(const object_type type, const mark_type mark = 0, object* next = nullptr)
			: next_{next},
			  type_{type},
			  marked_{mark} {}

		virtual ~object() noexcept = 0;

		object(const object&) = default;
		object& operator=(const object&) = default;
		object(object&&) = default;
		object& operator=(object&&) = default;

	public:
		[[nodiscard]] constexpr bool has_next() const noexcept { return next_; }

		[[nodiscard]] constexpr object*& get_next() noexcept { return next_; }

		[[nodiscard]] constexpr const object* get_next() const noexcept { return next_; }

		GAL_ASSERT_CONSTEXPR void link_next(object* next) noexcept
		{
			gal_assert(not next_, "Should not link an object that already exists `next`");
			next_ = next;
		}

		constexpr void reset_next(object* next) noexcept { next_ = next; }

		[[nodiscard]] constexpr object_type type() const noexcept { return type_; }

		constexpr void set_type(const object_type type) noexcept { type_ = type; }

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

		[[nodiscard]] explicit operator magic_value() const noexcept;

		GAL_ASSERT_CONSTEXPR void mark(main_state& state);
		constexpr void try_mark(main_state& state) { if (is_mark_white()) { mark(state); } }

		constexpr void delete_chain(main_state& state, object* end);

		/**
		 * @brief Tells whether a key or value can be cleared from
		 * a weak table. Non-collectable objects are never removed from weak
		 * tables. Strings behave as `values`, so are never removed too. for
		 * other objects: if really collected, cannot keep them..
		 */
		constexpr bool is_object_cleared();

		template<typename T, typename... Args>
			requires std::is_base_of_v<object, T>
		[[nodiscard]] constexpr static T* create(main_state& state, Args&&... args)
		{
			// construct object
			vm_allocator<T> allocator{state};

			auto ptr = allocator.allocate(1);
			allocator.construct(ptr, std::forward<Args>(args)...);

			return ptr;
		}

		template<typename T>
			requires std::is_base_of_v<object, T>
		constexpr static void destroy(main_state& state, T* ptr)
		{
			// free object
			ptr->do_destroy(state);

			vm_allocator<T> allocator{state};
			allocator.destroy(ptr);
			allocator.deallocate(ptr, 1);
		}

		[[nodiscard]] constexpr virtual std::size_t memory_usage() const noexcept = 0;
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

		[[nodiscard]] constexpr bool is_string() const noexcept { return is_object(object_type::string); }
		[[nodiscard]] constexpr bool is_table() const noexcept { return is_object(object_type::table); }
		[[nodiscard]] constexpr bool is_function() const noexcept { return is_object(object_type::function); }
		[[nodiscard]] constexpr bool is_user_data() const noexcept { return is_object(object_type::user_data); }
		[[nodiscard]] constexpr bool is_thread() const noexcept { return is_object(object_type::thread); }

		[[nodiscard]] inline object_string* as_string() const noexcept;
		[[nodiscard]] inline object_table* as_table() const noexcept;
		[[nodiscard]] inline object_closure* as_function() const noexcept;
		[[nodiscard]] inline object_user_data* as_user_data() const noexcept;
		[[nodiscard]] inline child_state* as_thread() const noexcept;

		constexpr void copy_magic_value(const main_state& state, magic_value target) noexcept;

		constexpr void mark(main_state& state) const noexcept { if (is_object()) { as_object()->try_mark(state); } }

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
		 * (null, booleans, numbers, strings) are equal if they have the
		 * same data. All other values are equal if they are identical objects.
		 */
		[[nodiscard]] bool equal(const magic_value& other) const;
	};

	constexpr magic_value magic_value_null{magic_value::null_val};
	constexpr magic_value magic_value_false{magic_value::false_val};
	constexpr magic_value magic_value_true{magic_value::true_val};
	constexpr magic_value magic_value_undefined{magic_value::undefined_val};

	inline object::operator magic_value() const noexcept { return magic_value{this}; }

	using stack_element_type = magic_value*;

	class object_string final : public object
	{
	public:
		using atomic_type = std::int16_t;
		using hash_type = std::uint32_t;
		using data_type = std::basic_string<char, std::char_traits<char>, vm_allocator<char>>;

	private:
		atomic_type atomic_;
		hash_type hash_;
		data_type data_;

		void do_mark(main_state& state) override
		{
			/* nothing need to do*/
		}

		void do_destroy(main_state& state) override;

	public:
		object_string(const hash_type hash, data_type&& data)
			: object{object_type::string},
			  atomic_{0},
			  hash_{hash},
			  data_{std::move(data)} {}

		[[nodiscard]] constexpr atomic_type get_atomic() const noexcept { return atomic_; }

		[[nodiscard]] constexpr hash_type get_hash() const noexcept { return hash_; }

		[[nodiscard]] constexpr const data_type& get_data() const noexcept { return data_; }

		constexpr void mark() noexcept { set_mark_white_to_gray(); }
	};

	class object_user_data final : public object
	{
	public:
		using data_type = std::uint8_t;
		using data_container_type = std::vector<data_type, vm_allocator<data_type>>;

	private:
		user_data_tag_type tag_;
		object_table* meta_table_;
		// If user_data has an inline_destructor, we always assume it is at the end of the data and of type gc_handler::user_data_gc_handler
		// [xxx...xxx...xxx...xxx destructor-pointer]
		// ^......read data......^^.....gc-pointer.....^
		data_container_type data_;

		constexpr void do_mark(main_state& state) override;

		void do_destroy(main_state& state) override;

	public:
		object_user_data(const user_data_tag_type tag, data_container_type&& data, object_table* meta_table = nullptr)
			: object{object_type::user_data},
			  tag_{tag},
			  meta_table_{meta_table},
			  data_{std::move(data)} {}

		[[nodiscard]] constexpr user_data_tag_type get_tag() const noexcept { return tag_; }

		[[nodiscard]] constexpr data_type* get_data() noexcept { return data_.data(); }

		[[nodiscard]] constexpr const data_type* get_data() const noexcept { return data_.data(); }

		constexpr void set_meta_table(object_table* meta_table) { meta_table_ = meta_table; }
	};

	/**
	 * @brief Function Prototypes
	 */
	class object_prototype final : public object
	{
	public:
		struct local_variable
		{
			object_string* name = nullptr;
			// first point where variable is active
			compiler::debug_pc_type begin_pc = 0;
			// first point where variable is dead
			compiler::debug_pc_type end_pc = 0;
			// register slot, relative to base, where variable is stored
			compiler::register_type reg = 0;
		};

		using constant_container_type = std::vector<magic_value, vm_allocator<magic_value>>;
		using instruction_container_type = std::vector<instruction_type, vm_allocator<instruction_type>>;
		using parent_prototype_container_type = std::vector<object_prototype*, vm_allocator<object_prototype*>>;
		using line_info_container_type = std::vector<compiler::baseline_delta_type, vm_allocator<compiler::baseline_delta_type>>;
		using local_variable_container_type = std::vector<local_variable, vm_allocator<local_variable>>;
		using upvalue_name_container_type = std::vector<object_string*, vm_allocator<object_string*>>;
		using debug_instruction_container_type = std::vector<compiler::operand_abc_underlying_type, vm_allocator<compiler::operand_abc_underlying_type>>;

	private:
		// constants used by the function
		constant_container_type constants_;
		// function bytecode
		instruction_container_type code_;
		// functions defined inside the function
		parent_prototype_container_type children_;
		// for each instruction, line number as a delta from baseline
		line_info_container_type line_info_;
		// baseline line info, one entry for each 1 << line_gap_log2_ instructions; point to line_info_
		decltype(line_info_.data()) abs_line_info_;
		int line_gap_log2_;
		// information about local variables
		local_variable_container_type local_variables_;
		// upvalue names
		upvalue_name_container_type upvalue_names_;

		object_string* source_;
		object_string* debug_name_;
		// a copy of code_ with just operands
		debug_instruction_container_type debug_instructions_;

		object* gc_list_;

		compiler::operand_abc_underlying_type num_upvalues_;
		compiler::operand_abc_underlying_type num_params_;
		compiler::operand_abc_underlying_type is_vararg_;
		compiler::operand_abc_underlying_type max_stack_size_;

		void do_mark(main_state& state) override
		{
			// todo
		}

		void do_destroy(main_state& state) override
		{
			constants_.clear();
			code_.clear();
			children_.clear();
			line_info_.clear();
			local_variables_.clear();
			upvalue_names_.clear();
			debug_instructions_.clear();
		}

	public:
		// todo: ctor

		[[nodiscard]] constexpr std::size_t memory_usage() const noexcept override
		{
			return sizeof(object_prototype) +
			       sizeof(magic_value) * constants_.size() +
			       sizeof(instruction_type) * code_.size() +
			       sizeof(object_prototype*) * children_.size() +
			       sizeof(line_info_container_type::value_type) * line_info_.size() +
			       sizeof(local_variable) * local_variables_.size() +
			       sizeof(object_string*) * upvalue_names_.size();
		}

		/**
		 * @brief All marks are conditional because a GC may happen while the
		 * prototype is still being created.
		 */
		void traverse(main_state& state);

		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }

		[[nodiscard]] constexpr object* get_gc_list() noexcept { return gc_list_; }

		[[nodiscard]] constexpr const object* get_gc_list() const noexcept { return gc_list_; }
	};

	class object_upvalue final : public object
	{
	private:
		// points to stack or to its own value
		stack_element_type value_;

		union upvalue_state
		{
			// the value (when closed)
			magic_value closed{};

			struct upvalue_link
			{
				// double linked list (when open)
				object_upvalue* prev{nullptr};
				object_upvalue* next{nullptr};
			} link;
		} upvalue_;

		void do_mark(main_state& state) override
		{
			// todo
		}

		void do_destroy(main_state& state) override
		{
			// is it open?
			if (*value_ != upvalue_.closed)
			{
				// remove from open list
				unlink();
			}
		}

	public:
		// ReSharper disable once CppParameterMayBeConst
		constexpr object_upvalue(stack_element_type value, object_upvalue& next)
			: object{object_type::upvalue},
			  value_{value},
			  upvalue_{.link = {.prev = nullptr, .next = &next}} { next.upvalue_.link.prev = this; }

		std::size_t memory_usage() const noexcept override
		{
			// todo
		}

		[[nodiscard]] constexpr magic_value* get_index() noexcept { return value_; }

		[[nodiscard]] constexpr const magic_value* get_index() const noexcept { return value_; }

		[[nodiscard]] constexpr magic_value get_close_value() const noexcept { return upvalue_.closed; }

		constexpr void close(const main_state& state)
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

		GAL_ASSERT_CONSTEXPR std::size_t remark(main_state& state) noexcept
		{
			std::size_t work = 0;
			for (auto* upvalue = upvalue_.link.next; upvalue != this; upvalue = upvalue->upvalue_.link.next)
			{
				work += sizeof(object_upvalue);
				gal_assert(upvalue->upvalue_.link.next->upvalue_.link.prev == upvalue);
				gal_assert(upvalue->upvalue_.link.prev->upvalue_.link.next == upvalue);
				if (upvalue->is_mark_gray()) { upvalue->try_mark(state); }
			}
			return work;
		}

		/**
		 * @return Return the end of the list.
		 */
		[[nodiscard]] object* close_until(main_state& state, stack_element_type level);
	};

	class object_closure final : public object
	{
	public:
		struct internal_type
		{
			using upvalue_container_type = std::vector<magic_value, vm_allocator<magic_value>>;

			internal_function_type function;
			continuation_function_type continuation;
			const char* debug_name;
			upvalue_container_type upvalues;
		};

		struct gal_type
		{
			using upreference_container_type = std::vector<magic_value, vm_allocator<magic_value>>;

			object_prototype* prototype;
			upreference_container_type upreferences;
		};

	private:
		compiler::operand_abc_underlying_type is_internal_;
		compiler::operand_abc_underlying_type stack_size_;
		compiler::operand_abc_underlying_type is_preload_;

		object* gc_list_;
		object_table* environment_;

		union function_type
		{
			internal_type internal;
			gal_type gal;
		} function_;

		void do_mark(main_state& state) override
		{
			// todo
		}

		void do_destroy(main_state& state) override
		{
			if (is_internal()) { function_.internal.upvalues.clear(); }
			else { function_.gal.upreferences.clear(); }
		}

	public:
		// todo: ctor

		[[nodiscard]] constexpr std::size_t memory_usage() const noexcept override
		{
			// todo: remove them
			static_assert(offsetof(object_closure, is_internal_) == sizeof(object));// 24
			static_assert(offsetof(object_closure, stack_size_) == sizeof(object) + 1);
			static_assert(offsetof(object_closure, is_preload_) == sizeof(object) + 2);
			static_assert(offsetof(object_closure, gc_list_) == sizeof(object) + 8);
			static_assert(offsetof(object_closure, environment_) == sizeof(object) + 16);

			static_assert(offsetof(object_closure, function_.internal) == sizeof(object) + 24);
			static_assert(offsetof(object_closure, function_.internal.function) == sizeof(object) + 24);
			static_assert(offsetof(object_closure, function_.internal.continuation) == sizeof(object) + 32);
			static_assert(offsetof(object_closure, function_.internal.debug_name) == sizeof(object) + 40);
			static_assert(offsetof(object_closure, function_.internal.upvalues) == sizeof(object) + 48);

			static_assert(offsetof(object_closure, function_.gal) == sizeof(object) + 24);
			static_assert(offsetof(object_closure, function_.gal.prototype) == sizeof(object) + 24);
			static_assert(offsetof(object_closure, function_.gal.upreferences) == sizeof(object) + 32);

			return is_internal()
				       ? (
					       offsetof(object_closure, function_.internal.upvalues) + sizeof(magic_value) * function_.internal.upvalues.size())
				       : (offsetof(object_closure, function_.gal.upreferences) + sizeof(magic_value) * function_.gal.upreferences.size());
		}

		[[nodiscard]] constexpr bool is_internal() const noexcept { return is_internal_; }

		void traverse(main_state& state);

		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }

		[[nodiscard]] constexpr object* get_gc_list() noexcept { return gc_list_; }

		[[nodiscard]] constexpr const object* get_gc_list() const noexcept { return gc_list_; }
	};
}// namespace gal::vm_dev

template<>
struct std::hash<::gal::vm_dev::magic_value>
{
	std::size_t operator()(const ::gal::vm_dev::magic_value& value) const noexcept
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

		if (value.is_string()) { return value.as_string()->get_hash(); }

		// todo

		return hash_bits(value.get_data());
	}
};

namespace gal::vm_dev
{
	class object_table final : public object
	{
	public:
		using node_container_type = utils::hash_map<magic_value, magic_value, std::hash<magic_value>, std::equal_to<>, vm_allocator<std::pair<const magic_value, magic_value>>>;
		using map_iterator = node_container_type::iterator;
		using map_const_iterator = node_container_type::const_iterator;

		using flag_type = std::uint8_t;

	private:
		// 1 << p means tagged_method(p) is not present
		flag_type flags_;
		// sand-box feature to prohibit writes to table
		bool immutable_;
		// environment does not share globals with other scripts
		bool sharable_;

		object_table* meta_table_;
		object* gc_list_;

		node_container_type nodes_;

		void do_mark(main_state& state) override
		{
			// todo
		}

		void do_destroy(main_state& state) override
		{
			// todo
			nodes_.clear();
		}

	public:
		// todo: ctor

		[[nodiscard]] constexpr std::size_t memory_usage() const noexcept override { return sizeof(object_table) + sizeof(node_container_type::value_type) * nodes_.size(); }

		[[nodiscard]] constexpr bool check_flag(const flag_type flag) const noexcept { return flags_ & (1 << flag); }

		[[nodiscard]] constexpr bool check_flag(const tagged_method_type flag) const noexcept { return check_flag(static_cast<flag_type>(flag)); }

		[[nodiscard]] constexpr bool has_meta_table() const noexcept { return meta_table_; }

		[[nodiscard]] constexpr object_table* get_meta_table() noexcept { return meta_table_; }

		[[nodiscard]] constexpr const object_table* get_meta_table() const noexcept { return meta_table_; }

		constexpr void mark_meta_table(main_state& state) const { if (meta_table_) { meta_table_->mark(state); } }

		constexpr void set_gc_list(object* list) noexcept { gc_list_ = list; }

		[[nodiscard]] constexpr object* get_gc_list() noexcept { return gc_list_; }

		[[nodiscard]] constexpr const object* get_gc_list() const noexcept { return gc_list_; }

		void traverse(main_state& state, bool weak_key, bool weak_value);

		/**
		 * @brief Looks up [key] in [table]. If found, returns the value. Otherwise, returns
		 * `magic_value_null`.
		 */
		[[nodiscard]] magic_value find(const magic_value value) const
		{
			if (const auto it = nodes_.find(value); it == nodes_.end()) { return magic_value_null; }
			else { return it->second; }
		}

		[[nodiscard]] magic_value get_tagged_method(const tagged_method_type event, const object_string& name)
		{
			gal_assert(utils::is_enum_between_of(event, tagged_method_type::index, tagged_method_type::equal));

			const auto tagged_method = find(name.operator magic_value());
			if (tagged_method == magic_value_null)
			{
				// no tagged method
				// cache this fact
				flags_ |= (1 << static_cast<flag_type>(event));
			}
			return tagged_method;
		}

		std::size_t clear_dead_node(main_state& state);
	};

	constexpr bool object::is_object_cleared()
	{
		if (type_ == object_type::string)
		{
			// strings are `values`, so are never weak
			dynamic_cast<object_string*>(this)->mark();
			return false;
		}
		return is_mark_white();
	}

	constexpr void object_user_data::do_mark(main_state& state)
	{
		// user data are never gray
		set_mark_gray_to_black();
		if (meta_table_) { meta_table_->try_mark(state); }
	}

	inline object_string* magic_value::as_string() const noexcept
	{
		gal_assert(is_string());
		return dynamic_cast<object_string*>(as_object());
	}

	inline object_table* magic_value::as_table() const noexcept
	{
		gal_assert(is_table());
		return dynamic_cast<object_table*>(as_object());
	}

	inline object_closure* magic_value::as_function() const noexcept
	{
		gal_assert(is_function());
		return dynamic_cast<object_closure*>(as_object());
	}

	inline object_user_data* magic_value::as_user_data() const noexcept
	{
		gal_assert(is_user_data());
		return dynamic_cast<object_user_data*>(as_object());
	}
}

#endif // GAL_LANG_VM_OBJECT_HPP
