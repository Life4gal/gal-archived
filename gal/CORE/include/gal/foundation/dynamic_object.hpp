#pragma once

#ifndef GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP
#define GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP

#include<gal/foundation/boxed_value.hpp>
#include<utils/format.hpp>

namespace gal::lang
{
	namespace exception
	{
		/**
		 * @code
		 *
		 * class my_class
		 * {
		 *	def my_class()
		 *	{
		 *		this.set_explicit(true);
		 *		this.data = 42; // raise an option_explicit_error because dynamic parameter definitions has been disabled
		 *	}
		 * }
		 *
		 * @endcode 
		 */
		class option_explicit_error final : public std::runtime_error
		{
		public:
			explicit option_explicit_error(std::string_view parameter)
				: std::runtime_error{std_format::format("option explicit set but parameter '{}' does not exist", parameter)} {}
		};
	}

	namespace foundation
	{
		class dynamic_object
		{
		public:
			using type_name_type = boxed_value::class_member_data_name_type;
			using type_name_view_type = boxed_value::class_member_data_name_view_type;

			using class_member_data_name_type = boxed_value::class_member_data_name_type;
			using class_member_data_name_view_type = boxed_value::class_member_data_name_view_type;
			using class_member_data_data_type = boxed_value;
			using class_member_data_type = std::map<class_member_data_name_type, class_member_data_data_type, std::less<>>;

			constexpr static type_name_view_type unknown_type_name = "unknown_type";

		private:
			type_name_type type_name_;
			bool is_explicit_;

			class_member_data_type members_;

		public:
			explicit dynamic_object(type_name_type&& type_name)
				: type_name_{std::move(type_name)},
				  is_explicit_{false} {}

			explicit dynamic_object(const type_name_type& type_name)
				: type_name_{type_name},
				  is_explicit_{false} {}

			dynamic_object()
				: dynamic_object{type_name_type{unknown_type_name}} {}

			[[nodiscard]] constexpr bool is_explicit() const noexcept { return is_explicit_; }

			constexpr void set_explicit(const bool value) noexcept { is_explicit_ = value; }

			[[nodiscard]] constexpr type_name_view_type type_name() const noexcept { return type_name_; }

			[[nodiscard]] bool has_member(const class_member_data_name_view_type name) const { return members_.contains(name); }

			[[nodiscard]] class_member_data_data_type& get_member(class_member_data_name_type&& name) { return members_[std::forward<class_member_data_name_type>(name)]; }

			[[nodiscard]] const class_member_data_data_type& get_member(const class_member_data_name_view_type name) const
			{
				if (const auto it = members_.find(name);
					it != members_.end()) { return it->second; }
				throw std::range_error{std_format::format("Member '{}' not found and cannot be added to a const object", name)};
			}

			void swap_members(dynamic_object& other) noexcept
			{
				using std::swap;
				swap(members_, other.members_);
			}

			[[nodiscard]] class_member_data_type copy_members() const { return members_; }

			/**
		 * @brief A function of the signature method_missing(object, name, param1, param2, param3) will be called if an appropriate method cannot be found.
		 *
		 * @code
		 *
		 * def method_missing(int i, string name, vector v) :
		 *	print("integer ${i} has no method named ${name} with ${v.size()} params")
		 *
		 * 42.not_exist_function(1, 2, 3) // prints "integer 42 has no method named not_exist_function with 3 params"
		 *
		 * @endcode
		 *
		 * @note method_missing signature can be either 2 parameters or 3 parameters.
		 * If the signature contains two parameters it is treated as a property.
		 * If the property contains a function then additional parameters are passed to the contained function.
		 * If both a 2 parameter and a 3 parameter signature match, the 3 parameter function always wins.
		 *
		 * @throw option_explicit_error explicit was set
		 */
			[[nodiscard]] decltype(auto) method_missing(class_member_data_name_type&& name)
			{
				if (is_explicit() && not has_member(name)) { throw exception::option_explicit_error{name}; }

				return get_member(std::forward<class_member_data_name_type>(name));
			}

			[[nodiscard]] decltype(auto) method_missing(const class_member_data_name_view_type name) const
			{
				if (is_explicit() && not has_member(name)) { throw exception::option_explicit_error{name}; }

				return get_member(name);
			}
		};
	}// namespace foundation
}

#endif // GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP
