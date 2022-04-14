#include <gal/gal.hpp>

// note: we currently only registered string (not registered string_view)
void hello_cpp(const std::string& string, double d, bool b) { std::cout << std_format::format("hello '{}', double: {}, bool: {}\n", string, d, b); }

int main()
{
	using namespace gal;

	lang::engine engine{};

	engine.add_function(
			"hello_cpp",
			lang::fun(&hello_cpp));

	try
	{
		auto result = engine.eval(
				R"(for var i = 0; i < 42; i += 1:
								hello_cpp("gal", i - 42.0, i % 2 == 0)
						)"
				);
	}
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
}
