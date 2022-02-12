#include <gtest/gtest.h>

#include<utils/fixed_string.hpp>
#include <utils/format.hpp>

using namespace gal::utils;

static_assert(GAL_UTILS_FIXED_STRING_TYPE("hello")::match("hello"));
static_assert(GAL_UTILS_FIXED_STRING_TYPE("world")::match("world"));

TEST(TestFixedString, TestFormat)
{
	using format_type_1 = GAL_UTILS_FIXED_STRING_TYPE("{} is not {}");
	EXPECT_STREQ(std_format::format(format_type_1::value, "hello", "world").c_str(), "hello is not world");
	EXPECT_EQ(std_format::format(format_type_1::value, "hello", "world"), "hello is not world");

	using format_type_2 = GAL_UTILS_FIXED_STRING_TYPE("{:<10} == {:>10}");
	EXPECT_STREQ(std_format::format(format_type_2::value, "hello", "world").c_str(), "hello      ==      world");
	EXPECT_EQ(std_format::format(format_type_2::value, "hello", "world"), "hello      ==      world");
}
