#include <gtest/gtest.h>

#include <utils/string_pool.hpp>
#include <vector>

using namespace gal;

TEST(TestStringPool, TestNotNullTerminatedStrings)
{
	constexpr static bool is_null_terminated = false;

	// char
	{
		using char_type = char;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append("one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		// pass to C API is undefined behavior (not null terminated)
		//		ASSERT_STREQ(strings[0].data(), "one");
		//		ASSERT_STREQ(strings[1].data(), "two");
		//		ASSERT_STREQ(strings[2].data(), "three");
		//		ASSERT_STREQ(strings[3].data(), "four");

		// pass to C API is undefined behavior (not null terminated)
		//		ASSERT_EQ(std::strcmp(strings[0].data(), "one"), 0);
		//		ASSERT_EQ(std::strcmp(strings[1].data(), "two"), 0);
		//		ASSERT_EQ(std::strcmp(strings[2].data(), "three"), 0);
		//		ASSERT_EQ(std::strcmp(strings[3].data(), "four"), 0);

		// call std::string::operation==
		ASSERT_EQ(strings[0], "one");
		ASSERT_EQ(strings[1], "two");
		ASSERT_EQ(strings[2], "three");
		ASSERT_EQ(strings[3], "four");
	}
}

TEST(TestStringPool, TestNullTerminatedStringsChar)
{
	constexpr static bool is_null_terminated = true;
	// char
	{
		using char_type = char;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append("one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		ASSERT_STREQ(strings[0].data(), "one");
		ASSERT_STREQ(strings[1].data(), "two");
		ASSERT_STREQ(strings[2].data(), "three");
		ASSERT_STREQ(strings[3].data(), "four");

		// pass to C API also is ok
		ASSERT_EQ(std::strcmp(strings[0].data(), "one"), 0);
		ASSERT_EQ(std::strcmp(strings[1].data(), "two"), 0);
		ASSERT_EQ(std::strcmp(strings[2].data(), "three"), 0);
		ASSERT_EQ(std::strcmp(strings[3].data(), "four"), 0);
	}
}

TEST(TestStringPool, TestNullTerminatedStringsWChar)
{
	constexpr static bool is_null_terminated = true;
	// char
	{
		using char_type = wchar_t;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append(L"one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{L"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		ASSERT_STREQ(strings[0].data(), L"one");
		ASSERT_STREQ(strings[1].data(), L"two");
		ASSERT_STREQ(strings[2].data(), L"three");
		ASSERT_STREQ(strings[3].data(), L"four");

		// pass to C API also is ok
		ASSERT_EQ(std::wcscmp(strings[0].data(), L"one"), 0);
		ASSERT_EQ(std::wcscmp(strings[1].data(), L"two"), 0);
		ASSERT_EQ(std::wcscmp(strings[2].data(), L"three"), 0);
		ASSERT_EQ(std::wcscmp(strings[3].data(), L"four"), 0);
	}
}

TEST(TestStringPool, TestNullTerminatedStringsChar8)
{
	constexpr static bool is_null_terminated = true;
	// char
	{
		using char_type = char8_t;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append(u8"one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{u8"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		ASSERT_EQ(strings[0], u8"one");
		ASSERT_EQ(strings[1], u8"two");
		ASSERT_EQ(strings[2], u8"three");
		ASSERT_EQ(strings[3], u8"four");
	}
}

TEST(TestStringPool, TestNullTerminatedStringsChar16)
{
	constexpr static bool is_null_terminated = true;
	// char
	{
		using char_type = char16_t;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append(u"one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{u"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		ASSERT_EQ(strings[0], u"one");
		ASSERT_EQ(strings[1], u"two");
		ASSERT_EQ(strings[2], u"three");
		ASSERT_EQ(strings[3], u"four");
	}
}

TEST(TestStringPool, TestNullTerminatedStringsChar32)
{
	constexpr static bool is_null_terminated = true;
	// char
	{
		using char_type = char32_t;

		std::vector<std::basic_string_view<char_type>> strings;
		string_pool<char_type, is_null_terminated> pool;

		strings.emplace_back(pool.append(U"one"));

		// not a null terminated string also is ok
		constexpr std::basic_string_view<char_type> str{U"two three four"};
		strings.emplace_back(pool.append(str.substr(0, 3)));
		strings.emplace_back(pool.append(str.substr(4, 5)));
		strings.emplace_back(pool.append(str.substr(10, 4)));

		ASSERT_EQ(strings[0], U"one");
		ASSERT_EQ(strings[1], U"two");
		ASSERT_EQ(strings[2], U"three");
		ASSERT_EQ(strings[3], U"four");
	}
}

TEST(TestStringPool, TestCustomeCapacity)
{
	// string_pool uses default block capacity of 8192 characters, but a custom value can be specified
	{
		constexpr static bool is_null_terminated = false;

		constexpr std::size_t so_big_size = 1'000'000;
		string_pool<wchar_t, is_null_terminated> so_big_pool{so_big_size};

		const auto str = so_big_pool.append(L"a long long long long long long long long str");
		string_pool<wchar_t, is_null_terminated> so_small_pool{str.length() / 2};

		// if you try to add a string exceeding default block capacity, string_pool will allocate a new block capable of storing the string
		const auto put_it_in = so_small_pool.append(str);

		// cannot use C API
		ASSERT_EQ(put_it_in, L"a long long long long long long long long str");
	}
	{
		constexpr static bool is_null_terminated = true;

		constexpr std::size_t so_big_size = 1'000'000'000;
		string_pool<wchar_t, is_null_terminated> so_big_pool{so_big_size};

		const auto str = so_big_pool.append(L"a long long long long long long long long str");
		string_pool<wchar_t, is_null_terminated> so_small_pool{str.length() / 2};

		const auto put_it_in = so_small_pool.append(str);

		// use C API
		ASSERT_EQ(std::wcscmp(put_it_in.data(), L"a long long long long long long long long str"), 0);
	}
}
