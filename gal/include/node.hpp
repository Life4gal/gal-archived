#pragma once

#ifndef GAL_LANG_NODE_HPP
	#define GAL_LANG_NODE_HPP

	#include <string>
	#include <string_view>
	#include <memory>

namespace gal
{
	class gal_expression;
	class gal_identifier;

	template<typename T, typename... Args>
	std::unique_ptr<gal_expression> make_expression(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	/*! Represents an identifier. */
	using identifier_type	   = std::string;
	/*! Represents an identifier view. */
	using identifier_view_type = std::string_view;
	/*! Represents an expression. */
	using expression_type	   = std::unique_ptr<gal_expression>;

	/*! Represents an expression. */
	class gal_expression
	{
	public:
		/*! Type of the AST node */
		enum class e_expression_type
		{
			// todo: fill it!
			statement_t,
			object_t,
			identifier_t,
			function_t,
			branch_t,
		};

		gal_expression()											= default;

		virtual ~gal_expression()									= 0;
		gal_expression(const gal_expression&)						= default;
		gal_expression(gal_expression&&)							= default;
		gal_expression&										operator=(const gal_expression&) = default;
		gal_expression&										operator=(gal_expression&&) = default;

		/*! Returns the type of the node. */
		[[nodiscard]] constexpr virtual e_expression_type	get_type() const noexcept	= 0;

		/*! Returns the textual representation. */
		[[nodiscard]] virtual std::string					to_string() const noexcept { return "expression"; }
	};

	/*! Represents a statement. */
	class gal_statement : public gal_expression
	{
	public:
		[[nodiscard]] constexpr e_expression_type	get_type() const noexcept override { return e_expression_type::statement_t; }
		[[nodiscard]] std::string					to_string() const noexcept override { return "statement"; }
	};

	/*! Represents an identifier. */
	class gal_identifier final : public gal_expression
	{
	public:
		using value_type = identifier_type;
		using view_type	 = identifier_view_type;

	private:
		value_type name_;

	public:
		explicit gal_identifier(value_type name) : name_(std::move(name)) {}

		[[nodiscard]] constexpr e_expression_type	get_type() const noexcept override { return e_expression_type::identifier_t; }
		[[nodiscard]] std::string					to_string() const noexcept override { return "identifier"; }

		[[nodiscard]] const value_type&				get_name() const noexcept { return name_; }
	};
}// namespace gal

#endif//GAL_LANG_NODE_HPP
