#pragma once

#ifndef GAL_LANG_BOXED_CAST_HPP
#define GAL_LANG_BOXED_CAST_HPP

#include <gal/foundation/boxed_cast.hpp>

namespace gal::lang
{
	/**
	 * @brief Used to register a to parent class relationship with GAL.
	 * Necessary if you want automatic conversions up your inheritance hierarchy.
	 *
	 * @note Create a new to class registration for applying to a module or to the
	 * GAL engine.
	 */
	template<typename Base, typename Derived>
	[[nodiscard]] foundation::type_conversion_type make_base_conversion()
	{
		// Can only be used with related polymorphic types
		// may be expanded some day to support conversions other than child -> parent
		static_assert(std::is_base_of_v<Base, Derived>, "Classes are not related by inheritance");

		if constexpr (std::is_polymorphic_v<Base> && std::is_polymorphic_v<Derived>) { return foundation::make_type_conversion<foundation::boxed_cast_detail::dynamic_conversion<Base, Derived>>(); }
		else { return foundation::make_type_conversion<foundation::boxed_cast_detail::static_conversion<Base, Derived>>(); }
	}

	template<typename Callable>
	[[nodiscard]] foundation::type_conversion_type make_convert_function(const foundation::gal_type_info& from, const foundation::gal_type_info& to, const Callable& function) { return foundation::make_type_conversion<foundation::boxed_cast_detail::type_conversion<Callable>>(from, to, function); }

	template<typename From, typename To, typename Callable>
	[[nodiscard]] foundation::type_conversion_type make_convert_function(const Callable& function)
	{
		return make_convert_function(
				foundation::make_type_info<From>(),
				foundation::make_type_info<To>(),
				[function](const foundation::boxed_value& object)
				{
					// not even attempting to call boxed_cast so that we don't get caught in some call recursion
					return foundation::boxed_value{function(foundation::boxed_cast_detail::cast_helper<const From&>::cast(object, nullptr))};
				});
	}

	template<typename From, typename To>
		requires std::is_convertible_v<From, To>
	[[nodiscard]] foundation::type_conversion_type make_convert_function()
	{
		return make_convert_function<From, To>(
				[](const foundation::boxed_value& object)
				{
					// not even attempting to call boxed_cast so that we don't get caught in some call recursion
					return foundation::boxed_value{static_cast<To>(foundation::boxed_cast_detail::cast_helper<From>::cast(object, nullptr))};
				});
	}

	template<typename ValueType, template<typename...> typename Container, typename PushFunction, typename... AnyOther>
		requires requires(Container<ValueType, AnyOther...>& container)
		         {
			         container.reserve(container.size());
			         {
				         std::ranges::begin(container)
			         } -> std::same_as<decltype(std::ranges::end(container))>;
		         } &&
		         (std::is_member_function_pointer_v<PushFunction> && requires(Container<ValueType, AnyOther...>& container, PushFunction push_function)
		         {
			         (container.*push_function)(std::declval<ValueType&&>());
		         }) or (not std::is_member_function_pointer_v<PushFunction> && requires(Container<ValueType, AnyOther...>& container, PushFunction push_function)
		         {
			         push_function(container, std::declval<ValueType&&>());
		         })
	[[nodiscard]] foundation::type_conversion_type make_container_convert_function(PushFunction push_function)
	{
		return make_convert_function(
				foundation::make_type_info<Container<ValueType, AnyOther...>>(),
				foundation::make_type_info<ValueType>(),
				[push_function](const foundation::boxed_value& data)
				{
					const auto& source = foundation::boxed_cast_detail::cast_helper<const Container<foundation::boxed_value, AnyOther>&>::cast(data, nullptr);

					Container<ValueType, AnyOther...> ret{};
					ret.reserve(source.size());
					std::ranges::for_each(
							source,
							[&, push_function](ValueType&& value)
							{
								if constexpr (std::is_member_function_pointer_v<PushFunction>) { (ret.*push_function)(std::forward<ValueType>(value)); }
								else { push_function(ret, std::forward<ValueType>(value)); }
							},
							[](const foundation::boxed_value& bv) { return foundation::boxed_cast_detail::cast_helper<ValueType>::cast(bv, nullptr); });

					return foundation::boxed_value{std::move(ret)};
				});
	}

