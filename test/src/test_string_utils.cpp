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

	ASSERT_EQ(vec.size(), 5);
	ASSERT_EQ(vec[0], "https:");
	ASSERT_EQ(vec[1], "//");
	ASSERT_EQ(vec[2], "cppreference");
	ASSERT_EQ(vec[3], ".");
	ASSERT_EQ(vec[4], "com");
}

TEST(TestStringUtils, TestSplitContainer)
{
	constexpr auto				  bits = "https:*//*cppreference*.*com"sv;

	std::vector<std::string_view> vec{};
	split("*"sv, bits, vec);

	ASSERT_EQ(vec.size(), 5);
	ASSERT_EQ(vec[0], "https:");
	ASSERT_EQ(vec[1], "//");
	ASSERT_EQ(vec[2], "cppreference");
	ASSERT_EQ(vec[3], ".");
	ASSERT_EQ(vec[4], "com");
}
