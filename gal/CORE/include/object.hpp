#pragma once

#ifndef GAL_LANG_OBJECT_HPP
#define GAL_LANG_OBJECT_HPP

/**
 * @file object.hpp
 * @brief Object and type object interface.
 * @details 
 * Objects are structures allocated on the heap.  Special rules apply to
 * the use of objects to ensure they are properly garbage-collected.
 * Objects are never allocated statically or on the stack; they must be
 * accessed through special functions only.
 *
 * An object has a 'reference count' that is increased or decreased when a
 * pointer to the object is copied or deleted; when the reference count
 * reaches zero there are no references to the object left and it can be
 * removed from the heap.
 *
 * An object has a 'type' that determines what it represents and what kind
 * of data it contains.  An object's type is fixed when it is created.
 * Types themselves are represented as objects; an object contains a
 * pointer to the corresponding type object.  The type itself has a type
 * pointer pointing to the object representing the type 'type', which
 * contains a pointer to itself!.
 *
 * Objects do not float around in memory; once allocated an object keeps
 * the same size and address.  Objects that must hold variable-size data
 * can contain pointers to variable-size parts of the object.  Not all
 * objects of the same type have the same size; but the size cannot change
 * after allocation.  (These restrictions are made so a reference to an
 * object can be simply a pointer -- moving an object would require
 * updating all the pointers, and changing an object's size would require
 * moving it if there was another object right next to it.)
 *
 * A standard interface exists for objects that contain an array of items
 * whose size is determined when the object is allocated.
 */

#include <def.hpp>
#include <utils/source_location.hpp>
#include <utils/enum_utils.hpp>
#include<type_traits>
#include <vector>

#if defined(GAL_LANG_DEBUG)
#include <string_view>
#endif

/**
* @details GAL_LANG_DEBUG implies GAL_LANG_REF_DEBUG.
*/
#if defined(GAL_LANG_DEBUG) && not defined(GAL_LANG_REF_DEBUG)
#define GAL_LANG_REF_DEBUG
#endif

#if defined(GAL_LANG_REF_DEBUG)
#define GAL_LANG_DO_IF_REF_DEBUG(...) __VA_ARGS__
#else
	#define GAL_LANG_DO_IF_REF_DEBUG(...)
#endif

#if defined(GAL_LANG_REF_TRACE)
#define GAL_LANG_DO_IF_REF_TRACE(...) __VA_ARGS__
#else
	#define GAL_LANG_DO_IF_REF_TRACE(...)
#endif

namespace gal::lang
{
	#if defined(GAL_LANG_REF_DEBUG)
	GAL_API_DATA gal_size_type g_object_total_refs;
	#endif

	class gal_object;
	class gal_type_object;

	[[noreturn]] GAL_API_FUNC void object_assert_failed(
			gal_object* object,
			const char* expression,
			const char* message,
			const std_source_location& location = std_source_location::current());

	[[noreturn]] GAL_API_FUNC inline void object_assert(
			const bool condition,
			gal_object* object,
			const char* expression,
			const char* message,
			const std_source_location& location = std_source_location::current()) { if (condition) { object_assert_failed(object, expression, message, location); } }

	GAL_LANG_DO_IF_DEBUG(
			[[noreturn]] GAL_API_FUNC void object_assert_failed(
				gal_object* object,
				std::string_view reason,
				const char* expression,
				const char* message,
				const std_source_location& location = std_source_location::current());)

	GAL_LANG_DO_IF_DEBUG(
			[[noreturn]] GAL_API_FUNC inline void object_assert(
				const bool condition,
				gal_object* object,
				const std::string_view reason,
				const char* expression,
				const char* message,
				const std_source_location& location = std_source_location::current()) {
			if (condition) { object_assert_failed(object, reason, expression, message, location); }
			})

	GAL_API_FUNC gal_object* get_doc_from_internal_doc(const char* name, const char* internal_doc);
	GAL_API_FUNC gal_object* get_text_signature_from_internal_doc(const char* name, const char* internal_doc);

	/**
	 * @brief Heuristic checking if the object memory is uninitialized or deallocated.
	 * Rely on the debug hooks on GAL memory allocators.
	 */
	GAL_API_FUNC bool is_object_freed(gal_object* object);
	GAL_API_FUNC bool is_object_abstract(gal_object* object);
	GAL_API_FUNC bool is_object_callable(gal_object* object);

