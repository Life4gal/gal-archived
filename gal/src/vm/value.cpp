#include <algorithm>
#include <vm/value.hpp>

namespace gal
{
	object::~object() noexcept = default;

	gal_index_type symbol_table::find(const char* name, object_string::size_type length)
	{
		const auto it = std::ranges::find_if(
				table_,
				[&](const value_type& str)
				{ return str.equal(name, length); });
		if (it != table_.end())
		{
			return std::distance(table_.begin(), it);
		}
		return gal_index_not_exist;
	}

	gal_index_type symbol_table::find(const object_string& string)
	{
		const auto it = std::ranges::find(
				table_,
				string);
		if (it != table_.end())
		{
			return std::distance(table_.begin(), it);
		}
		return gal_index_not_exist;
	}

	void symbol_table::blacken(gal_virtual_machine_state& state)
	{
		std::ranges::for_each(table_, [&](value_type& str)
							  { str.gray(state); });
	}
}// namespace gal
