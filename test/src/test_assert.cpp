#include <gtest/gtest.h>

#include <utils/assert.hpp>

using namespace gal;

TEST(TestAssert, AssertTrue)
{
	gal_assert(1 + 2 == 3);
}

//TEST(TestAssert, AssertFalse)
//{
//	gal_assert(42 == 0, "42 not equal to 0!");
//}
