#include <gtest/gtest.h>

#include<utils/template_string.hpp>
#include <utils/format.hpp>
#include <string>
#include <string_view>

using namespace gal::utils;
using namespace std::literals;

/**
 * @brief Unfortunately the fmt library currently only supports formatters of type char.
 * @note But in fact, the standard only requires support for char & wchar_t.(not include char8_t/char16_t/char32_t)
 */
#if __has_include(<format>)
#define COMPILER_SUPPORT_FORMAT_DO(...) __VA_ARGS__
#else
	#define COMPILER_SUPPORT_FORMAT_DO(...)
#endif

TEST(TestTemplateString, TestChar)
{
	static_assert(GAL_UTILS_TEMPLATE_STRING_TYPE("hello world")::match("hello world"));
	static_assert(GAL_UTILS_TEMPLATE_STRING_TYPE("hello world")::match("hello world"sv));
	ASSERT_TRUE(GAL_UTILS_TEMPLATE_STRING_TYPE("hello world")::match("hello world"s));

	using format_type_1 = GAL_UTILS_TEMPLATE_STRING_TYPE("{} is not {}");
	ASSERT_STREQ(std_format::format(format_type_1::value, "hello", "world").c_str(), "hello is not world");
	ASSERT_EQ(std_format::format(format_type_1::value, "hello", "world"), "hello is not world");

	using format_type_2 = GAL_UTILS_TEMPLATE_STRING_TYPE("{:<10} == {:>10}");
	ASSERT_STREQ(std_format::format(format_type_2::value, "hello", "world").c_str(), "hello      ==      world");
	ASSERT_EQ(std_format::format(format_type_2::value, "hello", "world"), "hello      ==      world");
}

