#include "scope.hpp"

#include <algorithm>

namespace gal
{
	gal_scope::scoped_variables_iterator gal_scope::get_this_scoped_variable(gal_identifier::view_type name) noexcept
	{
		return variables_.find(name);
	}

	gal_scope::scoped_variables_const_iterator gal_scope::get_this_scoped_variable(gal_identifier::view_type name) const noexcept
	{
		return variables_.find(name);
	}

	bool gal_scope::is_this_scope_exist_variable(gal_identifier::view_type name) const noexcept
	{
		return get_this_scoped_variable(name) != scoped_variables_end();
	}

	gal_scope::scoped_variables_iterator gal_scope::get_variable(gal_identifier::view_type name) noexcept
	{
		auto* p = this;
		do {
			auto result = p->get_this_scoped_variable(name);
			if (result != p->scoped_variables_end())
			{
				return result;
			}
		} while (parent_ && (p = parent_));

		return scoped_variables_end();
	}

	gal_scope::scoped_variables_const_iterator gal_scope::get_variable(gal_identifier::view_type name) const noexcept
	{
		// todo: silly duplicate, we need `deducing this`!
		auto* p = this;
		do {
			auto result = p->get_this_scoped_variable(name);
			if (result != p->scoped_variables_end())
			{
				return result;
			}
		} while (parent_ && (p = parent_));

		return scoped_variables_end();
	}

	bool gal_scope::is_exist_variable(gal_identifier::view_type name) const noexcept
	{
		return get_variable(name) != scoped_variables_end();
	}

	gal_scope* gal_scope::get_variable_scope(gal_identifier::view_type name) noexcept
	{
		auto* p = this;
		do
		{
			if (p->is_exist_variable(name))
			{
				return p;
			}
		} while (parent_ && (p = parent_));

		return nullptr;
	}

	const gal_scope* gal_scope::get_variable_scope(gal_identifier::view_type name) const noexcept
	{
		// todo: silly duplicate, we need `deducing this`!
		auto* p = this;
		do
		{
			if (p->is_exist_variable(name))
			{
				return p;
			}
		} while (parent_ && (p = parent_));

		return nullptr;
	}

	gal_scope::scoped_functions_iterator gal_scope::get_this_scoped_function(gal_identifier::view_type name) noexcept
	{
		return std::ranges::find(functions_, name, [](const function_type& function) -> decltype(auto)
								 { return function->get_function_name(); });
	}

	gal_scope::scoped_functions_const_iterator gal_scope::get_this_scoped_function(gal_identifier::view_type name) const noexcept
	{
		return std::ranges::find(functions_, name, [](const function_type& function) -> decltype(auto)
								 { return function->get_function_name(); });
	}

	bool gal_scope::is_this_scope_exist_function(gal_identifier::view_type name) const noexcept
	{
		return get_this_scoped_function(name) != scoped_functions_end();
	}

	gal_scope::scoped_functions_iterator gal_scope::get_function(gal_identifier::view_type name) noexcept
	{
		auto* p = this;
		do {
			auto result = p->get_this_scoped_function(name);
			if (result != p->scoped_functions_end())
			{
				return result;
			}
		} while (parent_ && (p = parent_));

		return scoped_functions_end();
	}

	gal_scope::scoped_functions_const_iterator gal_scope::get_function(gal_identifier::view_type name) const noexcept
	{
		// todo: silly duplicate, we need `deducing this`!
		auto* p = this;
		do {
			auto result = p->get_this_scoped_function(name);
			if (result != p->scoped_functions_end())
			{
				return result;
			}
		} while (parent_ && (p = parent_));

		return scoped_functions_end();
	}

	bool gal_scope::is_exist_function(gal_identifier::view_type name) const noexcept
	{
		return get_function(name) != scoped_functions_end();
	}

	gal_scope* gal_scope::get_function_scope(gal_identifier::view_type name) noexcept
	{
		auto* p = this;
		do
		{
			if (p->is_exist_function(name))
			{
				return p;
			}
		} while (parent_ && (p = parent_));

		return nullptr;
	}

	const gal_scope* gal_scope::get_function_scope(gal_identifier::view_type name) const noexcept
	{
		// todo: silly duplicate, we need `deducing this`!
		auto* p = this;
		do
		{
			if (p->is_exist_function(name))
			{
				return p;
			}
		} while (parent_ && (p = parent_));

		return nullptr;
	}


}// namespace gal
