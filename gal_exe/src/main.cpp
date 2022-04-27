#include<utils/format.hpp>
#include<iostream>

// #define GAL_LANG_NO_RECODE_CALL_LOCATION_DEBUG
#include <gal/gal.hpp>

void hello_cpp(double d) { std::cout << std_format::format("value: {}\n", d); }

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
				R"(
							if var i = 21 * 2; i == 42:
								hello_cpp(3.1415926)
							/
							else:
								hello_cpp(2.7182818)
							/
						)");
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}