TEST(TestTemplateString, TestWchar)
{
	static_assert(GAL_UTILS_TEMPLATE_WSTRING_TYPE("hello world")::match(L"hello world"));
	static_assert(GAL_UTILS_TEMPLATE_WSTRING_TYPE("hello world")::match(L"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_TEMPLATE_WSTRING_TYPE("hello world")::match(L"hello world"s));

	COMPILER_SUPPORT_FORMAT_DO(
			using format_type_1 = GAL_UTILS_TEMPLATE_WSTRING_TYPE("{} is not {}");
			ASSERT_STREQ(std::format(format_type_1::value, L"hello", L"world").c_str(), L"hello is not world");
			ASSERT_EQ(std::format(format_type_1::value, L"hello", L"world"), L"hello is not world");

			using format_type_2 = GAL_UTILS_TEMPLATE_WSTRING_TYPE("{:<10} == {:>10}");
			ASSERT_STREQ(std::format(format_type_2::value, L"hello", L"world").c_str(), L"hello      ==      world");
			ASSERT_EQ(std::format(format_type_2::value, L"hello", L"world"), L"hello      ==      world");
			)
}

TEST(TestTemplateString, Testu8char)
{
	static_assert(GAL_UTILS_TEMPLATE_U8STRING_TYPE("hello world")::match(u8"hello world"));
	static_assert(GAL_UTILS_TEMPLATE_U8STRING_TYPE("hello world")::match(u8"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_TEMPLATE_U8STRING_TYPE("hello world")::match(u8"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_TEMPLATE_U8STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, u8"hello", u8"world").c_str(), u8"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, u8"hello", u8"world"), u8"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_TEMPLATE_U8STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, u8"hello", u8"world").c_str(), u8"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, u8"hello", u8"world"), u8"hello      ==      world");
	// 		)
}

TEST(TestTemplateString, Testu16char)
{
	static_assert(GAL_UTILS_TEMPLATE_U16STRING_TYPE("hello world")::match(u"hello world"));
	static_assert(GAL_UTILS_TEMPLATE_U16STRING_TYPE("hello world")::match(u"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_TEMPLATE_U16STRING_TYPE("hello world")::match(u"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_TEMPLATE_U16STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, u"hello", u"world").c_str(), u"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, u"hello", u"world"), u"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_TEMPLATE_U16STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, u"hello", u"world").c_str(), u"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, u"hello", u"world"), u"hello      ==      world");
	// 		)
}

TEST(TestTemplateString, Testu32char)
{
	static_assert(GAL_UTILS_TEMPLATE_U32STRING_TYPE("hello world")::match(U"hello world"));
	static_assert(GAL_UTILS_TEMPLATE_U32STRING_TYPE("hello world")::match(U"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_TEMPLATE_U32STRING_TYPE("hello world")::match(U"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_TEMPLATE_U32STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, U"hello", U"world").c_str(), U"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, U"hello", U"world"), U"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_TEMPLATE_U32STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, U"hello", U"world").c_str(), U"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, U"hello", U"world"), U"hello      ==      world");
	// 		)
}

TEST(TestTemplateString, TestCharBilateral)
{
	using hello_world_type = GAL_UTILS_BILATERAL_TEMPLATE_STRING_TYPE("hello", "world");

	static_assert(hello_world_type::match_left("hello"));
	static_assert(hello_world_type::match_left("hello"sv));
	ASSERT_TRUE(hello_world_type::match_left("hello"s));

	static_assert(hello_world_type::match_right("world"));
	static_assert(hello_world_type::match_right("world"sv));
	ASSERT_TRUE(hello_world_type::match_right("world"s));
}

TEST(TestTemplateString, TestWcharBilateral)
{
	using hello_world_type = GAL_UTILS_BILATERAL_TEMPLATE_WSTRING_TYPE(L"hello", L"world");

	static_assert(hello_world_type::match_left(L"hello"));
	static_assert(hello_world_type::match_left(L"hello"sv));
	ASSERT_TRUE(hello_world_type::match_left(L"hello"s));

	static_assert(hello_world_type::match_right(L"world"));
	static_assert(hello_world_type::match_right(L"world"sv));
	ASSERT_TRUE(hello_world_type::match_right(L"world"s));
}

TEST(TestTemplateString, Testu8charBilateral)
{
	using hello_world_type = GAL_UTILS_BILATERAL_TEMPLATE_U8STRING_TYPE(u8"hello", u8"world");

	static_assert(hello_world_type::match_left(u8"hello"));
	static_assert(hello_world_type::match_left(u8"hello"sv));
	ASSERT_TRUE(hello_world_type::match_left(u8"hello"s));

	static_assert(hello_world_type::match_right(u8"world"));
	static_assert(hello_world_type::match_right(u8"world"sv));
	ASSERT_TRUE(hello_world_type::match_right(u8"world"s));
}

TEST(TestTemplateString, Testu16charBilateral)
{
	using hello_world_type = GAL_UTILS_BILATERAL_TEMPLATE_U16STRING_TYPE(u"hello", u"world");

	static_assert(hello_world_type::match_left(u"hello"));
	static_assert(hello_world_type::match_left(u"hello"sv));
	ASSERT_TRUE(hello_world_type::match_left(u"hello"s));

	static_assert(hello_world_type::match_right(u"world"));
	static_assert(hello_world_type::match_right(u"world"sv));
	ASSERT_TRUE(hello_world_type::match_right(u"world"s));
}

TEST(TestTemplateString, Testu32charBilateral)
{
	using hello_world_type = GAL_UTILS_BILATERAL_TEMPLATE_U32STRING_TYPE(U"hello", U"world");

	static_assert(hello_world_type::match_left(U"hello"));
	static_assert(hello_world_type::match_left(U"hello"sv));
	ASSERT_TRUE(hello_world_type::match_left(U"hello"s));

	static_assert(hello_world_type::match_right(U"world"));
	static_assert(hello_world_type::match_right(U"world"sv));
	ASSERT_TRUE(hello_world_type::match_right(U"world"s));
}

TEST(TestTemplateString, TestCharSymmetry)
{
	using hello_world_type = GAL_UTILS_SYMMETRY_TEMPLATE_STRING_TYPE("({[{(" ")}]})");

	static_assert(hello_world_type::match_left("({[{("));
	static_assert(hello_world_type::match_left("({[{("sv));
	ASSERT_TRUE(hello_world_type::match_left("({[{("s));

	static_assert(hello_world_type::match_right(")}]})"));
	static_assert(hello_world_type::match_right(")}]})"sv));
	ASSERT_TRUE(hello_world_type::match_right(")}]})"s));
}

TEST(TestTemplateString, TestWcharSymmetry)
{
	using hello_world_type = GAL_UTILS_SYMMETRY_TEMPLATE_WSTRING_TYPE(L"({[{(" L")}]})");

	static_assert(hello_world_type::match_left(L"({[{("));
	static_assert(hello_world_type::match_left(L"({[{("sv));
	ASSERT_TRUE(hello_world_type::match_left(L"({[{("s));

	static_assert(hello_world_type::match_right(L")}]})"));
	static_assert(hello_world_type::match_right(L")}]})"sv));
	ASSERT_TRUE(hello_world_type::match_right(L")}]})"s));
}

TEST(TestTemplateString, Testu8charSymmetry)
{
	using hello_world_type = GAL_UTILS_SYMMETRY_TEMPLATE_U8STRING_TYPE(u8"({[{(" u8")}]})");

	static_assert(hello_world_type::match_left(u8"({[{("));
	static_assert(hello_world_type::match_left(u8"({[{("sv));
	ASSERT_TRUE(hello_world_type::match_left(u8"({[{("s));

	static_assert(hello_world_type::match_right(u8")}]})"));
	static_assert(hello_world_type::match_right(u8")}]})"sv));
	ASSERT_TRUE(hello_world_type::match_right(u8")}]})"s));
}

TEST(TestTemplateString, Testu16charSymmetry)
{
	using hello_world_type = GAL_UTILS_SYMMETRY_TEMPLATE_U16STRING_TYPE(u"({[{(" u")}]})");

	static_assert(hello_world_type::match_left(u"({[{("));
	static_assert(hello_world_type::match_left(u"({[{("sv));
	ASSERT_TRUE(hello_world_type::match_left(u"({[{("s));

	static_assert(hello_world_type::match_right(u")}]})"));
	static_assert(hello_world_type::match_right(u")}]})"sv));
	ASSERT_TRUE(hello_world_type::match_right(u")}]})"s));
}

TEST(TestTemplateString, Testu32charSymmetry)
{
	using hello_world_type = GAL_UTILS_SYMMETRY_TEMPLATE_U32STRING_TYPE(U"({[{(" U")}]})");

	static_assert(hello_world_type::match_left(U"({[{("));
	static_assert(hello_world_type::match_left(U"({[{("sv));
	ASSERT_TRUE(hello_world_type::match_left(U"({[{("s));

	static_assert(hello_world_type::match_right(U")}]})"));
	static_assert(hello_world_type::match_right(U")}]})"sv));
	ASSERT_TRUE(hello_world_type::match_right(U")}]})"s));
}

TEST(TestTemplateString, TestCharMultiple1)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_1("hello world");

	static_assert(hello_world_type::match<0>("hello world"));
	static_assert(hello_world_type::match<0>("hello world"sv));
	ASSERT_TRUE(hello_world_type::match<0>("hello world"s));
}

TEST(TestTemplateString, TestCharMultiple2)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_2("hello", "world");

	static_assert(hello_world_type::match<0>("hello"));
	static_assert(hello_world_type::match<0>("hello"sv));
	ASSERT_TRUE(hello_world_type::match<0>("hello"s));

	static_assert(hello_world_type::match<1>("world"));
	static_assert(hello_world_type::match<1>("world"sv));
	ASSERT_TRUE(hello_world_type::match<1>("world"s));
}

