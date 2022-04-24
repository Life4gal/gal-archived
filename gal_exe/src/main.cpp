#include<string>
#include<utils/format.hpp>
#include<iostream>

#include<gal/foundation/type_info.hpp>
#include <gal/foundation/boxed_value.hpp>
#include <gal/foundation/boxed_cast.hpp>

// note: we currently only registered string (not registered string_view)
void hello_cpp(const std::string& string, double d, bool b) { std::cout << std_format::format("hello '{}', double: {}, bool: {}\n", string, d, b); }

int main() {}
