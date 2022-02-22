#pragma once

#ifndef GAL_LANG_KITS_UTILITY_HPP
#define GAL_LANG_KITS_UTILITY_HPP

#include <gal/kits/operators.hpp>
#include <gal/kits/proxy_constructor.hpp>

namespace gal::lang::kits
{
	namespace detail
	{
		using lang::detail::dispatch_function;
	}

	/**
	 * @brief Single step command for registering a class.
	 *
	 * @param m Model to add class to.
	 * @param name Name of the class being registered.
	 * @param constructors Constructors to add.
	 * @param functions Methods to add.
	 *
	 * @code
	 *
	 * register_class(
	 *	m,
	 *	"my_class",
	 *	{
	 *		make_constructor<my_class()>(), // default ctor
	 *		make_constructor<my_class(const my_class&)>() // copy ctor
	 *	},
	 *	{
	 *		{"function1", fun(&my_class::function1)},
	 *		{"function2", fun(&my_class::function2)},
	 *		{"overload_function1", fun(static_cast<std::string(my_class::*)(int)>(&my_class::overload_function1)},
	 *		{"overload_function2", fun(static_cast<std::string(my_class::*)(double)>(&my_class::overload_function2)}
	 *	}
	 * );
	 *
	 * @endcode 
	 */
	template<typename T>
	void register_class(
			engine_module& m,
			const std::string_view name,
			const detail::dispatch_function::functions_type& constructors,
			const engine_module::functions_type& functions
			)
	{
		m.add_type_info(name, utility::make_type_info<T>());

		std::ranges::for_each(
				constructors,
				[&m, name](const auto& ctor) { m.add_function(name, ctor); });

		std::ranges::for_each(
				functions,
				[&m](const auto& pair) { m.add_function(pair.first, pair.second); });
	}

	template<typename T>
	void register_default_constructor(const std::string_view name, engine_module& m) { m.add_function(name, make_constructor<T()>()); }

	/**
	* @brief Adds a copy constructor for the given type to the given Model.
	*
	* @tparam T The type to add a copy constructor for.
	* @param name The name of the type. The copy constructor will be named "name".
	* @param m The Module to add the copy constructor to.
	*/
	template<typename T>
	void register_copy_constructor(const std::string_view name, engine_module& m) { m.add_function(name, make_constructor<T(const T&)>()); }

	/**
	 * @brief Adds a move constructor for the given type to the given Model.
	 *
	 * @tparam T The type to add a move constructor for.
	 * @param name The name of the type. The copy constructor will be named "name".
	 * @param m The Module to add the move constructor to.
	 */
	template<typename T>
	void register_move_constructor(const std::string_view name, engine_module& m) { m.add_function(name, make_constructor<T(T&&)>()); }

	template<typename T, bool NeedMove, bool NeedCopy = true>
	void register_basic_constructor(const std::string_view name, engine_module& m)
	{
		register_default_constructor<T>(name, m);

		if constexpr (NeedCopy) { register_copy_constructor<T>(name, m); }
		else { }

		if constexpr (NeedMove) { register_move_constructor<T>(name, m); }
		else {}
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	void register_arithmetic_boxed_cast(const std::string_view name, engine_module& m)
	{
		m.add_function(
				name,
				fun([](const boxed_number& num) { return num.as<T>(); }));
	}

	template<typename T>
		requires std::is_arithmetic_v<T>
	void register_arithmetic_from_string(const std::string_view name, engine_module& m)
	{
		// todo: prefix?
		engine_module::name_type n{"to_"};
		n.reserve(n.size() + name.size());
		n.append(name);

		m.add_function(
				std::move(n),
				fun(
						[](const std::string_view& string)
						{
							if constexpr (requires { std::from_chars(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), std::declval<T&>()); })
							{
								T t;
								std::from_chars(string.data(), string.data() + string.size(), t);
								return t;
							}
							else { throw std::runtime_error{"Parsing given type is not supported yet"}; }
						}));
	}

	/**
	 * @brief Add all common functions for a POD type. All operators, and common conversions.
	 */
	template<typename T>
		requires std::is_arithmetic_v<T>
	void register_arithmetic(const std::string_view name, engine_module& m)
	{
		m.add_type_info(name, utility::make_type_info<T>());
		register_default_constructor<T>(name, m);
		register_arithmetic_boxed_cast<T>(name, m);
		register_arithmetic_from_string<T>(name, m);

		// todo: prefix?
		engine_module::name_type n{"to_"};
		n.reserve(n.size() + name.size());
		n.append(name);
		m.add_function(std::move(n), fun([](const T t) { return t; }));
	}
}

#endif // GAL_LANG_KITS_UTILITY_HPP
