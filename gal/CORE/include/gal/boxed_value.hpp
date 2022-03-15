#pragma once

#ifndef GAL_LANG_BOXED_VALUE_HPP
#define GAL_LANG_BOXED_VALUE_HPP

#include <gal/foundation/boxed_value.hpp>

namespace gal::lang
{
	namespace boxed_value_detail
	{
		using foundation::boxed_value;

		/**
		 * @brief Takes a value, copies it and returns a boxed_value object that is immutable.
		 * @param object Value to copy and make const
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const T& object) { return boxed_value{std::make_shared<std::add_const_t<T>>(object)}; }

		/**
		 * @brief Takes a pointer to a value, adds const to the pointed to type and returns an
		 * immutable boxed_value. Does not copy the pointed to value.
		 * @param object Pointer to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(T* object) { return boxed_value{const_cast<std::add_const_t<T>*>(object)}; }

		/**
		 * @brief Takes a std::shared_ptr to a value, adds const to the pointed to type and
		 * returns an immutable boxed_value. Does not copy the pointed to value.
		 * @param object Pointer to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const std::shared_ptr<T>& object) { return boxed_value{std::const_pointer_cast<std::add_const_t<T>>(object)}; }

		/**
		 * @brief Takes a std::reference_wrapper value, adds const to the referenced type
		 * and returns an immutable boxed_value. Does not copy the referenced value.
		 * @param object Reference object to make immutable
		 * @return Immutable boxed_value
		 */
		template<typename T>
		boxed_value make_const_boxed_value(const std::reference_wrapper<T>& object) { return boxed_value{std::cref(object.get())}; }
	}

	/**
	 * @brief Creates a boxed_value. If the object passed in is a value type, it is copied.
	 * If it is a pointer, std::shared_ptr, or std::reference_type a copy is not made.
	 *
	 * @param t The value to box
	 */
	template<typename T>
	foundation::boxed_value var(T&& t) { return foundation::boxed_value{std::forward<T>(t)}; }

	/**
	 * @brief Takes an object and returns an immutable boxed_value. If the object is a
	 * std::reference or pointer type the value is not copied.
	 * If it is an object type, it is copied.
	 * @param object Object to make immutable
	 * @return Immutable boxed_value
	 */
	template<typename T>
	foundation::boxed_value const_var(const T& object) { return boxed_value_detail::make_const_boxed_value(object); }

	inline foundation::boxed_value void_var()
	{
		const static foundation::boxed_value v{foundation::boxed_value::void_type{}};
		return v;
	}

	inline foundation::boxed_value const_var(const bool b)
	{
		const static foundation::boxed_value t{boxed_value_detail::make_const_boxed_value(true)};
		const static foundation::boxed_value f{boxed_value_detail::make_const_boxed_value(false)};

		return b ? t : f;
	}
}


#endif // GAL_LANG_BOXED_VALUE_HPP
