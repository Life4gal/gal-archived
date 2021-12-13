#include <vm/gc.hpp>
#include <vm/value.hpp>
#include <allocator.hpp>

#include <unordered_set>

namespace
{
	using object_container_type = std::unordered_set<gal::object*, std::hash<::gal::object*>, std::equal_to<>, ::gal::gal_allocator<gal::object*>>;
	object_container_type objects;
}

namespace gal
{
	void gal_gc::add(object* obj) { objects.insert(obj); }

	void gal_gc::clear()
	{
		auto copy{std::move(objects)};
		objects.clear();

		std::ranges::for_each(copy,
		                      [](object* obj) { object::dtor(obj); });
	}
}
