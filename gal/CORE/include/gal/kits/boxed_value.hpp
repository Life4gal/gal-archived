#pragma once

#ifndef GAL_LANG_KITS_BOXED_VALUE_HPP
#define GAL_LANG_KITS_BOXED_VALUE_HPP

#include <map>
#include <memory>
#include <string>
#include <any>
#include <gal/utility/type_info.hpp>

namespace gal::lang::kits
{
	class boxed_value
	{
	public:
		struct void_type {};

		using attribute_name_type = std::string;

	private:
		struct real_data
		{
			using attributes_type = std::map<attribute_name_type, std::shared_ptr<real_data>>;

			utility::gal_type_info ti;
			std::any object;
			void* data;
			const void* const_data;
			std::unique_ptr<attributes_type> attributes;
			bool is_reference;
			bool is_return_value;

			real_data(
					const utility::gal_type_info& ti,
					std::any to,
					const void* const_data,
					const bool is_reference,
					const bool is_return_value
					)
				: ti{ti},
				  object{std::move(to)},
				  data{ti.is_const() ? nullptr : const_cast<void*>(const_data)},
				  const_data{const_data},
				  is_reference{is_reference},
				  is_return_value{is_return_value} {}

			real_data(const real_data&) = delete;

			real_data& operator =(const real_data& other)
			{
				if (&other == this)
				[[unlikely]]
				{
					return *this;
				}

				ti = other.ti;
				object = other.object;
				data = other.data;
				const_data = other.const_data;
				is_reference = other.is_reference;
				is_return_value = other.is_return_value;

				if (other.attributes) { attributes = std::make_unique<attributes_type>(*other.attributes); }

				return *this;
			}

			real_data(real_data&&) = default;
			real_data& operator=(real_data&&) = default;

			~real_data() = default;
		};

		struct real_data_factory
		{
			static auto make()
			{
				return std::make_shared<real_data>(
						utility::gal_type_info{},
						std::any{},
						nullptr,
						false,
						false);
			}

			static auto make(void_type, bool is_return_value)
			{
				return std::make_shared<real_data>(
						utility::detail::type_info_factory<void>::make(),
						std::any{},
						nullptr,
						false,
						is_return_value);
			}

			template<typename T>
			static auto make(const std::shared_ptr<T>& object, bool is_return_value)
			{
				return std::make_shared<real_data>(
						utility::detail::type_info_factory<T>::make(),
						std::any{object},
						object.get(),
						false,
						is_return_value);
			}

			template<typename T>
			static auto make(std::shared_ptr<T>&& object, bool is_return_value)
			{
				auto ptr = object.get();
				return std::make_shared<real_data>(
						utility::detail::type_info_factory<T>::make(),
						std::any{std::move(object)},
						ptr,
						false,
						is_return_value);
			}

			template<typename T>
			static auto make(const std::shared_ptr<T>* object, const bool is_return_value) { return make(*object, is_return_value); }

			template<typename T>
			static auto make(T* object, const bool is_return_value) { return make(std::ref(*object), is_return_value); }

			template<typename T>
			static auto make(const T* object, const bool is_return_value) { return make(std::cref(*object), is_return_value); }

			template<typename T>
			static auto make(std::reference_wrapper<T> object, bool is_return_value)
			{
				auto ptr = &object.get();
				return std::make_shared<real_data>(
						utility::detail::type_info_factory<T>::make(),
						std::any{std::move(object)},
						ptr,
						true,
						is_return_value);
			}

			template<typename T>
			static auto make(std::unique_ptr<T>&& object, bool is_return_value)
			{
				auto ptr = object.get();
				return std::make_shared<real_data>(
						utility::detail::type_info_factory<T>::make(),
						std::any{std::make_shared<std::unique_ptr<T>>(std::move(object))},
						ptr,
						true,
						is_return_value);
			}

			template<typename T>
			static auto make(T t, const bool is_return_value) { return make(std::move(std::make_shared<T>(std::move(t))), is_return_value); }
		};

		std::shared_ptr<real_data> data_;

		// necessary to avoid hitting the templated && constructor of boxed_value
		struct internal_construction_tag {};

		boxed_value(std::shared_ptr<real_data> data, internal_construction_tag)
			: data_{std::move(data)} {}

	public:
		boxed_value()
			: data_{real_data_factory::make()} {}

		template<typename T>
			requires(not std::is_same_v<std::decay_t<T>, boxed_value>)
		explicit boxed_value(T&& object, const bool is_return_value = false)// NOLINT(bugprone-forwarding-reference-overload)
			: data_{real_data_factory::make(std::forward<T>(object), is_return_value)} {}

		void swap(boxed_value& other) noexcept
		{
			using std::swap;
			swap(data_, other.data_);
		}

		/**
		 * @brief Copy the values stored in other.data_ to data_.
		 *
		 * @note data_ pointers are not shared in this case
		 */
		boxed_value assign(const boxed_value& other) noexcept
		{
			*data_ = *other.data_;
			return *this;
		}

		[[nodiscard]] const utility::gal_type_info& type_info() const noexcept { return data_->ti; }

