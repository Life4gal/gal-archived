#pragma once

#ifndef GAL_LANG_BRANCH_HPP
	#define GAL_LANG_BRANCH_HPP

	#include <ast_node.hpp>
	#include <vector>

namespace gal
{
	// todo: branches not implemented yet

	class gal_branch;
	using branch_type = std::unique_ptr<gal_branch>;

	/*! Represents a gal_branch. */
	class gal_branch : public gal_expression
	{
	public:
		enum class e_branch_type
		{
			branch_if_t,
			branch_else_t,
			branch_elif_t
		};

		[[nodiscard]] constexpr e_expression_type	  get_type() const noexcept override { return e_expression_type::branch_t; }
		[[nodiscard]] std::string					  to_string() const noexcept override { return "branch"; }

		/*! Return the type of the branch. */
		[[nodiscard]] constexpr virtual e_branch_type get_branch_type() const noexcept = 0;
	};

	/*! Represents a else gal_branch. */
	class gal_branch_else final : public gal_branch
	{
	private:
		expression_type body_;

	public:
		explicit gal_branch_else(expression_type body) : body_(std::move(body)) {}

		[[nodiscard]] constexpr e_branch_type get_branch_type() const noexcept override { return e_branch_type::branch_else_t; }
	};

	/*! Represents a if gal_branch base. */
	class gal_branch_if_base : public gal_branch
	{
	private:
		expression_type condition_;
		expression_type body_;

		branch_type		next_;

		gal_branch_if_base(expression_type condition, expression_type body) : condition_(std::move(condition)),
																			  body_(std::move(body)) {}

		/*! Set next branch. */
		void set_next_branch(branch_type branch) { next_ = std::move(branch); }
	};

	/*! Represents a if gal_branch. */
	class gal_branch_if final : public gal_branch_if_base
	{
	public:
		[[nodiscard]] constexpr e_branch_type get_branch_type() const noexcept override { return e_branch_type::branch_if_t; }
	};

	/*! Represents a elif gal_branch. */
	class gal_branch_elif final : public gal_branch_if_base
	{
	public:
		[[nodiscard]] constexpr e_branch_type get_branch_type() const noexcept override { return e_branch_type::branch_elif_t; }
	};
}// namespace gal

#endif//GAL_LANG_BRANCH_HPP
