#pragma once

#ifndef GAL_LANG_FOUNDATION_BOXED_VALUE_HPP
#define GAL_LANG_FOUNDATION_BOXED_VALUE_HPP

#include <utils/string_pool.hpp>
#include <gal/foundation/type_info.hpp>
#include <map>
#include <any>

namespace gal::lang::foundation
{
	class boxed_value
	{
		struct member_data;
	public:
		struct void_type
		{
			using type = void;
		};

		using class_member_data_name_type = std::string;
		using class_member_data_name_view_type = std::string_view;
		using class_member_data_data_type = std::shared_ptr<member_data>;

		using class_member_data_type = std::map<class_member_data_name_type, class_member_data_data_type, std::less<>>;

	private:
		struct member_data
		{
			using data_type = std::any;

			gal_type_info ti;
			data_type data;

			class_member_data_type members;

			void* raw;
			const void* const_raw;

			bool is_reference;
			bool is_xvalue;

			member_data(
					const gal_type_info ti,
					data_type data,
					const void* const_raw,
					const bool is_reference,
					const bool is_xvalue)
				: ti{ti},
				  data{std::move(data)},
				  raw{ti.is_const() ? nullptr : const_cast<void*>(const_raw)},
				  const_raw{const_raw},
				  is_reference{is_reference},
				  is_xvalue{is_xvalue} {}

			member_data(const member_data&) = delete;

			member_data& operator =(const member_data& other)
			{
				if (&other == this)
				[[unlikely]]
				{
					return *this;
				}

				ti = other.ti;
				data = other.data;
				raw = other.raw;
				const_raw = other.const_raw;
				is_reference = other.is_reference;
				is_xvalue = other.is_xvalue;

				if (not other.members.empty())
				{
					gal_assert(members.empty());
					members = other.members;
				}

				return *this;
			}

			member_data(member_data&&) = delete;
			member_data& operator=(member_data&&) = default;

			~member_data() noexcept = default;
		};

		struct member_data_maker
		{
			static class_member_data_data_type make()
			{
				return std::make_shared<member_data>(
						make_invalid_type_info(),
						member_data::data_type{},
						nullptr,
						false,
						false);
			}

			static class_member_data_data_type make(void_type, const bool is_xvalue)
			{
				return std::make_shared<member_data>(
						make_type_info<void_type::type>(),
						member_data::data_type{},
						nullptr,
						false,
						is_xvalue);
			}

			template<typename T>
			static class_member_data_data_type make(const std::shared_ptr<T>& data, const bool is_xvalue)
			{
				return std::make_shared<member_data>(
						make_type_info<T>(),
						member_data::data_type{data},
						data.get(),
						false,
						is_xvalue);
			}

			template<typename T>
			static class_member_data_data_type make(std::shared_ptr<T>&& data, const bool is_xvalue)
			{
				// todo: evaluation order?
				return std::make_shared<member_data>(
						make_type_info<T>(),
						member_data::data_type{std::move(data)},
						data.get(),
						false,
						is_xvalue);
			}

			template<typename T>
			static class_member_data_data_type make(std::unique_ptr<T>&& data, const bool is_xvalue)
			{
				// todo: evaluation order?
				return std::make_shared<member_data>(
						make_type_info<T>(),
						member_data::data_type{std::make_shared<std::unique_ptr<T>>(std::move(data))},
						data.get(),
						true,
						is_xvalue);
			}

			template<typename T>
			static class_member_data_data_type make(const std::shared_ptr<T>* data, const bool is_xvalue) { return make(*data, is_xvalue); }

			template<typename T>
			static class_member_data_data_type make(T* data, const bool is_xvalue) { return make(std::ref(*data), is_xvalue); }

			template<typename T>
			static class_member_data_data_type make(const T* data, const bool is_xvalue) { return make(std::cref(*data), is_xvalue); }

			template<typename T>
			static class_member_data_data_type make(std::reference_wrapper<T> data, const bool is_xvalue)
			{
				// todo: evaluation order?
				return std::make_shared<member_data>(
						make_type_info<T>(),
						member_data::data_type{std::move(data)},
						&data.get(),
						true,
						is_xvalue);
			}

