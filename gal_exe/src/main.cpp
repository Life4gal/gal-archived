#include<utils/format.hpp>
#include<iostream>

#define GAL_LANG_NO_RECODE_CALL_LOCATION_DEBUG
#define GAL_LANG_NO_AST_VISIT_PRINT
#include <gal/gal.hpp>

void hello_cpp(double d) { std::cout << std_format::format("value: {}\n", d); }

void print_and_change(int& i)
{
	std::cout << std_format::format("value: {}\n", i);
	i = 1;
}

int main()
{
	using namespace gal;

	lang::engine engine{};

	engine.add_function(
			"hello_cpp",
			lang::fun(&hello_cpp));
	engine.add_function(
			"print_and_change",
			lang::fun(&print_and_change));

	try { auto result = engine.eval_file("test.gal"); }
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }

	std::cout << "================================\n";

	try
	{
		int v42 = 42;
		engine.add_global_mutable("v42", lang::var(std::ref(v42)));
		[[maybe_unused]] auto result = engine.eval("print_and_change(v42)");
		std::cout << "v42 after changed: " << v42 << '\n';
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
