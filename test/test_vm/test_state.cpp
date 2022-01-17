#include <gtest/gtest.h>

#include<gal.hpp>
#include <iostream>
#include <fstream>

using namespace gal::vm;

TEST(TestState, TestState)
{
	using namespace state;

	const std::ofstream out{"allocator_output.txt"};

	const auto		  previous_buf = std::clog.rdbuf();
	std::clog.rdbuf(out.rdbuf());

	auto* state = new_state();
	destroy_state(*state);

	std::clog.rdbuf(previous_buf);
}