			template<typename T>
			static class_member_data_data_type make(T data, const bool is_xvalue) { return make(std::move(std::make_shared<T>(std::move(data))), is_xvalue); }
		};

		class_member_data_data_type data_;

		// necessary to avoid hitting the templated && constructor of boxed_value
		struct internal_construction_tag { };

		boxed_value(class_member_data_data_type data, internal_construction_tag)
			: data_{std::move(data)} {}

	public:
		boxed_value()
			: data_{member_data_maker::make()} {}

		template<typename Any>
			requires(not std::is_same_v<std::decay_t<Any>, boxed_value>)
		explicit boxed_value(Any&& data, const bool is_xvalue = false)// NOLINT(bugprone-forwarding-reference-overload)
			: data_{member_data_maker::make(std::forward<Any>(data), is_xvalue)} {}

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
		boxed_value& assign(const boxed_value& other) noexcept
		{
			*data_ = *other.data_;
			return *this;
		}

		[[nodiscard]] const gal_type_info& type_info() const noexcept { return data_->ti; }

		[[nodiscard]] static bool is_type_matched(const boxed_value& lhs, const boxed_value& rhs) noexcept { return lhs.type_info() == rhs.type_info(); }

		/**
		 * @brief return true if the object is uninitialized
		 */
		[[nodiscard]] bool is_undefined() const noexcept { return data_->ti.is_undefined(); }

		[[nodiscard]] bool is_const() const noexcept { return data_->ti.is_const(); }

		[[nodiscard]] bool is_null() const noexcept { return data_->raw == nullptr && data_->const_raw == nullptr; }

		[[nodiscard]] bool is_reference() const noexcept { return data_->is_reference; }

		[[nodiscard]] bool is_pointer() const noexcept { return not is_reference(); }

		[[nodiscard]] bool is_xvalue() const noexcept { return data_->is_xvalue; }

		void to_lvalue() const noexcept { data_->is_xvalue = false; }

		[[nodiscard]] bool is_type_of(const gal_type_info ti) const noexcept { return data_->ti.bare_equal(ti); }

		template<typename T>
		auto pointer_sentinel(std::shared_ptr<T>& ptr) const noexcept
		{
			struct sentinel
			{
				std::reference_wrapper<std::shared_ptr<T>> ptr;
				std::reference_wrapper<member_data> data;

				sentinel(const sentinel&) = delete;
				sentinel& operator=(const sentinel&) = delete;
				sentinel(sentinel&&) = default;
				sentinel& operator=(sentinel&&) = default;

				~sentinel() noexcept
				{
					// save new pointer data
					const auto p = ptr.get().get();
					data.get().raw = p;
					data.get().const_raw = p;
				}

				// ReSharper disable once CppNonExplicitConversionOperator
				operator std::shared_ptr<T>&() const noexcept { return ptr.get(); }
			};

			return sentinel{.ptr = ptr, .data = *data_};
		}

		[[nodiscard]] const member_data::data_type& get() const noexcept { return data_->data; }

		[[nodiscard]] void* get_raw() const noexcept { return data_->raw; }

		[[nodiscard]] const void* get_const_raw() const noexcept { return data_->const_raw; }

		[[nodiscard]] boxed_value get_member_data(const class_member_data_name_view_type name) const
		{
			if (const auto it = data_->members.find(name);
				it != data_->members.end()) { return {it->second, internal_construction_tag{}}; }
			// default construct a new one
			boxed_value ret{};
			gal_assert(data_->members.emplace(name, ret.data_).second);
			return ret;
		}

		boxed_value& set_member_data(const boxed_value& target)
		{
			gal_assert(data_->members.empty());
			data_->members = target.data_->members;
			return *this;
		}

		boxed_value& clone_member_data(const boxed_value& target)
		{
			set_member_data(target);
			to_lvalue();
			return *this;
		}
	};
}

#endif // GAL_LANG_FOUNDATION_BOXED_VALUE_HPP
