#include <iostream>

#include <gal/boxed_value.hpp>
#include <gal/boxed_cast.hpp>
#include <gal/proxy_function.hpp>
#include <gal/foundation/dynamic_object_function.hpp>
#include <gal/language/name.hpp>
#include <gal/functor_maker.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/foundation/operator_register.hpp>
#include <gal/function_register.hpp>
#include <gal/exception_handler.hpp>
#include <gal/language/common.hpp>
#include <gal/language/eval.hpp>

int main()
{
	std::cout << "hello " << gal::lang::lang::dynamic_object_type_name::value << '\n';
}
