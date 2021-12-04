#include <gtest/gtest.h>
#include "allocator.hpp"

#include <vector>
#include <iostream>

using namespace gal;

TEST(TestAllocator, Test)
{
	std::vector<int, gal_allocator<int>> vec{};

	vec.reserve(10);
	for(auto i = 0; i < 10; ++i)
	{
		vec.push_back(i);
	}

	for(auto i = 0; i < 5; ++i)
	{
		vec.pop_back();
	}

	vec.shrink_to_fit();

	std::cout << "output vec: ";
	for(auto i : vec)
	{
		std::cout << i << '\t';
	}
	std::cout << '\n';

	vec.clear();
}
