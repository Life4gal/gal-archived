#pragma once

#ifndef GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP
#define GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP

#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/parameters.hpp>
#include <gal/foundation/string.hpp>
#include <gal/language/name.hpp>
#include <optional>

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
		gal_type_info object_type_;
		bool is_member_;

		static type_infos_type build_param_types(const type_infos_view_type types, const gal_type_info& object_type)
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
				const type_conversion_state& conversion) const noexcept
		{
			if (object.type_info().bare_equal(object_type_))
			{
				try { return name == lang::dynamic_object_type_name::value || name == boxed_cast<const dynamic_object&>(object, &conversion).type_name(); }
				catch (const std::bad_cast&) { return false; }
			}

			if (type.has_value()) { return object.type_info().bare_equal(type.value()); }
			return false;
		}

		[[nodiscard]] bool object_name_match(
				const parameters_view_type objects,
				const string_view_type name,
				const std::optional<gal_type_info>& type,
				const type_conversion_state& conversion) const noexcept
		{
			if (not objects.empty()) { return object_name_match(objects.front(), name, type, conversion); }
			return false;
		}

		[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const type_conversion_state& conversion) const override
		{
			if (object_name_match(params, name_, type_, conversion)) { return (*function_)(params, conversion); }
			throw gal::lang::exception::guard_error{};
		}

	public:
		dynamic_object_function(
				string_type&& name,
				proxy_function&& function,
				const bool is_member = false)
			: proxy_function_base{function->types(), function->get_arity()},
			  name_{std::move(name)},
			  function_{std::move(function)},
			  object_type_{make_type_info<dynamic_object>()},
			  is_member_{is_member} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_object_function(
				string_type&& name,
				proxy_function&& function,
				const gal_type_info& type,
				const bool is_member = false)
			: proxy_function_base{build_param_types(type_infos_view_type{function->types()}, type), function->get_arity()},
			  name_{std::move(name)},
			  function_{std::move(function)},
			  type_{not type.is_undefined() ? std::make_optional(type) : std::nullopt},
			  object_type_{make_type_info<dynamic_object>()},
			  is_member_{is_member} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_object_function(const dynamic_object_function&) = delete;
		dynamic_object_function& operator=(const dynamic_object_function&) = delete;
		dynamic_object_function(dynamic_object_function&&) = default;
		dynamic_object_function& operator=(dynamic_object_function&&) = default;

		[[nodiscard]] constexpr bool is_member_function() const noexcept override { return is_member_; }

		[[nodiscard]] immutable_proxy_functions_type container_functions() const override { return {function_}; }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
		{
			if (const auto* func = dynamic_cast<const dynamic_object_function*>(&other)) { return func->name_ == name_ && *func->function_ == *function_; }
			return false;
		}

		[[nodiscard]] bool match(parameters_view_type params, const type_conversion_state& conversion) const override
		{
			if (object_name_match(params, name_, type_, conversion)) { return function_->match(params, conversion); }
			return false;
		}

		[[nodiscard]] bool is_first_type_match(const boxed_value& object, const type_conversion_state& conversion) const noexcept override { return object_name_match(object, name_, type_, conversion); }
	};

	/**
	 * @brief A proxy_function implementation designed for creating a new dynamic_object
	 * that is automatically guarded based on the first param based on the param's type name
	 */
	class dynamic_object_constructor final : public proxy_function_base
	{
	private:
		string_type name_;
		proxy_function function_;

		static type_infos_type build_param_types(const type_infos_view_type types)
		{
			if (types.empty()) { return {}; }
			return {types.begin() + 1, types.end()};
		}

		[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const override
		{
			parameters_type ps{boxed_value{dynamic_object{name_}, true}};
			ps.reserve(ps.size() + params.size());

			ps.insert(ps.end(), params.begin(), params.end());
			(void)(*function_)(parameters_view_type{ps}, conversion);

			return ps.front();
		}

	public:
		dynamic_object_constructor(
				string_type&& name,
				proxy_function&& function)
			: proxy_function_base{
					  build_param_types(type_infos_view_type{function->types()}),
					  function->get_arity() - 1},
			  name_{std::move(name)},
			  function_{std::move(function)} { gal_assert(function_->get_arity() > 0 || function_->get_arity() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
		{
			const auto* d = dynamic_cast<const dynamic_object_constructor*>(&other);
			return d && d->name_ == name_ && *d->function_ == *function_;
		}

		[[nodiscard]] bool match(const parameters_view_type params, const type_conversion_state& conversion) const override
		{
			parameters_type ps{boxed_value{dynamic_object{name_}}};
			ps.reserve(ps.size() + params.size());

			ps.insert(ps.end(), params.begin(), params.end());
			return function_->match(parameters_view_type{ps}, conversion);
		}
	};
}// namespace gal::lang::foundation

#endif// GAL_LANG_FOUNDATION_DYNAMIC_OBJECT_FUNCTION_HPP
