#include <iostream>

#include <gal/boxed_value.hpp>
#include <gal/boxed_cast.hpp>
#include <gal/proxy_function.hpp>
#include <gal/foundation/dynamic_object_function.hpp>
#include <gal/language/name.hpp>
#include <gal/functor_maker.hpp>
#include <gal/foundation/dispatcher.hpp>

int main()
{
	std::cout << "hello " << gal::lang::lang::dynamic_object_type_name::value << '\n';
}
