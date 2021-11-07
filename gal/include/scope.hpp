#pragma once

#ifndef GAL_LANG_SCOPE_HPP
	#define GAL_LANG_SCOPE_HPP

	#include <ast_node.hpp>
	#include <function.hpp>
	#include <object.hpp>
	#include <map>
	#include <vector>

namespace gal
{
	/*! Represents a scope. */
	class gal_scope
	{
	public:
		// this std::less<> just for 'is_transparent'
		using scoped_variables_type			  = std::map<identifier_type, object_type, std::less<>>;
		using scoped_functions_type			  = std::vector<function_type>;

		using scoped_variables_iterator		  = scoped_variables_type::iterator;
		using scoped_variables_const_iterator = scoped_variables_type::const_iterator;

		using scoped_functions_iterator		  = scoped_functions_type::iterator;
		using scoped_functions_const_iterator = scoped_functions_type::const_iterator;

	private:
		identifier_type		  name_;
		scoped_variables_type variables_;
		scoped_functions_type functions_;

		gal_scope*			  parent_;

		explicit gal_scope(identifier_type name, gal_scope* parent = nullptr)
			: name_(std::move(name)),
			  parent_(parent)
		{
		}

		/*! Get current scope name. */
		[[nodiscard]] const identifier_type&		  get_scope_name() const noexcept { return name_; }

		///////////////// parent ///////////////////////////

		/*! Set current scope's parent. */
		void										  set_parent(gal_scope* parent) { parent_ = parent; }

		/*! Does current scope has a parent. */
		[[nodiscard]] bool							  has_parent() const noexcept { return parent_; }

		/*! Get parent scope name, regardless the parent valid or not. */
		[[nodiscard]] const identifier_type&		  get_parent_name() const noexcept { return parent_->get_scope_name(); }

		///////////////// variable ///////////////////////////

		/*! Get an scoped variables begin iterator. */
		[[nodiscard]] scoped_variables_iterator		  scoped_variables_begin() noexcept { return variables_.begin(); }

		/*! Get an scoped variables begin iterator. */
		[[nodiscard]] scoped_variables_const_iterator scoped_variables_begin() const noexcept { return variables_.begin(); }

		/*! Get an scoped variables end iterator. */
		[[nodiscard]] scoped_variables_iterator		  scoped_variables_end() noexcept { return variables_.end(); }

		/*! Get an scoped variables end iterator. */
		[[nodiscard]] scoped_variables_const_iterator scoped_variables_end() const noexcept { return variables_.end(); }

		/*! Get an scoped variables iterator in this scope, if it not exist, return scoped_variables_end(). */
		[[nodiscard]] scoped_variables_iterator		  get_this_scoped_variable(gal_identifier::view_type name) noexcept;

		/*! Get an scoped variables iterator in this scope, if it not exist, return scoped_variables_end(). */
		[[nodiscard]] scoped_variables_const_iterator get_this_scoped_variable(gal_identifier::view_type name) const noexcept;

		/*! Is there is a variable named 'name' in this scope. */
		[[nodiscard]] bool							  is_this_scope_exist_variable(gal_identifier::view_type name) const noexcept;

		/*! Get an scoped variables iterator in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist), if it not exist, return scoped_variables_end(). */
		[[nodiscard]] scoped_variables_iterator		  get_variable(gal_identifier::view_type name) noexcept;

		/*! Get an scoped variables iterator in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist), if it not exist, return scoped_variables_end(). */
		[[nodiscard]] scoped_variables_const_iterator get_variable(gal_identifier::view_type name) const noexcept;

		/*! Is there is a variable named 'name' in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist). */
		[[nodiscard]] bool							  is_exist_variable(gal_identifier::view_type name) const noexcept;

		/*! Get an pointer point to a scope where variable in, if variable not exist any scope(include parent's scope), return nullptr. */
		[[nodiscard]] gal_scope*					  get_variable_scope(gal_identifier::view_type name) noexcept;

		/*! Get an pointer point to a scope where variable in, if variable not exist any scope(include parent's scope), return nullptr. */
		[[nodiscard]] const gal_scope*				  get_variable_scope(gal_identifier::view_type name) const noexcept;

		///////////////// function ///////////////////////////

		[[nodiscard]] scoped_functions_iterator		  scoped_functions_begin() noexcept { return functions_.begin(); }

		[[nodiscard]] scoped_functions_const_iterator scoped_functions_begin() const noexcept { return functions_.begin(); }

		[[nodiscard]] scoped_functions_iterator		  scoped_functions_end() noexcept { return functions_.end(); }

		[[nodiscard]] scoped_functions_const_iterator scoped_functions_end() const noexcept { return functions_.end(); }

		/*! Get an scoped functions iterator in this scope, if it not exist, return scoped_functions_end(). */
		[[nodiscard]] scoped_functions_iterator		  get_this_scoped_function(gal_identifier::view_type name) noexcept;

		/*! Get an scoped functions iterator in this scope, if it not exist, return scoped_functions_end(). */
		[[nodiscard]] scoped_functions_const_iterator get_this_scoped_function(gal_identifier::view_type name) const noexcept;

		/*! Is there is a function named 'name' in this scope. */
		[[nodiscard]] bool							  is_this_scope_exist_function(gal_identifier::view_type name) const noexcept;

		/*! Get an scoped functions iterator in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist), if it not exist, return scoped_functions_end(). */
		[[nodiscard]] scoped_functions_iterator		  get_function(gal_identifier::view_type name) noexcept;

		/*! Get an scoped functions iterator in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist), if it not exist, return scoped_functions_end(). */
		[[nodiscard]] scoped_functions_const_iterator get_function(gal_identifier::view_type name) const noexcept;

		/*! Is there is a function named 'name' in this scope or parent scope(or parent's parent scope, until there are no more parent scopes exist). */
		[[nodiscard]] bool							  is_exist_function(gal_identifier::view_type name) const noexcept;

		/*! Get an pointer point to a scope where function in, if function not exist any scope(include parent's scope), return nullptr. */
		[[nodiscard]] gal_scope*					  get_function_scope(gal_identifier::view_type name) noexcept;

		/*! Get an pointer point to a scope where function in, if function not exist any scope(include parent's scope), return nullptr. */
		[[nodiscard]] const gal_scope*				  get_function_scope(gal_identifier::view_type name) const noexcept;
	};
}// namespace gal

#endif//GAL_LANG_SCOPE_HPP
