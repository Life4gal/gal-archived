#include <utils/point.hpp>

#include <utils/format.hpp>

namespace gal
{
	std::string point::to_string() const
	{
		return std_format::format("({}, {})", line, column);
	}

	std::string line::to_string() const
	{
		return std_format::format("{} -> {}", begin.to_string(), end.to_string());
	}
}