TEST(TestTemplateString, TestCharMultiple3)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_3("hello", " ", "world");

	static_assert(hello_world_type::match<0>("hello"));
	static_assert(hello_world_type::match<0>("hello"sv));
	ASSERT_TRUE(hello_world_type::match<0>("hello"s));

	static_assert(hello_world_type::match<1>(" "));
	static_assert(hello_world_type::match<1>(" "sv));
	ASSERT_TRUE(hello_world_type::match<1>(" "s));

	static_assert(hello_world_type::match<2>("world"));
	static_assert(hello_world_type::match<2>("world"sv));
	ASSERT_TRUE(hello_world_type::match<2>("world"s));
}

TEST(TestTemplateString, TestCharMultiple4)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_4("hello", " ", "world", "!");

	static_assert(hello_world_type::match<0>("hello"));
	static_assert(hello_world_type::match<0>("hello"sv));
	ASSERT_TRUE(hello_world_type::match<0>("hello"s));

	static_assert(hello_world_type::match<1>(" "));
	static_assert(hello_world_type::match<1>(" "sv));
	ASSERT_TRUE(hello_world_type::match<1>(" "s));

	static_assert(hello_world_type::match<2>("world"));
	static_assert(hello_world_type::match<2>("world"sv));
	ASSERT_TRUE(hello_world_type::match<2>("world"s));

	static_assert(hello_world_type::match<3>("!"));
	static_assert(hello_world_type::match<3>("!"sv));
	ASSERT_TRUE(hello_world_type::match<3>("!"s));
}

TEST(TestTemplateString, TestCharMultiple5)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_5("hello", " ", "world", "!", "?");

	static_assert(hello_world_type::match<0>("hello"));
	static_assert(hello_world_type::match<0>("hello"sv));
	ASSERT_TRUE(hello_world_type::match<0>("hello"s));

	static_assert(hello_world_type::match<1>(" "));
	static_assert(hello_world_type::match<1>(" "sv));
	ASSERT_TRUE(hello_world_type::match<1>(" "s));

	static_assert(hello_world_type::match<2>("world"));
	static_assert(hello_world_type::match<2>("world"sv));
	ASSERT_TRUE(hello_world_type::match<2>("world"s));

	static_assert(hello_world_type::match<3>("!"));
	static_assert(hello_world_type::match<3>("!"sv));
	ASSERT_TRUE(hello_world_type::match<3>("!"s));

	static_assert(hello_world_type::match<4>("?"));
	static_assert(hello_world_type::match<4>("?"sv));
	ASSERT_TRUE(hello_world_type::match<4>("?"s));
}

