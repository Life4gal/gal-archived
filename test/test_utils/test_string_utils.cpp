#include <gtest/gtest.h>

#include<utils/string_utils.hpp>

using namespace gal::utils;
using namespace std::literals;

TEST(TestStringUtils, TestJoin)
{
	const auto bits = {"https:"sv, "//"sv, "cppreference"sv, "."sv, "com"sv};

	ASSERT_EQ(join(bits, "*"), "https:*//*cppreference*.*com");
}

TEST(TestStringUtils, TestSplitInserter)
{
	constexpr auto bits = "https:*//*cppreference*.*com"sv;

	std::vector<std::string_view> vec{};
	split("*"sv, bits, std::back_inserter(vec));

	ASSERT_EQ(vec.size(), static_cast<decltype(vec.size())>(5));
	ASSERT_EQ(vec[0], "https:");
	ASSERT_EQ(vec[1], "//");
	ASSERT_EQ(vec[2], "cppreference");
	ASSERT_EQ(vec[3], ".");
	ASSERT_EQ(vec[4], "com");
}

TEST(TestStringUtils, TestSplitContainer)
{
	constexpr auto bits = "https:*//*cppreference*.*com"sv;

	std::vector<std::string_view> vec{};
	split("*"sv, bits, vec);

	ASSERT_EQ(vec.size(), static_cast<decltype(vec.size())>(5));
	ASSERT_EQ(vec[0], "https:");
	ASSERT_EQ(vec[1], "//");
	ASSERT_EQ(vec[2], "cppreference");
	ASSERT_EQ(vec[3], ".");
	ASSERT_EQ(vec[4], "com");
}

TEST(TestStringUtils, TestTrim)
{
	const std::string str0 = "11111abc22222";

	const auto str1 = trim_left(str0, [](const char c) { return is_digit(c); });

	const auto str2 = trim_right(str0, [](const char c) { return is_digit(c); });

	const auto str3 = trim_entire(str0, [](const char c) { return is_digit(c); });

	ASSERT_EQ(str1, "abc22222");
	ASSERT_EQ(str2, "11111abc");
	ASSERT_EQ(str3, "abc");
}