	template<typename KeyType, typename MappedType, template<typename...> typename Container, typename PushFunction, typename... AnyOther>
		requires requires(Container<KeyType, MappedType, AnyOther...>& container)
		         {
			         {
				         std::ranges::begin(container)
			         } -> std::same_as<decltype(std::ranges::end(container))>;
		         } &&
		         (std::is_member_function_pointer_v<PushFunction> && requires(Container<KeyType, MappedType, AnyOther...>& container, PushFunction push_function)
		         {
			         (container.*push_function)(std::declval<std::pair<KeyType, MappedType>&&>());
		         }) or
		         (not std::is_member_function_pointer_v<PushFunction> && requires(Container<KeyType, MappedType, AnyOther...>& container, PushFunction push_function)
		         {
			         push_function(container, std::declval<std::pair<KeyType, MappedType>&&>());
		         })
	[[nodiscard]] foundation::type_conversion_type make_container_convert_function(PushFunction push_function)
	{
		return make_convert_function(
				foundation::make_type_info<Container<KeyType, MappedType, AnyOther...>>(),
				foundation::make_type_info<MappedType>(),
				[push_function](const foundation::boxed_value& data)
				{
					const auto& source = foundation::boxed_cast_detail::cast_helper<const Container<KeyType, foundation::boxed_value, AnyOther...>&>::cast(data, nullptr);

					Container<KeyType, MappedType, AnyOther...> ret{};
					std::ranges::for_each(
							source,
							[&, push_function](std::pair<KeyType, MappedType>&& pair)
							{
								if constexpr (std::is_member_function_pointer_v<PushFunction>) { (ret.*push_function)(std::forward<std::pair<KeyType, MappedType>>(pair)); }
								else { push_function(ret, std::forward<std::pair<KeyType, MappedType>>(pair)); }
							},
							[](const auto& pair) { return std::make_pair(pair.first, foundation::boxed_cast_detail::cast_helper<MappedType>::cast(pair.second, nullptr)); });

					return foundation::boxed_value{std::move(ret)};
				});
	}

	/**
	 * @throw bad_boxed_cast(std::bad_cast)
	 */
	template<typename T>
	[[nodiscard]] decltype(auto) boxed_cast(const foundation::boxed_value& object, const foundation::type_conversion_state* conversion = nullptr)
	{
		if (not conversion ||
		    object.type_info().bare_equal(foundation::make_type_info<T>()) ||
		    (not conversion->operator*().is_convertible_type<T>()))
		{
			try
			{
				return foundation::boxed_cast_detail::cast_invoker<T>::cast(object, conversion);
			}
			catch (const std::bad_any_cast&) { }
		}

		if (conversion && conversion->operator*().is_convertible_type<T>())
		{
			try
			{
				// We will not catch any bad_boxed_dynamic_cast that is thrown, let the user get it
				// either way, we are not responsible if it doesn't work
				return foundation::boxed_cast_detail::cast_helper<T>::cast(conversion->operator*().boxed_type_conversion<T>(conversion->saves(), object), conversion);
			}
			catch (...)
			{
				try
				{
					// try going the other way
					return foundation::boxed_cast_detail::cast_helper<T>::cast(conversion->operator*().boxed_type_down_conversion<T>(conversion->saves(), object), conversion);
				}
				catch (const std::bad_any_cast&) { throw foundation::exception::bad_boxed_cast{object.type_info(), typeid(T)}; }
			}
		}

		// If it's not convertible, just throw the error, don't waste the time on the
		// attempted dynamic_cast
		throw foundation::exception::bad_boxed_cast{object.type_info(), typeid(T)};
	}
}

#endif // GAL_LANG_BOXED_CAST_HPP
