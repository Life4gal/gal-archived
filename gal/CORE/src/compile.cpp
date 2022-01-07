#include <compile.hpp>
#include <compiler/compiler.hpp>

namespace gal
{
	std::string compile(const std::string_view source, const compile_options option) { return compiler::compile(source, option); }
}
