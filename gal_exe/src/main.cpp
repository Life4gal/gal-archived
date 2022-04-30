#include<utils/format.hpp>
#include<iostream>

#define GAL_LANG_NO_RECODE_CALL_LOCATION_DEBUG
#define GAL_LANG_NO_AST_VISIT_PRINT
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
		auto result = engine.eval_file("test.gal");
	}
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
}

/* test.gal
if var i = 21 * 2; i == 42:
	hello_cpp(3.1415926)
/
else:
	hello_cpp(2.7182818)
/

global j = 0
while j < 10:
	hello_cpp(j)
	
	if j >= 4:
		if j % 2 == 0:
			hello_cpp(-j)
		/
		else:
			hello_cpp(42)
			break
		/
	/
	j += 1
/
 */
