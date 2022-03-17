#pragma once

#ifndef GAL_LANG_KITS_DYNAMIC_OBJECT_FUNCTION_HPP
#define GAL_LANG_KITS_DYNAMIC_OBJECT_FUNCTION_HPP

#include <optional>
#include <gal/kits_old/proxy_function.hpp>

namespace gal::lang::kits
{
	/**
	 * @brief A proxy_function implementation designed for calling a function
	 * that is automatically guarded based on the first param based on the
	 * param's type name
	 */
	class dynamic_object_function final : public proxy_function_base
	{
	private:
		std::string type_name_;
		proxy_function function_;
		std::optional<utility::gal_type_info> ti_;
		utility::gal_type_info object_ti_;
		bool is_attribute_;

		static type_infos_type build_param_types(const type_infos_type& types, const utility::gal_type_info& object_ti)
		{
			type_infos_type ret{types};

			gal_assert(ret.size() > 1);
			ret[1] = object_ti;
			return ret;
		}

		bool dynamic_object_type_name_match(
				const boxed_value& object,
				const std::string& name,
				const std::optional<utility::gal_type_info> ti,
				const type_conversion_state& conversion
				) const noexcept
		{
			if (object.type_info().bare_equal(object_ti_))
			{
				try { return name == dynamic_object_type_name::value || boxed_cast<const dynamic_object&>(object, &conversion).type_name() == name; }
				catch (const std::bad_cast&) { return false; }
			}

			if (ti.has_value()) { return object.type_info().bare_equal(ti.value()); }
			return false;
		}

		bool dynamic_object_type_name_match(
				const function_parameters& objects,
				const std::string& name,
				const std::optional<utility::gal_type_info> ti,
				const type_conversion_state& conversion) const noexcept
		{
			if (not objects.empty()) { return dynamic_object_type_name_match(objects.front(), name, ti, conversion); }
			return false;
		}

		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			if (dynamic_object_type_name_match(params, type_name_, ti_, conversion)) { return (*function_)(params, conversion); }
			throw guard_error{};
		}

	public:
		dynamic_object_function(
				std::string type_name,
				proxy_function function,
				const bool is_attribute = false
				)
			: proxy_function_base{function->types(), function->get_arity()},
			  type_name_{std::move(type_name)},
			  function_{std::move(function)},
			  object_ti_{utility::make_type_info<dynamic_object>()},
			  is_attribute_{is_attribute} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_object_function(
				std::string type_name,
				proxy_function function,
				const utility::gal_type_info& ti,
				const bool is_attribute = false)
			: proxy_function_base{build_param_types(function->types(), ti), function->get_arity()},
			  type_name_{std::move(type_name)},
			  function_{std::move(function)},
			  ti_{not ti.is_undefined() ? std::make_optional(ti) : std::nullopt},
			  object_ti_{utility::make_type_info<dynamic_object>()},
			  is_attribute_{is_attribute} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_object_function(const dynamic_object_function&) = delete;
		dynamic_object_function& operator=(const dynamic_object_function&) = delete;
		dynamic_object_function(dynamic_object_function&&) = default;
		dynamic_object_function& operator=(dynamic_object_function&&) = default;

		[[nodiscard]] constexpr bool is_attribute_function() const noexcept override { return is_attribute_; }

		[[nodiscard]] contained_functions_type get_contained_function() const override { return {function_}; }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
		{
			if (const auto* func = dynamic_cast<const dynamic_object_function*>(&other)) { return func->type_name_ == type_name_ && *func->function_ == *function_; }
			return false;
		}

		[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			if (dynamic_object_type_name_match(params, type_name_, ti_, conversion)) { return function_->match(params, conversion); }
			return false;
		}

		[[nodiscard]] bool is_first_type_match(const boxed_value& object, const type_conversion_state& conversion) const noexcept override { return dynamic_object_type_name_match(object, type_name_, ti_, conversion); }
	};

	/**
	 * @brief A proxy_function implementation designed for creating a new dynamic_object
	 * that is automatically guarded based on the first param based on the param's type name
	 */
	class dynamic_object_constructor final : public proxy_function_base
	{
	private:
		std::string type_name_;
		proxy_function function_;

		static type_infos_type build_param_types(const type_infos_type& types)
		{
			if (types.empty()) { return {}; }
			return {types.begin() + 1, types.end()};
		}

		[[nodiscard]] boxed_value do_invoke(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			std::vector ps{boxed_value{dynamic_object{type_name_}, true}};
			ps.reserve(ps.size() + params.size());

			ps.insert(ps.end(), params.begin(), params.end());
			(*function_)(function_parameters{ps}, conversion);

			return ps.front();
		}

	public:
		dynamic_object_constructor(
				std::string type_name,
				proxy_function function
				)
			: proxy_function_base{build_param_types(function->types()), function->get_arity() - 1},
			  type_name_{std::move(type_name)},
			  function_{std::move(function)} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
		{
			const auto* d = dynamic_cast<const dynamic_object_constructor*>(&other);
			return d && d->type_name_ == type_name_ && *d->function_ == *function_;
		}

		[[nodiscard]] bool match(const function_parameters& params, const type_conversion_state& conversion) const override
		{
			std::vector ps{boxed_value{dynamic_object{type_name_}}};

			ps.insert(ps.end(), params.begin(), params.end());
			return function_->match(function_parameters{ps}, conversion);
		}
	};
}

#endif // GAL_LANG_KITS_DYNAMIC_OBJECT_FUNCTION_HPP
