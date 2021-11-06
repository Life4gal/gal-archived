#include "context.hpp"

#include <algorithm>

namespace gal
{
	decltype(context_scope::named_variables_)::iterator context_scope::get_local_variable_helper(ast_identifier::view_type name)
	{
		return std::ranges::find(named_variables_, name, [](const ast_identifier& identifier) -> decltype(auto)
								 { return identifier.get_name(); });
	}

	decltype(context_scope::named_variables_)::const_iterator context_scope::get_local_variable_helper(ast_identifier::view_type name) const
	{
		return const_cast<context_scope&>(*this).get_local_variable_helper(name);
	}

	decltype(context_scope::children_scope_)::iterator context_scope::get_child_scope_helper(identifier_view_type name)
	{
		return std::ranges::find(children_scope_, name, [](const context_scope& scope) -> decltype(auto)
								 { return scope.get_scope_name(); });
	}

	decltype(context_scope::children_scope_)::const_iterator context_scope::get_child_scope_helper(identifier_view_type name) const
	{
		return const_cast<context_scope&>(*this).get_child_scope_helper(name);
	}

	bool context_scope::is_local_variable(ast_identifier::view_type name) const noexcept
	{
		return get_local_variable_helper(name) != named_variables_.end();
	}

	void context_scope::add_local_variable(ast_identifier identifier)
	{
		named_variables_.push_back(std::move(identifier));
	}

	ast_identifier& context_scope::get_local_variable(ast_identifier::view_type name)
	{
		auto result = get_local_variable_helper(name);
		if (result != named_variables_.end())
		{
			return *result;
		}

		static ast_identifier not_exist{"unknown identifier"};
		return not_exist;
	}

	const ast_identifier& context_scope::get_local_variable(ast_identifier::view_type name) const
	{
		return const_cast<context_scope&>(*this).get_local_variable(name);
	}

	bool context_scope::is_child_scope(identifier_view_type name) const noexcept
	{
		return get_child_scope_helper(name) != children_scope_.end();
	}

	void context_scope::add_child_scope(context_scope scope)
	{
		children_scope_.push_back(std::move(scope));
	}

	context_scope& context_scope::get_child_scope(identifier_view_type name)
	{
		auto result = get_child_scope_helper(name);
		if (result != children_scope_.end())
		{
			return *result;
		}

		static context_scope not_exist{"unknown scope"};
		return not_exist;
	}

	const context_scope& context_scope::get_child_scope(identifier_view_type name) const
	{
		return const_cast<context_scope&>(*this).get_child_scope(name);
	}
}// namespace gal
