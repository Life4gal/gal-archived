#include <gal/defines.hpp>

#include <gal/kits/return_handler.hpp>
#include <gal/kits/proxy_function.hpp>
#include <gal/kits/call_function.hpp>
#include <gal/kits/register_function.hpp>
#include <gal/kits/bootstrap.hpp>
#include <gal/kits/proxy_constructor.hpp>
#include <gal/kits/dispatch.hpp>
#include <gal/kits/operators.hpp>
#include <gal/kits/utility.hpp>
#include <gal/language/eval.hpp>

#include <iostream>

int main()
{
	std::cout << "hello GAL ast_node: " << gal::lang::ast_node_type_name::value << '\n';
}
