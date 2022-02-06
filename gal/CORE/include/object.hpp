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
#include<object_interface.hpp>
#include <utils/source_location.hpp>
#include<type_traits>

#include "object.hpp"

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

	class gal_type_object : public gal_var_object
	{
	public:
		using flag_type = std::uint32_t;

		// Set if the type object is dynamically allocated
		constexpr static flag_type flag_heap_type = flag_type{1} << 9;
		// Set if the type allows sub-classing
		constexpr static flag_type flag_base_type = flag_type{1} << 10;
		// Set if the type implements the vectorcall protocol
		constexpr static flag_type flag_have_vectorcall = flag_type{1} << 11;
		// Set if the type is 'ready' -- fully initialized
		constexpr static flag_type flag_ready = flag_type{1} << 12;
		// Set while the type is being 'readied', to prevent recursive ready calls
		constexpr static flag_type flag_readying = flag_type{1} << 13;
		// Objects support garbage collection (see object_impl.hpp)
		constexpr static flag_type flag_have_gc = flag_type{1} << 14;
		// Objects behave like an unbound method
		constexpr static flag_type flag_method_descriptor = flag_type{1} << 17;
		// Objects support type attribute cache
		constexpr static flag_type flag_have_version_tag = flag_type{1} << 18;
		constexpr static flag_type flag_valid_version_tag = flag_type{1} << 19;
		// Type is abstract and cannot be instantiated
		constexpr static flag_type flag_is_abstract = flag_type{1} << 20;
		// These flags are used to determine if a type is a subclass
		constexpr static flag_type flag_long_subclass = flag_type{1} << 24;
		constexpr static flag_type flag_list_subclass = flag_type{1} << 25;
		constexpr static flag_type flag_tuple_subclass = flag_type{1} << 26;
		constexpr static flag_type flag_bytes_subclass = flag_type{1} << 27;
		constexpr static flag_type flag_unicode_subclass = flag_type{1} << 28;
		constexpr static flag_type flag_dictionary_subclass = flag_type{1} << 29;
		constexpr static flag_type flag_base_exc_subclass = flag_type{1} << 30;
		constexpr static flag_type flag_type_subclass = flag_type{1} << 31;

		// default flag
		constexpr static flag_type flag_default = flag_have_version_tag;

		using vectorcall_function = gal_object*(*)(gal_object& callable, const gal_object* const * args, gal_size_type num_args, gal_object* pair_args);

	private:
		// For printing, in format "<module>.<name>"
		const char* name_;

		// Flags to define presence of optional/expanded features
		flag_type flag_;

		// Documentation string
		const char* document_;

		struct gal_method_define* methods_{nullptr};
		struct gal_member_define* members_{nullptr};
		struct gal_rw_interface_define* rw_interfaces_{nullptr};
		struct gal_type_object_dictionary* metadata_{nullptr};

		gal_object* base_{nullptr};
		gal_object* method_resolution_order_{nullptr};
		gal_object* cache_{nullptr};
		gal_object* sub_classes_{nullptr};
		gal_object* weak_list_{nullptr};

		vectorcall_function vectorcall_{nullptr};

	public:
		gal_type_object(
				gal_type_object* type,
				const char* name,
				const flag_type flag,
				const char* document = nullptr,
				gal_method_define* methods = nullptr,
				gal_member_define* members = nullptr,
				gal_rw_interface_define* rw_interfaces = nullptr,
				gal_type_object_dictionary* metadata = nullptr
				)
			: gal_var_object{type},
			  name_{name},
			  flag_{flag},
			  document_{document},
			  methods_{methods},
			  members_{members},
			  rw_interfaces_{rw_interfaces},
			  metadata_{metadata} {}
	};

	class gal_type_object_type : public gal_type_object
	{
	public:
		struct object_life_manager : traits::object_life_interface<gal_type_object_type>
		{
			struct deallocate_type : std::true_type
			{
				static void call(host_class_type& self);
			};

			struct construct_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};

			struct initial_type : std::true_type
			{
				static bool call(gal_object& self, gal_object* args, gal_object* pair_args);
			};

			struct clear_type : std::true_type
			{
				static bool call(gal_object& self);
			};
		};

		struct object_traverse_manager : traits::object_traverse_interface<gal_type_object_type>
		{
			struct traverse_type
			{
				static bool call(host_class_type& self);
			};
		};

		constexpr static gal_size_type gc_checker_id = 0;

		struct object_gc_checker : traits::object_inquire_interface<gal_type_object_type, gc_checker_id>
		{
			static_assert(gc_checker_id == dummy_id);

			struct inquire_type : std::true_type
			{
				static bool call(host_class_type& self);
			};
		};

		struct object_represent_manager : traits::object_represent_interface<gal_type_object_type>
		{
			struct represent_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};
		};

		struct object_invoke_manager : traits::object_invoke_interface<gal_type_object_type>
		{
			struct invoke_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};
		};

		struct object_attribute_manager : traits::object_attribute_interface<gal_type_object_type>
		{
			struct object_get_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object& name);
			};

			struct object_set_type : std::true_type
			{
				static bool call(host_class_type& self, gal_object& name, gal_object* value);
			};
		};

	private:
		explicit gal_type_object_type(gal_type_object* self);

	public:
		static gal_type_object_type& type();
	};

	class gal_type_object_object : public gal_type_object
	{
	public:
		struct object_life_manager : traits::object_life_interface<gal_type_object_object>
		{
			struct allocate_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_size_type num_items);
			};

			struct deallocate_type : std::true_type
			{
				static void call(host_class_type& self);
			};

			struct construct_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};

			struct initial_type : std::true_type
			{
				static bool call(gal_object& self, gal_object* args, gal_object* pair_args);
			};
		};

		struct object_represent_manager : traits::object_represent_interface<gal_type_object_object>
		{
			struct represent_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};

			struct string_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};
		};

		struct object_attribute_manager : traits::object_attribute_interface<gal_type_object_type>
		{
			struct object_get_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object& name);
			};

			struct object_set_type : std::true_type
			{
				static bool call(host_class_type& self, gal_object& name, gal_object* value);
			};
		};

		struct object_compare_manager : traits::object_compare_interface<gal_type_object_object>
		{
			struct compare_type : std::true_type
			{
				static gal_object* compare(gal_object& lhs, gal_object& rhs, compare_operand operand);
			};
		};

	private:
		gal_type_object_object();

	public:
		static gal_type_object_object& type();
	};

	class gal_type_object_super : public gal_type_object
	{
	public:
		struct object_life_manager : traits::object_life_interface<gal_type_object_type>
		{
			struct allocate_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_size_type num_items);
			};

			struct deallocate_type : std::true_type
			{
				static void call(host_class_type& self);
			};

			struct construct_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};

			struct initial_type : std::true_type
			{
				static bool call(gal_object& self, gal_object* args, gal_object* pair_args);
			};

			struct destroy_type : std::true_type
			{
				static void call(gal_object& self);
			};
		};

		struct object_traverse_manager : traits::object_traverse_interface<gal_type_object_type>
		{
			struct traverse_type
			{
				static bool call(host_class_type& self);
			};
		};

		struct object_represent_manager : traits::object_represent_interface<gal_type_object_type>
		{
			struct represent_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};
		};

		struct object_descriptor_manager : traits::object_descriptor_interface<gal_type_object_super>
		{
			struct descriptor_get_type : std::true_type
			{
				static gal_object* call(gal_object& self, gal_object& object, gal_object* type);
			};
		};

		struct object_attribute_manager : traits::object_attribute_interface<gal_type_object_type>
		{
			struct object_get_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object& name);
			};
		};

	private:
		gal_type_object_super();

	public:
		static gal_type_object_super& type();
	};

	class gal_type_object_null : public gal_type_object
	{
	public:
		struct object_life_manager : traits::object_life_interface<gal_type_object_null>
		{
			struct construct_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};
		};

		struct object_represent_manager : traits::object_represent_interface<gal_type_object_null>
		{
			struct represent_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};
		};

		struct object_mathematical_manager : traits::object_mathematical_interface<gal_type_object_null>
		{
			struct to_boolean : std::true_type
			{
				static bool call(const gal_object& self);
			};
		};

	private:
		gal_type_object_null();

	public:
		static gal_type_object_null& type();

		static gal_object& instance(
				GAL_LANG_DO_IF_DEBUG(const std_source_location& location = std_source_location::current())
				);
	};

	class gal_type_object_not_implemented : public gal_type_object
	{
	public:
		struct object_life_manager : traits::object_life_interface<gal_type_object_null>
		{
			struct construct_type : std::true_type
			{
				static gal_object* call(host_class_type& self, gal_object* args, gal_object* pair_args);
			};
		};

		struct object_represent_manager : traits::object_represent_interface<gal_type_object_null>
		{
			struct represent_type : std::true_type
			{
				static gal_object* call(host_class_type& self);
			};
		};

		struct object_mathematical_manager : traits::object_mathematical_interface<gal_type_object_null>
		{
			struct to_boolean : std::true_type
			{
				static bool call(const gal_object& self);
			};
		};

	private:
		gal_type_object_not_implemented();

	public:
		static gal_type_object_not_implemented& type();

		static gal_object& instance(
				GAL_LANG_DO_IF_DEBUG(const std_source_location& location = std_source_location::current()));
	};
}

#endif // GAL_LANG_OBJECT_HPP
