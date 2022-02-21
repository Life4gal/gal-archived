#include <gtest/gtest.h>

#include <utils/function_signature.hpp>
#include <gal/kits/return_handler.hpp>
#include <gal/kits/proxy_function.hpp>
#include <gal/kits/call_function.hpp>
#include <gal/kits/register_function.hpp>
#include <gal/kits/bootstrap.hpp>
#include <gal/kits/proxy_constructor.hpp>
#include <gal/kits/dispatch.hpp>

using namespace gal::utils;

TEST(TestFunctionSignature, TestLambda)
{
	using test_type = decltype(make_function_signature([]() noexcept { return 42; }));
	static_assert(std::is_same_v<test_type::return_type, int>);
	static_assert(test_type::is_noexcept);
	static_assert(not test_type::is_member_object);
	static_assert(test_type::is_object);
}

struct test
{
	[[nodiscard]] constexpr int foo(int) const noexcept
	{
		(void)this;
		return 42;
	};

	template<typename T>
	[[nodiscard]] constexpr T bar() const noexcept
	{
		(void)this;
		return T{};
	}

	template<typename T>
	[[nodiscard]] constexpr T baz(const T&) const noexcept
	{
		(void)this;
		return T{};
	}

	int data = 42;
};

TEST(TestFunctionSignature, TestClass)
{
	using foo_type = decltype(make_function_signature(&test::foo));
	static_assert(std::is_same_v<foo_type::return_type, int>);
	static_assert(foo_type::is_noexcept);
	static_assert(not foo_type::is_member_object);
	static_assert(not foo_type::is_object);

	using bar_type = decltype(make_function_signature(&test::bar<double>));
	static_assert(std::is_same_v<bar_type::return_type, double>);
	static_assert(bar_type::is_noexcept);
	static_assert(not bar_type::is_member_object);
	static_assert(not bar_type::is_object);

	using baz_type = decltype(make_function_signature(&test::baz<std::size_t>));
	static_assert(std::is_same_v<baz_type::return_type, std::size_t>);
	static_assert(baz_type::is_noexcept);
	static_assert(not baz_type::is_member_object);
	static_assert(not baz_type::is_object);

	using data_type = decltype(make_function_signature(&test::data));
	static_assert(std::is_same_v<data_type::return_type, int>);
	static_assert(data_type::is_noexcept);
	static_assert(data_type::is_member_object);
	static_assert(not data_type::is_object);
}
