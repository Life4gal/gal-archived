#include<string>
#include<utils/format.hpp>
#include<iostream>

#include<gal/foundation/type_info.hpp>
#include <gal/foundation/boxed_value.hpp>
#include <gal/boxed_cast.hpp>
#include <gal/foundation/boxed_number.hpp>
#include <gal/boxed_value.hpp>
#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/return_wrapper.hpp>
#include <gal/foundation/function_proxy.hpp>
#include <gal/foundation/dynamic_function.hpp>
#include <gal/foundation/dispatcher.hpp>
#include <gal/language/common.hpp>
#include <gal/language/eval.hpp>
#include <gal/foundation/function_register.hpp>

// note: we currently only registered string (not registered string_view)
void hello_cpp(const std::string& string, double d, bool b) { std::cout << std_format::format("hello '{}', double: {}, bool: {}\n", string, d, b); }

int main() {}