	void safe_clear_object(
			gal_object*& object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current()
					)
			);

	void safe_clear_object(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_increase_object_ref_count(
			gal_object* object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_increase_object_ref_count(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_decrease_object_ref_count(
			gal_object* object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_decrease_object_ref_count(
			gal_object& object
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_assign_object(
			gal_object*& lhs,
			gal_object* rhs
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current())
			);

	void safe_assign_object_lhs_maybe_null(
			gal_object*& lhs,
			gal_object* rhs
			GAL_LANG_DO_IF_DEBUG(,
					std::string_view reason,
					const std_source_location& location = std_source_location::current()));

	namespace detail
	{
		struct identifier
		{
			const char* name;
			gal_object* object;
			identifier* next;
		};

		#define GAL_LANG_DETAIL_MAKE_IDENTIFIER(var, id) \
		static detail::identifier var { .name = (id) }
	}

	class gal_object
	{
	public:
		using ref_count_type = gal_size_type;

		GAL_LANG_DO_IF_REF_TRACE(
				struct ref_tracer {
				gal_object* prev{nullptr};
				gal_object* next{nullptr};
				};

				// public for convenience
				ref_tracer tracer{};
				)

	private:
		ref_count_type ref_count_;
		gal_type_object* type_;

	public:
		constexpr explicit gal_object(gal_type_object* type)
			: ref_count_{1},
			  type_{type} {}

		[[nodiscard]] constexpr ref_count_type count() const noexcept { return ref_count_; }

		constexpr void set_count(
				const ref_count_type count
				GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()))
		{
			ref_count_ = count;
			GAL_LANG_DO_IF_DEBUG((void)reason, (void)location);
		}

		void increase_count(GAL_LANG_DO_IF_DEBUG(std::string_view reason, const std_source_location& location = std_source_location::current())) noexcept
		{
			GAL_LANG_DO_IF_REF_DEBUG(++g_object_total_refs);
			++ref_count_;
			GAL_LANG_DO_IF_DEBUG((void)reason, (void)location);
		}

		void decrease_count(GAL_LANG_DO_IF_DEBUG(const std::string_view reason, const std_source_location& location = std_source_location::current())) noexcept
		{
			GAL_LANG_DO_IF_REF_DEBUG(--g_object_total_refs);

			if constexpr (std::is_unsigned_v<decltype(ref_count_)>)
			{
				if (GAL_LANG_DO_IF_REF_DEBUG(const auto current_count = ref_count_;)-- ref_count_ != 0)
				{
					GAL_LANG_DO_IF_DEBUG(
							object_assert(ref_count_ < current_count, this, reason, nullptr, "object ref count underflow", location);)
				}
				else
				{
					// deallocate();
					(void)this;
				}
			}
			else
			{
				if (--ref_count_ != 0)
				{
					GAL_LANG_DO_IF_DEBUG(
							object_assert(ref_count_ > 0, this, reason, nullptr, "object ref count is negative", location);)
				}
				else
				{
					// deallocate();
					(void)this;
				}
			}
		}

		[[nodiscard]] constexpr gal_type_object* type() noexcept { return type_; }

		[[nodiscard]] constexpr const gal_type_object* type() const noexcept { return type_; }

		constexpr void set_type(
				gal_type_object* type
				GAL_LANG_DO_IF_DEBUG(, std::string_view reason, const std_source_location& location = std_source_location::current()))
		{
			type_ = type;
			GAL_LANG_DO_IF_DEBUG((void)reason, (void)location);
		}

		[[nodiscard]] constexpr bool is_type_of(const gal_type_object* type) const noexcept { return type_ == type; }
	};

	class gal_var_object : public gal_object
	{
	public:
		using item_size_type = gal_size_type;

	private:
		/**
		 * @brief Number of items in variable part.
		 * @note size_ is an element count, not necessarily a byte count.
		 */
		item_size_type size_;

	public:
		constexpr explicit gal_var_object(gal_type_object* type, const item_size_type size = 0)
			: gal_object{type},
			  size_{size} {}

		constexpr void set_size(
				const item_size_type size
				GAL_LANG_DO_IF_DEBUG(, std::string_view reason))
		{
			size_ = size;
			GAL_LANG_DO_IF_DEBUG((void)reason);
		}
	};

	/**
	 * @brief Buffer interface
	 */
	class gal_buffer
	{
	public:
		using buffer_type = std::vector<std::byte>;
		using size_type = buffer_type::size_type;

		using flag_type = std::uint32_t;

		constexpr static size_type dimension_limit = 64;

		enum class flags : flag_type
		{
			simple = 0,
			writable = 1 << 0,
			format = 1 << 2,
			nd = 1 << 3,
			strides = 1 << 4 | nd,
			c_contiguous = 1 << 5 | strides,
			f_contiguous = 1 << 6 | strides,
			any_contiguous = 1 << 7 | strides,
			indirect = 1 << 8 | strides,

			read = 1 << 8,
			write = 1 << 9,

			contiguous_readonly = nd,
			contiguous = contiguous_readonly | writable,


			strode_readonly = strides,
			strode = strode_readonly | writable,

			records_readonly = strides | format,
			records = records_readonly | writable,

			full_readonly = indirect | format,
			full = full_readonly | writable,
		};

	private:
		gal_object* owner_;

		buffer_type buffer_;

		bool readonly_;
		size_type num_dimension_;
		char* format_;

		size_type* shape_;
		size_type* strides_;
		size_type* sub_offsets_;
		void* internal_;
	};

	class gal_type_object : public gal_var_object
	{
	public:
		using flag_type = std::uint32_t;

		enum class flags : flag_type
		{
			invalid = 0,

			// Set if the type object is dynamically allocated
			heap_type = flag_type{1} << 9,
			// Set if the type allows sub-classing
			base_type = flag_type{1} << 10,
			// Set if the type implements the vectorcall protocol
			have_vectorcall = flag_type{1} << 11,
			// Set if the type is 'ready' -- fully initialized
			ready = flag_type{1} << 12,
			// Set while the type is being 'readied', to prevent recursive ready calls
			readying = flag_type{1} << 13,
			// Objects support garbage collection (see object_impl.hpp)
			have_gc = flag_type{1} << 14,
			// Objects behave like an unbound method
			method_descriptor = flag_type{1} << 17,
			// Objects support type attribute cache
			have_version_tag = flag_type{1} << 18,
			valid_version_tag = flag_type{1} << 19,
			// Type is abstract and cannot be instantiated
			is_abstract = flag_type{1} << 20,
			// These flags are used to determine if a type is a subclass
			long_subclass = flag_type{1} << 24,
			list_subclass = flag_type{1} << 25,
			tuple_subclass = flag_type{1} << 26,
			bytes_subclass = flag_type{1} << 27,
			unicode_subclass = flag_type{1} << 28,
			dictionary_subclass = flag_type{1} << 29,
			base_exc_subclass = flag_type{1} << 30,
			type_subclass = flag_type{1} << 31,

			// default flag
			default_flag = have_version_tag,
		};

		using vectorcall_function = gal_object*(*)(gal_object& callable, const gal_object* const * args, gal_size_type num_args, gal_object* pair_args);

	private:
		const char* name_;

		// Flags to define presence of optional/expanded features
		flags flag_;

		class gal_method_define* methods_{nullptr};
		class gal_member_define* members_{nullptr};
		class gal_rw_interface_define* rw_interfaces_{nullptr};
		class gal_type_object_dictionary* metadata_{nullptr};

		gal_type_object* base_{nullptr};
		gal_object* method_resolution_order_{nullptr};
		gal_object* cache_{nullptr};
		gal_object* sub_classes_{nullptr};
		gal_object* weak_list_{nullptr};

		vectorcall_function vectorcall_{nullptr};

	protected:
		gal_type_object(
				gal_type_object* type,
				const char* name,
				const flags flag,
				gal_method_define* methods = nullptr,
				gal_member_define* members = nullptr,
				gal_rw_interface_define* rw_interfaces = nullptr,
				gal_type_object_dictionary* metadata = nullptr,
				gal_type_object*			base		  = nullptr
				)
			: gal_var_object{type},
			  name_{name},
			  flag_{flag},
			  methods_{methods},
			  members_{members},
			  rw_interfaces_{rw_interfaces},
			  metadata_{metadata},
			  base_{base} {}

		virtual ~gal_type_object();

		gal_type_object(const gal_type_object&) = default;
		gal_type_object& operator=(const gal_type_object&) = default;
		gal_type_object(gal_type_object&&) = default;
		gal_type_object& operator=(gal_type_object&&) = default;

	public:
		/**
		 * @brief For printing, in format "<module>.<name>"
		 */
		[[nodiscard]] constexpr const char* who_am_i() const noexcept { return name_; }

		[[nodiscard]] constexpr bool check_all_flag(std::same_as<flags> auto ... fs) const noexcept { return utils::check_all_enum_flag(flag_, fs...); }

		[[nodiscard]] constexpr bool check_any_flag(std::same_as<flags> auto ... fs) const noexcept { return utils::check_any_enum_flag(flag_, fs...); }

		constexpr void set_flag(std::same_as<flags> auto ... fs) noexcept { utils::set_enum_flag_set(flag_, fs...); }

		/**
		 * @brief Documentation string.
		 */
		[[nodiscard]] constexpr virtual const char* about() const noexcept = 0;

		//*****************************
		// object life interfaces below
		//*****************************

		virtual gal_object* allocate(gal_size_type num_items);
		virtual gal_object* construct(gal_object* args, gal_object* pair_args);
		virtual bool initial(gal_object* args, gal_object* pair_args);
		virtual bool finalize(gal_object& self);
		virtual bool clear(gal_object& self);
		virtual void destroy(gal_object& self);
		virtual void deallocate(gal_object& self);

		[[nodiscard]] constexpr virtual bool is_collectable(gal_object& self) const noexcept;

		virtual gal_object* represent(gal_object& self);
		virtual gal_object* as_string(gal_object& self) { return represent(self); }

		[[nodiscard]] virtual gal_hash_type hash(gal_object& self) const;

		enum class compare_operand
		{
			equal,
			not_equal,
			less,
			less_equal,
			greater,
			greater_equal
		};

		[[nodiscard]] virtual gal_object* compare(gal_object& self, gal_object& other, compare_operand operand) const;

		[[nodiscard]] virtual gal_object* iteration_begin(gal_object& self);
		[[nodiscard]] virtual gal_object* iteration_end(gal_object& self);
		[[nodiscard]] virtual gal_object* iteration_next(gal_object& self);

		[[nodiscard]] virtual gal_object* descriptor_get(gal_object& self, gal_object* args, gal_object* pair_args);
		[[nodiscard]] virtual bool descriptor_set(gal_object& self, gal_object* args, gal_object* pair_args);

		[[nodiscard]] virtual gal_object* invoke(gal_object& self, gal_object* args, gal_object* pair_args);

		[[nodiscard]] virtual gal_object* attribute_get_with_name(gal_object& self, const char* name);
		virtual bool attribute_set_with_name(gal_object& self, const char* name, gal_object* value);
		[[nodiscard]] virtual gal_object* attribute_get_with_object(gal_object& self, gal_object& name);
		virtual bool attribute_set_with_object(gal_object& self, gal_object& name, gal_object* value);

		/**
		 * @note see invoker.hpp -> gal_invoker_math for details of math operations below.
		 */

		[[nodiscard]] virtual gal_object* math_plus(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_minus(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_multiply(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_floor_divide(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_real_divide(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_divide_modulus(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_remainder(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_power(gal_object& self, gal_object& object1, gal_object* object2);

		[[nodiscard]] virtual gal_object* math_plus_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_minus_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_multiply_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_floor_divide_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_real_divide_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_remainder_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_power_assign(gal_object& self, gal_object& object1, gal_object* object2);

		[[nodiscard]] virtual gal_object* math_bit_left_shift(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_right_shift(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_and(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_or(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_xor(gal_object& self, gal_object& other);

		[[nodiscard]] virtual gal_object* math_bit_left_shift_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_right_shift_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_and_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_or_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* math_bit_xor_assign(gal_object& self, gal_object& other);

		[[nodiscard]] virtual gal_object* math_negative(gal_object& self);
		[[nodiscard]] virtual gal_object* math_positive(gal_object& self);
		[[nodiscard]] virtual gal_object* math_absolute(gal_object& self);
		[[nodiscard]] virtual gal_object* math_invert(gal_object& self);
		[[nodiscard]] virtual gal_object* math_index(gal_object& self);
		[[nodiscard]] virtual gal_object* math_to_boolean(gal_object& self);
		[[nodiscard]] virtual gal_object* math_to_integer(gal_object& self);
		[[nodiscard]] virtual gal_object* math_to_floating_point(gal_object& self);

		[[nodiscard]] virtual gal_size_type sequence_length(gal_object& self);
		[[nodiscard]] virtual gal_object* sequence_concat(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* sequence_concat_assign(gal_object& self, gal_object& other);
		[[nodiscard]] virtual gal_object* sequence_repeat(gal_object& self, gal_size_type times);
		[[nodiscard]] virtual gal_object* sequence_repeat_assign(gal_object& self, gal_size_type times);
		[[nodiscard]] virtual gal_object* sequence_element_get(gal_object& self, gal_size_type index);
		[[nodiscard]] virtual gal_object* sequence_element_set(gal_object& self, gal_size_type index, gal_object* value);

		[[nodiscard]] virtual gal_size_type mapping_length(gal_object& self);
		[[nodiscard]] virtual gal_object* mapping_element_get(gal_object& self, gal_object& index);
		[[nodiscard]] virtual gal_object* mapping_element_set(gal_object& self, gal_object& index, gal_object* value);

		[[nodiscard]] virtual gal_object* async_await(gal_object& self);
		[[nodiscard]] virtual gal_object* async_iteration(gal_object& self);
		[[nodiscard]] virtual gal_object* async_next(gal_object& self);

		virtual bool buffer_get(gal_object& self, gal_buffer& buffer, gal_buffer::flags flag);
		virtual void buffer_release(gal_object& self, gal_buffer& buffer);
	};

	class gal_type_object_type final : public gal_type_object
	{
	private:
		explicit gal_type_object_type(gal_type_object* self);

	public:
		static gal_type_object_type& type();

		[[nodiscard]] constexpr const char* about() const noexcept override
		{
			return "use type(object) to get an object's type.\n"
					"use type(name, bases, metadata, **pair_args) to get a new type.\n";
		}
	};

	class gal_type_object_object final : public gal_type_object
	{
	private:
		gal_type_object_object();

	public:
		static gal_type_object_object& type();

		[[nodiscard]] constexpr const char* about() const noexcept override
		{
			return "object() -- The base class of the class hierarchy.\n\n"
					"When called, it accepts no arguments and returns a new featureless\n"
					"instance that has no instance attributes and cannot be given any.\n";
		}
	};

	class gal_type_object_super final : public gal_type_object
	{
	private:
		gal_type_object_super();

	public:
		static gal_type_object_super& type();

		[[nodiscard]] constexpr const char* about() const noexcept override
		{
			return "super() -> same as super(__class__, <first argument>)\n"
					"super(type) -> unbound super object\n"
					"super(type, object) -> bound super object; requires instance_of(object, type)\n"
					"super(type, type2) -> bound super object; requires subclass_of(type2, type)\n"
					"Typical use to call a cooperative superclass method.\n";
		}
	};

	class gal_type_object_null final : public gal_type_object
	{
	private:
		gal_type_object_null();

	public:
		static gal_type_object_null& type();

		static gal_object& instance(
				GAL_LANG_DO_IF_DEBUG(const std_source_location& location = std_source_location::current())
				);

		[[nodiscard]] constexpr const char* about() const noexcept override
		{
			return "undefined type which can be used in contexts"
					"where nullptr is not suitable (since nullptr often means 'error')";
		}
	};

	class gal_type_object_not_implemented final : public gal_type_object
	{
	private:
		gal_type_object_not_implemented();

	public:
		static gal_type_object_not_implemented& type();

		static gal_object& instance(
				GAL_LANG_DO_IF_DEBUG(const std_source_location& location = std_source_location::current()));

		[[nodiscard]] constexpr const char* about() const noexcept override { return "current content is not implemented yet."; }
	};
}

#endif // GAL_LANG_OBJECT_HPP
