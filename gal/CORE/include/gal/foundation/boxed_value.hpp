#pragma once

#ifndef GAL_LANG_FOUNDATION_BOXED_VALUE_HPP
#define GAL_LANG_FOUNDATION_BOXED_VALUE_HPP

#include <gal/foundation/string.hpp>
#include <gal/foundation/type_info.hpp>
#include <any>

namespace gal::lang::foundation
{
	class boxed_value
	{
		struct internal_data;

	public:
		struct void_type
		{
			using type = void;
		};

		using internal_data_type = std::shared_ptr<internal_data>;

		GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) static const gal_type_info& class_type() noexcept
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)static gal_type_info type = make_type_info<boxed_value>();
			return type;
		}

	private:
		/**
		 * @brief structure which holds the internal state of a boxed_value
		 *
		 * @todo get rid of any and merge it with this, reducing an allocation in the process
		 */
		struct internal_data
		{
			using data_type = std::any;

			gal_type_info type;
			data_type data;

			void* raw;
			const void* const_raw;

			bool is_reference;
			bool is_xvalue;

			internal_data(
					const gal_type_info type,
					data_type data,
					const void* const_raw,
					const bool is_reference,
					const bool is_xvalue)
				: type{type},
				  data{std::move(data)},
				  raw{type.is_const() ? nullptr : const_cast<void*>(const_raw)},
				  const_raw{const_raw},
				  is_reference{is_reference},
				  is_xvalue{is_xvalue} { }

			internal_data(const internal_data&) = delete;
			internal_data(internal_data&&) = delete;

			internal_data& operator=(const internal_data&) = default;
			internal_data& operator=(internal_data&&) = default;

			~internal_data() noexcept = default;
		};

		struct internal_data_factory
		{
			static auto make()
			{
				return std::make_shared<internal_data>(
						make_invalid_type_type(),
						internal_data::data_type{},
						nullptr,
						false,
						false);
			}

			static auto make(void_type, const bool is_xvalue)
			{
				return std::make_shared<internal_data>(
						make_type_info<void_type::type>(),
						internal_data::data_type{},
						nullptr,
						false,
						is_xvalue);
			}

			template<typename T>
			static auto make(const std::shared_ptr<T>& data, const bool is_xvalue)
			{
				return std::make_shared<internal_data>(
						make_type_info<T>(),
						internal_data::data_type{data},
						data.get(),
						false,
						is_xvalue);
			}

			template<typename T>
			static auto make(std::shared_ptr<T>&& data, const bool is_xvalue)
			{
				auto raw = data.get();
				return std::make_shared<internal_data>(
						make_type_info<T>(),
						internal_data::data_type{std::move(data)},
						raw,
						false,
						is_xvalue);
			}

			template<typename T>
			static auto make(std::unique_ptr<T>&& data, const bool is_xvalue)
			{
				auto raw = data.get();
				return std::make_shared<internal_data>(
						make_type_info<T>(),
						internal_data::data_type{std::make_shared<std::unique_ptr<T>>(std::move(data))},
						raw,
						true,
						is_xvalue);
			}

			template<typename T>
			static auto make(const std::shared_ptr<T>* data, const bool is_xvalue) { return internal_data_factory::make(*data, is_xvalue); }

			template<typename T>
			static auto make(T* data, const bool is_xvalue) { return internal_data_factory::make(std::ref(*data), is_xvalue); }

			template<typename T>
			static auto make(const T* data, const bool is_xvalue) { return internal_data_factory::make(std::cref(*data), is_xvalue); }

			template<typename T>
			static auto make(std::reference_wrapper<T> data, const bool is_xvalue)
			{
				auto& real = data.get();
				return std::make_shared<internal_data>(
						make_type_info<T>(),
						internal_data::data_type{std::move(data)},
						&real,
						true,
						is_xvalue);
			}

			template<typename T>
			static auto make(T data, const bool is_xvalue) { return internal_data_factory::make(std::make_shared<T>(std::move(data)), is_xvalue); }
		};

		internal_data_type data_;

		// necessary to avoid hitting the templated && constructor of boxed_value
		struct internal_construction_tag { };

		boxed_value(internal_data_type data, internal_construction_tag)
			: data_{std::move(data)} {}

	public:
		boxed_value()
			: data_{internal_data_factory::make()} {}

		template<typename Any>
			requires(not std::is_same_v<std::decay_t<Any>, boxed_value>)
		explicit boxed_value(Any&& data, const bool is_xvalue = false)// NOLINT(bugprone-forwarding-reference-overload)
			: data_{internal_data_factory::make(std::forward<Any>(data), is_xvalue)} { }

		/**
		 * @brief Copy the values stored in other.data_ to data_.
		 *
		 * @note data_ pointers are not shared in this case
		 */
		boxed_value& assign(const boxed_value& other) noexcept
		{
			*data_ = *other.data_;
			return *this;
		}

		[[nodiscard]] const gal_type_info& type_info() const noexcept { return data_->type; }

		[[nodiscard]] bool type_match(const boxed_value& other) const noexcept { return type_info() == other.type_info(); }

		/**
		 * @brief return true if the object is uninitialized
		 */
		[[nodiscard]] bool is_undefined() const noexcept { return data_->type.is_undefined(); }

		[[nodiscard]] bool is_const() const noexcept { return data_->type.is_const(); }

		[[nodiscard]] bool is_null() const noexcept { return data_->raw == nullptr && data_->const_raw == nullptr; }

		[[nodiscard]] bool is_reference() const noexcept { return data_->is_reference; }

		[[nodiscard]] bool is_pointer() const noexcept { return not is_reference(); }

		[[nodiscard]] bool is_xvalue() const noexcept { return data_->is_xvalue; }

		void to_lvalue() const noexcept { data_->is_xvalue = false; }

		/**
		 * @brief return true if object's bare type equal
		 */
		[[nodiscard]] bool is_type_of(const gal_type_info ti) const noexcept { return data_->type.bare_equal(ti); }

		template<typename T>
		auto pointer_sentinel(std::shared_ptr<T>& ptr) const noexcept
		{
			struct sentinel
			{
				std::reference_wrapper<std::shared_ptr<T>> ptr;
				std::reference_wrapper<internal_data> data;

				sentinel(std::shared_ptr<T>& ptr, internal_data& data)
					: ptr{ptr},
					  data{data} {}

				sentinel(const sentinel&) = delete;
				sentinel& operator=(const sentinel&) = delete;
				sentinel(sentinel&&) = default;
				sentinel& operator=(sentinel&&) = default;

				~sentinel() noexcept
				{
					// save new pointer data
					const auto p = ptr.get().get();
					// todo: check it
					data.get().raw = const_cast<void*>(static_cast<const void*>(p));
					data.get().const_raw = p;
				}

				// ReSharper disable once CppNonExplicitConversionOperator
				operator std::shared_ptr<T>&() const noexcept { return ptr.get(); }
			};

			// return sentinel{.ptr = ptr, .data = *data_};
			return sentinel{ptr, *data_};
		}

		/**
		 * @throw std::bad_any_cast type not match
		 */
		template<typename To>
		[[nodiscard]] decltype(auto) cast() const
		{
			if constexpr (std::is_pointer_v<To>)
			{
				if constexpr (std::is_const_v<To>) { return std::any_cast<To>(&data_->data); }
				else
				{
					// we won't change the object itself directly(?), but we need the object to be mutable
					return std::any_cast<To>(&const_cast<boxed_value&>(*this).data_->data);
				}
			}
			else
			{
				if constexpr (std::is_const_v<To>) { return std::any_cast<To>(data_->data); }
				else
				{
					// we won't change the object itself directly(?), but we need the object to be mutable
					return std::any_cast<To>(const_cast<boxed_value&>(*this).data_->data);
				}
			}
		}

		[[nodiscard]] void* get_raw() const noexcept { return data_->raw; }

		[[nodiscard]] const void* get_const_raw() const noexcept { return data_->const_raw; }
	};
}

template<>
struct std::hash<gal::lang::foundation::boxed_value> : std::hash<gal::lang::foundation::boxed_value::internal_data_type>{};

#endif// GAL_LANG_FOUNDATION_BOXED_VALUE_HPP
