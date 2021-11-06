#pragma once

#ifndef GAL_LANG_CONTEXT_HPP
	#define GAL_LANG_CONTEXT_HPP

	#include <ast_node.hpp>

namespace gal
{
	/*! Represents a scope. */
	class context_scope
	{
	private:
		identifier_type											 name_;

		std::vector<ast_identifier>								 named_variables_;

		std::optional<std::reference_wrapper<context_scope>>	 parent_scope_;
		std::vector<context_scope>								 children_scope_;

		/*! Get a variable iterator in current scope. */
		[[nodiscard]] decltype(named_variables_)::iterator		 get_local_variable_helper(ast_identifier::view_type name);

		/*! Get a variable iterator in current scope. */
		[[nodiscard]] decltype(named_variables_)::const_iterator get_local_variable_helper(ast_identifier::view_type name) const;

		/*! Get a child scope iterator in current scope. */
		[[nodiscard]] decltype(children_scope_)::iterator		 get_child_scope_helper(identifier_view_type name);

		/*! Get a child scope iterator in current scope. */
		[[nodiscard]] decltype(children_scope_)::const_iterator	 get_child_scope_helper(identifier_view_type name) const;

	public:
		explicit context_scope(identifier_type name, std::optional<std::reference_wrapper<context_scope>> parent = std::nullopt)
			: name_(std::move(name)),
			  parent_scope_(parent) {}

		/*! Is current scope has a variable named 'name'. */
		[[nodiscard]] bool					 is_local_variable(ast_identifier::view_type name) const noexcept;

		/*! Add a variable into current scope. */
		void								 add_local_variable(ast_identifier identifier);

		/*! Get a variable in current scope. */
		[[nodiscard]] ast_identifier&		 get_local_variable(ast_identifier::view_type name);

		/*! Get a variable in current scope. */
		[[nodiscard]] const ast_identifier&	 get_local_variable(ast_identifier::view_type name) const;

		/*! Is current scope has a scope named 'name'. */
		[[nodiscard]] bool					 is_child_scope(identifier_view_type name) const noexcept;

		/*! Add a child scope into current children scope. */
		void								 add_child_scope(context_scope scope);

		/*! Get a child scope in current scope. */
		[[nodiscard]] context_scope&		 get_child_scope(identifier_view_type name);

		/*! Get a child scope in current scope. */
		[[nodiscard]] const context_scope&	 get_child_scope(identifier_view_type name) const;

		/*! Get current scope name. */
		[[nodiscard]] const identifier_type& get_scope_name() const noexcept
		{
			return name_;
		}

		/*! Set current scope's parent. */
		void set_parent(std::reference_wrapper<context_scope> parent)
		{
			parent_scope_ = parent;
		}

		/*! Does current scope has a parent. */
		[[nodiscard]] bool has_parent() const noexcept
		{
			return parent_scope_.has_value();
		}

		/*! Get parent scope name, regardless the parent valid or not(if parent is not valid, throw std::bad_optional_access). */
		[[nodiscard]] const identifier_type& get_parent_name() const noexcept
		{
			return parent_scope_.value().get().get_scope_name();
		}
	};

	class context
	{
	private:
		context_scope			  global_scope_;
		std::vector<ast_function> functions_;

	public:
	};
}// namespace gal

#endif//GAL_LANG_CONTEXT_HPP
