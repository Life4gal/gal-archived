#pragma once

#ifndef GAL_LANG_KITS_DYNAMIC_OBJECT_HPP
#define GAL_LANG_KITS_DYNAMIC_OBJECT_HPP

#include <map>
#include <gal/kits/boxed_value.hpp>
#include <utils/format.hpp>

namespace gal::lang::kits
{
	class option_explicit_error final : public std::runtime_error
	{
	public:
		explicit option_explicit_error(std::string_view parameter)
			: std::runtime_error{
					std_format::format("option explicit set but parameter '{}' does not exist", parameter)} {}
	};

	class dynamic_object
	{
	public:
		using type_name_type = std::string;

		using attribute_name_type = std::string;
		using attributes_type = std::map<attribute_name_type, boxed_value>;

		inline static type_name_type unknown_type_name = "unknown";

	private:
		type_name_type type_name_;
		bool is_explicit_;

		attributes_type attributes_;

	public:
		explicit dynamic_object(type_name_type type_name)
			: type_name_{std::move(type_name)},
			  is_explicit_{false} {}

		dynamic_object()
			: dynamic_object{unknown_type_name} {}

		[[nodiscard]] constexpr bool is_explicit() const noexcept { return is_explicit_; }

		constexpr void set_explicit(const bool value) noexcept { is_explicit_ = value; }

		[[nodiscard]] constexpr const type_name_type& type_name() const noexcept { return type_name_; }

		[[nodiscard]] attributes_type copy_attributes() const { return attributes_; }

		[[nodiscard]] bool has_attribute(const attribute_name_type& name) const { return attributes_.contains(name); }

		[[nodiscard]] boxed_value& get_attribute(const attribute_name_type& name) { return attributes_[name]; }

		[[nodiscard]] const boxed_value& get_attribute(const attribute_name_type& name) const
		{
			if (const auto it = attributes_.find(name);
				it != attributes_.end()) { return it->second; }
			throw std::range_error{std_format::format("Attribute '{}' not found and cannot be added to a const object", name)};
		}

		[[nodiscard]] const boxed_value& method_missing(const attribute_name_type& name) const
		{
			if (is_explicit() && not has_attribute(name)) { throw option_explicit_error{name}; }

			return get_attribute(name);
		}
	};
}

#endif // GAL_LANG_KITS_DYNAMIC_OBJECT_HPP
