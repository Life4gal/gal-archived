#include<utils/format.hpp>
#include<iostream>

#define GAL_LANG_NO_RECODE_CALL_LOCATION_DEBUG
#define GAL_LANG_NO_AST_VISIT_PRINT
#include <gal/gal.hpp>

void print_and_change(int& i)
{
	std::cout << std_format::format("value: {}\n", i);
	i = 1;
}

int main()
{
	using namespace gal;

	lang::engine engine{};

	engine.add_function(
			"print_and_change",
			lang::fun(&print_and_change));

	try { auto result = engine.eval_file("test.gal"); }
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }

	std::cout << "================================\n";

	try
	{
		int v42 = 42;
		engine.add_global_mutable("v42", lang::var(std::ref(v42)));
		[[maybe_unused]] auto result = engine.eval("print_and_change(v42)");
		std::cout << "v42 after changed: " << v42 << '\n';
	}
	catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
}

/* test.gal
def to_string(list l)
{
	var s = "["
	for(var v in l)
	{
		s += to_string(v)
		s += ", "
	}
	# erase ' '
	s.pop_back()
	# overwrite ','
	s[s.size() - 1] = ']'
	return s
}

def to_string(map m)
{
	var s = "{"
	for(var kv in m)
	{
		s += to_string(kv.first)
		s += ": "
		s += to_string(kv.second)
		s += ", "
	}
	# erase ' '
	s.pop_back()
	# overwrite ','
	s[s.size() - 1] = '}'
	return s;
}

def print(list l)
{
	print(to_string(list))
}

def println(list l)
{
	println(to_string(l))
}

def print(map m)
{
	print(to_string(m))
}

def println(map m)
{
	println(to_string(m))
}

println("hello world!")

global s = list()
s.push_back("hello")
s.push_back("world")
s.push_back(42)
println(s.is_typeof("list"))
println(s)

global m = map()
m["hello"] = "world"
m["answer"] = 42
m["list"] := s
println("m['list'] reference to s: ${m["list"].size() == s.size()}")
println(m.is_typeof("map"))
#println(m)

println("print ${range(0, 42, 2)}")
for(var i in range(0, 42, 2))
{
	print("${i} ")
}
print("\n")

global j = 0
while(j < 10)
{
	println(j)
	
	if(j >= 4)
	{
		if(j % 2 == 0)
		{
			println(-j)
		}
		else
		{
			println(42)
			break
		}
	}
	j += 1
}
 */
