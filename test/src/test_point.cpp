#include <gtest/gtest.h>

#include <utils/point.hpp>

using namespace gal;

TEST(TestPoint, TestPoint)
{
	constexpr point p0{0, 0};
	constexpr point p1{0, 0};
	constexpr point p2{10, 10};
	constexpr point p3{3, 7};
	constexpr point p4{7, 3};

	static_assert(p0 == p1);
	static_assert(p0 != p2);
	static_assert(p1 < p2);
	static_assert(p1 <= p2);
	static_assert(p2 > p1);
	static_assert(p2 >= p1);
	static_assert(p3 < p4);
	static_assert(p3 <= p4);
	static_assert(p4 > p3);
	static_assert(p4 >= p3);

	constexpr point p5{5, 5};
	constexpr point p6{5, 4};
	constexpr point p7{5, 6};

	static_assert(p5 > p6);
	static_assert(p5 < p7);
}

TEST(TestPoint, TestLine)
{
	constexpr line l0{{0, 0}, {10, 10}};
	constexpr line l1{{0, 0}, {10, 10}};
	constexpr line l2{{3, 7}, {7, 3}};

	constexpr point p0{0, 0};
	constexpr point p1{5, 5};
	constexpr point p2{10, 10};

	static_assert(l0 == l1);
	static_assert(l0 != l2);

	static_assert(l0.contain<true>(p0));
	static_assert(l0.contain<false>(p0));
	static_assert(l0.contain<true>(p1));
	static_assert(l0.contain<false>(p1));
	static_assert(l0.contain<true>(p2));
	static_assert(not l0.contain<false>(p2));
}
