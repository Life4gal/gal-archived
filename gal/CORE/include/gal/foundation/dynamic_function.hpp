#pragma once

#ifndef GAL_LANG_FOUNDATION_DYNAMIC_FUNCTION_HPP
#define GAL_LANG_FOUNDATION_DYNAMIC_FUNCTION_HPP

#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/parameters.hpp>
#include <gal/foundation/name.hpp>
#include <gal/boxed_cast.hpp>
#include <optional>
#include <utils/assert.hpp>

namespace gal::lang::foundation
{
	/**
	 * @brief A function_proxy implementation designed for calling a function
	 * that is automatically guarded based on the first param based on the
	 * param's type name
	 */
	class dynamic_function final : public function_proxy_base
	{
	private:
		string_type name_;
		function_proxy_type function_;
		std::optional<gal_type_info> type_;
		gal_type_info object_type_;
		bool is_member_;

		[[nodiscard]] static type_infos_type build_param_types(const type_infos_view_type types, const gal_type_info& object_type)
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
				const convertor_manager_state& state) const noexcept
		{
			if (object.type_info().bare_equal(object_type_))
			{
				try { return name == dynamic_object_type_name::value || name == boxed_cast<const dynamic_object&>(object, &state).nameof(); }
				catch (const std::bad_cast&) { return false; }
			}

			if (type.has_value()) { return object.type_info().bare_equal(*type); }
			return false;
		}

		[[nodiscard]] bool object_name_match(
				const parameters_view_type objects,
				const string_view_type name,
				const std::optional<gal_type_info>& type,
				const convertor_manager_state& state) const noexcept
		{
			if (not objects.empty()) { return object_name_match(objects.front(), name, type, state); }
			return false;
		}

		[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override
		{
			if (object_name_match(params, name_, type_, state)) { return (*function_)(params, state); }
			throw exception::guard_error{};
		}

	public:
		dynamic_function(
				string_type name,
				function_proxy_type function,
				const bool is_member = false)
			: function_proxy_base{function->type_view(), function->arity_size()},
			  name_{std::move(name)},
			  function_{std::move(function)},
			  object_type_{make_type_info<dynamic_object>()},
			  is_member_{is_member} { gal_assert(function_->arity_size() > 0 || function_->arity_size() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_function(
				const string_view_type name,
				function_proxy_type function,
				const bool is_member = false)
			: dynamic_function{string_type{name}, std::move(function), is_member} {}

		dynamic_function(
				string_type name,
				function_proxy_type function,
				const gal_type_info& type,
				const bool is_member = false)
			: function_proxy_base{build_param_types(function->type_view(), type), function->arity_size()},
			  name_{std::move(name)},
			  function_{std::move(function)},
			  type_{not type.is_undefined() ? std::make_optional(type) : std::nullopt},
			  object_type_{make_type_info<dynamic_object>()},
			  is_member_{is_member} { gal_assert(function_->arity_size() > 0 || function_->arity_size() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_function(
				const string_view_type name,
				function_proxy_type function,
				const gal_type_info& type,
				const bool is_member = false)
			: dynamic_function{string_type{name}, std::move(function), type, is_member} {}

		dynamic_function(const dynamic_function&) = delete;
		dynamic_function& operator=(const dynamic_function&) = delete;
		dynamic_function(dynamic_function&&) = default;
		dynamic_function& operator=(dynamic_function&&) = default;
		~dynamic_function() noexcept override = default;

		[[nodiscard]] bool is_member_function() const noexcept override { return is_member_; }

		[[nodiscard]] const_function_proxies_type overloaded_functions() const override { return {function_}; }

		[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override
		{
			if (const auto* func = dynamic_cast<const dynamic_function*>(&other)) { return func->name_ == name_ && *func->function_ == *function_; }
			return false;
		}

		[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override
		{
			if (object_name_match(params, name_, type_, state)) { return function_->match(params, state); }
			return false;
		}

		[[nodiscard]] bool is_first_type_match(const boxed_value& object, const convertor_manager_state& state) const noexcept override { return object_name_match(object, name_, type_, state); }
	};

	/**
	 * @brief A function_proxy implementation designed for creating a new dynamic_object
	 * that is automatically guarded based on the first param based on the param's type name
	 */
	class dynamic_constructor final : public function_proxy_base
	{
	private:
		string_type name_;
		function_proxy_type function_;

		[[nodiscard]] static type_infos_type build_param_types(const type_infos_view_type types)
		{
			if (types.empty()) { return {}; }
			return {types.begin() + 1, types.end()};
		}

		[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override
		{
			parameters_type ps{};
			ps.reserve(1 + params.size());
			ps.emplace_back(dynamic_object{name_}, true);

			ps.insert(ps.end(), params.begin(), params.end());
			(void)(*function_)(ps, state);

			return ps.front();
		}

	public:
		dynamic_constructor(
				string_type name,
				function_proxy_type function)
			: function_proxy_base{
					  build_param_types(function->type_view()),
					  function->arity_size() - 1},
			  name_{std::move(name)},
			  function_{std::move(function)} { gal_assert(function_->arity_size() > 0 || function_->arity_size() < 0, "dynamic_object_function must have at least one parameter (this)."); }

		dynamic_constructor(
				const string_view_type name,
				function_proxy_type function)
			: dynamic_constructor{string_type{name}, std::move(function)} {}

		[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override
		{
			if (const auto ctor = dynamic_cast<const dynamic_constructor*>(&other)) { return ctor->name_ == name_ && *ctor->function_ == *function_; }
			return false;
		}

		[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override
		{
			parameters_type ps{};
			ps.reserve(1 + params.size());
			ps.emplace_back(dynamic_object{name_});

			ps.insert(ps.end(), params.begin(), params.end());
			return function_->match(ps, state);
		}
	};
}

#endif // GAL_LANG_FOUNDATION_DYNAMIC_FUNCTION_HPP
