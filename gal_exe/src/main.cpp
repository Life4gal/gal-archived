#include <gal/gal.hpp>

int plus_2(const int a, const int b) noexcept { return a + b; }

int main()
{
	using namespace gal;

	lang::engine engine{};

	engine.add_function(
			"plus_2",
			lang::fun(&plus_2));

	try
	{
		auto result = engine.eval(
				R"(
					for var i = 0; i < 42; i += 1:
						plus_2(i, 42 - i)
				)");
	}
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
}