		[[nodiscard]] static bool is_type_match(const boxed_value& lhs, const boxed_value& rhs) noexcept { return lhs.type_info() == rhs.type_info(); }

		/**
		 * @brief return true if the object is uninitialized
		 */
		[[nodiscard]] bool is_undefined() const noexcept { return data_->ti.is_undefined(); }

		[[nodiscard]] bool is_const() const noexcept { return data_->ti.is_const(); }

		[[nodiscard]] bool is_null() const noexcept { return data_->data == nullptr && data_->const_data == nullptr; }

		[[nodiscard]] bool is_reference() const noexcept { return data_->is_reference; }

		[[nodiscard]] bool is_pointer() const noexcept { return not is_reference(); }

		[[nodiscard]] bool is_return_value() const noexcept { return data_->is_return_value; }

		void reset_return_value() const noexcept { data_->is_return_value = false; }

		[[nodiscard]] bool is_type_of(const utility::gal_type_info& ti) const noexcept { return data_->ti.bare_equal(ti); }

		template<typename T>
		auto pointer_sentinel(std::shared_ptr<T>& ptr) const noexcept
		{
			struct sentinel
			{
				std::reference_wrapper<std::shared_ptr<T>> ptr;
				std::reference_wrapper<real_data> data;

				sentinel(const sentinel&) = delete;
				sentinel& operator=(const sentinel&) = delete;
				sentinel(sentinel&&) = default;
				sentinel& operator=(sentinel&&) = default;

				~sentinel() noexcept
				{
					// save new pointer data
					const auto p = ptr.get().get();
					data.get().data = p;
					data.get().const_data = p;
				}

				// ReSharper disable once CppNonExplicitConversionOperator
				operator std::shared_ptr<T>&() const noexcept { return ptr.get(); }
			};

			return sentinel{.ptr = ptr, .data = *data_};
		}

		[[nodiscard]] const std::any& get() const noexcept { return data_->object; }

		[[nodiscard]] void* get_ptr() const noexcept { return data_->data; }

		[[nodiscard]] const void* get_const_ptr() const noexcept { return data_->data; }

		[[nodiscard]] boxed_value get_attribute(const attribute_name_type& name) const
		{
			if (not data_->attributes) { data_->attributes = std::make_unique<real_data::attributes_type>(); }

			if (auto& attribute = (*data_->attributes)[name];
				attribute) { return {attribute, internal_construction_tag{}}; }
			else
			{
				boxed_value ret;// default construct a new one
				attribute = ret.data_;
				return ret;
			}
		}

		boxed_value& copy_attributes(const boxed_value& other)
		{
			if (other.data_->attributes) { data_->attributes = std::make_unique<real_data::attributes_type>(*other.data_->attributes); }
			return *this;
		}

		boxed_value& clone_attributes(const boxed_value& other)
		{
			copy_attributes(other);
			reset_return_value();
			return *this;
		}
	};

	namespace detail
	{
		/**
		 * @brief Takes a value, copies it and returns a boxed_value object that is immutable.
		 * @param object Value to copy and make const
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const T& object) { return boxed_value{std::make_shared<std::add_const_t<T>>(object)}; }

		/**
		 * @brief Takes a pointer to a value, adds const to the pointed to type and returns an
		 * immutable boxed_value. Does not copy the pointed to value.
		 * @param object Pointer to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(T* object) { return boxed_value{const_cast<std::add_const_t<T>*>(object)}; }

		/**
		 * @brief Takes a std::shared_ptr to a value, adds const to the pointed to type and
		 * returns an immutable boxed_value. Does not copy the pointed to value.
		 * @param object Pointer to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const std::shared_ptr<T>& object) { return boxed_value{std::const_pointer_cast<std::add_const_t<T>>(object)}; }

		/**
		 * @brief Takes a std::reference_wrapper value, adds const to the referenced type
		 * and returns an immutable boxed_value. Does not copy the referenced value.
		 * @param object Reference object to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const std::reference_wrapper<T>& object) { return boxed_value{std::cref(object.get())}; }
	}

	/**
	 * @brief Creates a boxed_value. If the object passed in is a value type, it is copied.
	 * If it is a pointer, std::shared_ptr, or std::reference_type a copy is not made.
	 *
	 * @param t The value to box
	 */
	template<typename T>
	boxed_value var(T&& t) { return boxed_value{std::forward<T>(t)}; }

	/**
	 * @brief Takes an object and returns an immutable boxed_value. If the object is a
	 * std::reference or pointer type the value is not copied.
	 * If it is an object type, it is copied.
	 * @param object Object to make immutable
	 * @return Immutable boxed_value
	 */
	template<typename T>
	boxed_value const_var(const T& object) { return detail::make_const_boxed_value(object); }

	inline boxed_value void_var()
	{
		const static boxed_value v{boxed_value::void_type{}};
		return v;
	}

	inline boxed_value const_var(const bool b)
	{
		const static boxed_value t{detail::make_const_boxed_value(true)};
		const static boxed_value f{detail::make_const_boxed_value(false)};

		return b ? t : f;
	}
}

#endif // GAL_LANG_KITS_BOXED_VALUE_HPP
