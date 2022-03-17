#pragma once

#ifndef GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP
	#define GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP

#include <optional>
#include <gal/foundation/string.hpp>
#include <gal/foundation/parameters.hpp>
#include <gal/foundation/dynamic_object.hpp>
#include <gal/language/name.hpp>

namespace gal::lang::foundation
{
	/**
	 * @brief A proxy_function implementation designed for calling a function
	 * that is automatically guarded based on the first param based on the
	 * param's type name
	 */
	class dynamic_object_function final : public proxy_function_base
	{
	private:
		string_type name_;
		proxy_function function_;
		std::optional<gal_type_info> type_;
		gal_type_info				 object_type_;
		bool						 is_member_;

		static type_infos_type		 build_param_types(const type_infos_view_type types, const gal_type_info& object_type)
		{
			auto ret = types.to<type_infos_type>();

			gal_assert(ret.size() > 1);
			ret[1] = object_type;
			return ret;
		}

		[[nodiscard]] bool object_name_match(
		const boxed_value& object,
			const string_view_type name,
			const std::optional<gal_type_info>& type,
			const type_conversion_state& conversion
		) const noexcept
		{
			if (object.type_info().bare_equal(object_type_))
			{
				try
				{
					return name == lang::dynamic_object_type_name::value || name == boxed_cast<const dynamic_object&>(object, &conversion).type_name();
				}
				catch (const std::bad_cast&)
				{
					return false;
				}
			}

			if (type.has_value())
			{
				return object.type_info().bare_equal(type.value());
			}
			return false;
		}

		[[nodiscard]] bool object_name_match(
			const parameters_view_type objects,
			const string_view_type name,
			const std::optional<gal_type_info>& type,
			const type_conversion_state& conversion
		) const noexcept
		{
			if (not objects.empty()) 
				{
				return object_name_match(objects.front(), name, type, conversion);
			}
				return false;
		}

		
	};
}

#endif // GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP
