#include <gtest/gtest.h>

#include<utils/fixed_string.hpp>
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

TEST(TestFixedString, TestChar)
{
	static_assert(GAL_UTILS_FIXED_STRING_TYPE("hello world")::match("hello world"));
	static_assert(GAL_UTILS_FIXED_STRING_TYPE("hello world")::match("hello world"sv));
	ASSERT_TRUE(GAL_UTILS_FIXED_STRING_TYPE("hello world")::match("hello world"s));

	using format_type_1 = GAL_UTILS_FIXED_STRING_TYPE("{} is not {}");
	ASSERT_STREQ(std_format::format(format_type_1::value, "hello", "world").c_str(), "hello is not world");
	ASSERT_EQ(std_format::format(format_type_1::value, "hello", "world"), "hello is not world");

	using format_type_2 = GAL_UTILS_FIXED_STRING_TYPE("{:<10} == {:>10}");
	ASSERT_STREQ(std_format::format(format_type_2::value, "hello", "world").c_str(), "hello      ==      world");
	ASSERT_EQ(std_format::format(format_type_2::value, "hello", "world"), "hello      ==      world");
}

TEST(TestFixedString, TestWchar)
{
	static_assert(GAL_UTILS_FIXED_WSTRING_TYPE("hello world")::match(L"hello world"));
	static_assert(GAL_UTILS_FIXED_WSTRING_TYPE("hello world")::match(L"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_FIXED_WSTRING_TYPE("hello world")::match(L"hello world"s));

	COMPILER_SUPPORT_FORMAT_DO(
			using format_type_1 = GAL_UTILS_FIXED_WSTRING_TYPE("{} is not {}");
			ASSERT_STREQ(std::format(format_type_1::value, L"hello", L"world").c_str(), L"hello is not world");
			ASSERT_EQ(std::format(format_type_1::value, L"hello", L"world"), L"hello is not world");

			using format_type_2 = GAL_UTILS_FIXED_WSTRING_TYPE("{:<10} == {:>10}");
			ASSERT_STREQ(std::format(format_type_2::value, L"hello", L"world").c_str(), L"hello      ==      world");
			ASSERT_EQ(std::format(format_type_2::value, L"hello", L"world"), L"hello      ==      world");
			)
}

TEST(TestFixedString, Testu8char)
{
	static_assert(GAL_UTILS_FIXED_U8STRING_TYPE("hello world")::match(u8"hello world"));
	static_assert(GAL_UTILS_FIXED_U8STRING_TYPE("hello world")::match(u8"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_FIXED_U8STRING_TYPE("hello world")::match(u8"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_FIXED_U8STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, u8"hello", u8"world").c_str(), u8"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, u8"hello", u8"world"), u8"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_FIXED_U8STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, u8"hello", u8"world").c_str(), u8"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, u8"hello", u8"world"), u8"hello      ==      world");
	// 		)
}

TEST(TestFixedString, Testu16char)
{
	static_assert(GAL_UTILS_FIXED_U16STRING_TYPE("hello world")::match(u"hello world"));
	static_assert(GAL_UTILS_FIXED_U16STRING_TYPE("hello world")::match(u"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_FIXED_U16STRING_TYPE("hello world")::match(u"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_FIXED_U16STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, u"hello", u"world").c_str(), u"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, u"hello", u"world"), u"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_FIXED_U16STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, u"hello", u"world").c_str(), u"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, u"hello", u"world"), u"hello      ==      world");
	// 		)
}

TEST(TestFixedString, Testu32char)
{
	static_assert(GAL_UTILS_FIXED_U32STRING_TYPE("hello world")::match(U"hello world"));
	static_assert(GAL_UTILS_FIXED_U32STRING_TYPE("hello world")::match(U"hello world"sv));
	ASSERT_TRUE(GAL_UTILS_FIXED_U32STRING_TYPE("hello world")::match(U"hello world"s));

	// COMPILER_SUPPORT_FORMAT_DO(
	// 		using format_type_1 = GAL_UTILS_FIXED_U32STRING_TYPE("{} is not {}");
	// 		ASSERT_STREQ(std::format(format_type_1::value, U"hello", U"world").c_str(), U"hello is not world");
	// 		ASSERT_EQ(std::format(format_type_1::value, U"hello", U"world"), U"hello is not world");
	//
	// 		using format_type_2 = GAL_UTILS_FIXED_U32STRING_TYPE("{:<10} == {:>10}");
	// 		ASSERT_STREQ(std::format(format_type_2::value, U"hello", U"world").c_str(), U"hello      ==      world");
	// 		ASSERT_EQ(std::format(format_type_2::value, U"hello", U"world"), U"hello      ==      world");
	// 		)
}
