#pragma once

#ifndef GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP
#define GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP

#include<gal/foundation/boxed_value.hpp>
// #include <unordered_map>
#include <map>

namespace gal::lang::foundation
{
	class dynamic_object;

	class dynamic_object
	{
	public:
		constexpr static string_view_type unknown_type_name = "unknown_type";
		// todo: transparent!
		// using members_type = std::unordered_map<string_type, boxed_value, std::hash<string_type>, std::equal_to<>>;
		using members_type = std::map<string_type, boxed_value, std::less<>>;

		GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) static const gal_type_info& class_type() noexcept
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) static gal_type_info type = make_type_info<dynamic_object>();
			return type;
		}

	private:
		string_type type_name_;

		members_type members_;

	public:
		explicit dynamic_object(const string_type& name)
			: type_name_{std::move(name)} {}

		explicit dynamic_object(const string_view_type name = unknown_type_name)
			: type_name_{name} {}

		//************************************************************************
		//****************************** INTERFACES ****************************
		//************************************************************************

		[[nodiscard]] constexpr string_view_type nameof() const noexcept { return type_name_; }

		[[nodiscard]] bool has_attr(const string_view_type name) const { return members_.contains(name); }

		[[nodiscard]] boxed_value& get_attr(const string_view_type name)
		{
			// todo: transparent!
			// return members_[name];
			return members_.emplace(name, boxed_value{}).first->second;
		}

		[[nodiscard]] const boxed_value& get_attr(const string_view_type name) const
		{
			if (const auto it = members_.find(name);
				it != members_.end()) { return it->second; }
			throw std::range_error{std_format::format("Member '{}' not found and cannot be added to a const object", name)};
		}

		bool set_attr(const string_view_type name, boxed_value&& new_value)
		{
			// todo: transparent!
			return members_.insert_or_assign(string_type{name}, std::move(new_value)).second;
		}

		bool set_attr(const string_view_type name, const boxed_value& new_value)
		{
			// todo: transparent!
			return members_.insert_or_assign(string_type{name}, new_value).second;
		}

		bool del_attr(const string_view_type name) { return members_.erase(name); }
	};
}

#endif // GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_HPP