TEST(TestTemplateString, TestCharMultiple6)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_6("h", "e", "l", "l", "o", "world");

	static_assert(hello_world_type::match<0>("h"));
	static_assert(hello_world_type::match<0>("h"sv));
	ASSERT_TRUE(hello_world_type::match<0>("h"s));

	static_assert(hello_world_type::match<1>("e"));
	static_assert(hello_world_type::match<1>("e"sv));
	ASSERT_TRUE(hello_world_type::match<1>("e"s));

	static_assert(hello_world_type::match<2>("l"));
	static_assert(hello_world_type::match<2>("l"sv));
	ASSERT_TRUE(hello_world_type::match<2>("l"s));

	static_assert(hello_world_type::match<3>("l"));
	static_assert(hello_world_type::match<3>("l"sv));
	ASSERT_TRUE(hello_world_type::match<3>("l"s));

	static_assert(hello_world_type::match<4>("o"));
	static_assert(hello_world_type::match<4>("o"sv));
	ASSERT_TRUE(hello_world_type::match<4>("o"s));

	static_assert(hello_world_type::match<5>("world"));
	static_assert(hello_world_type::match<5>("world"sv));
	ASSERT_TRUE(hello_world_type::match<5>("world"s));
}

TEST(TestTemplateString, TestCharMultiple7)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_7("h", "e", "l", "l", "o", " ", "world");

	static_assert(hello_world_type::match<0>("h"));
	static_assert(hello_world_type::match<0>("h"sv));
	ASSERT_TRUE(hello_world_type::match<0>("h"s));

	static_assert(hello_world_type::match<1>("e"));
	static_assert(hello_world_type::match<1>("e"sv));
	ASSERT_TRUE(hello_world_type::match<1>("e"s));

	static_assert(hello_world_type::match<2>("l"));
	static_assert(hello_world_type::match<2>("l"sv));
	ASSERT_TRUE(hello_world_type::match<2>("l"s));

	static_assert(hello_world_type::match<3>("l"));
	static_assert(hello_world_type::match<3>("l"sv));
	ASSERT_TRUE(hello_world_type::match<3>("l"s));

	static_assert(hello_world_type::match<4>("o"));
	static_assert(hello_world_type::match<4>("o"sv));
	ASSERT_TRUE(hello_world_type::match<4>("o"s));

	static_assert(hello_world_type::match<5>(" "));
	static_assert(hello_world_type::match<5>(" "sv));
	ASSERT_TRUE(hello_world_type::match<5>(" "s));

	static_assert(hello_world_type::match<6>("world"));
	static_assert(hello_world_type::match<6>("world"sv));
	ASSERT_TRUE(hello_world_type::match<6>("world"s));
}

TEST(TestTemplateString, TestCharMultiple8)
{
	using hello_world_type = GAL_UTILS_MULTIPLE_TEMPLATE_STRING_TYPE_8("h", "e", "l", "l", "o", " ", "world", "!");

	static_assert(hello_world_type::match<0>("h"));
	static_assert(hello_world_type::match<0>("h"sv));
	ASSERT_TRUE(hello_world_type::match<0>("h"s));

	static_assert(hello_world_type::match<1>("e"));
	static_assert(hello_world_type::match<1>("e"sv));
	ASSERT_TRUE(hello_world_type::match<1>("e"s));

	static_assert(hello_world_type::match<2>("l"));
	static_assert(hello_world_type::match<2>("l"sv));
	ASSERT_TRUE(hello_world_type::match<2>("l"s));

	static_assert(hello_world_type::match<3>("l"));
	static_assert(hello_world_type::match<3>("l"sv));
	ASSERT_TRUE(hello_world_type::match<3>("l"s));

	static_assert(hello_world_type::match<4>("o"));
	static_assert(hello_world_type::match<4>("o"sv));
	ASSERT_TRUE(hello_world_type::match<4>("o"s));

	static_assert(hello_world_type::match<5>(" "));
	static_assert(hello_world_type::match<5>(" "sv));
	ASSERT_TRUE(hello_world_type::match<5>(" "s));

	static_assert(hello_world_type::match<6>("world"));
	static_assert(hello_world_type::match<6>("world"sv));
	ASSERT_TRUE(hello_world_type::match<6>("world"s));

	static_assert(hello_world_type::match<7>("!"));
	static_assert(hello_world_type::match<7>("!"sv));
	ASSERT_TRUE(hello_world_type::match<7>("!"s));
}
