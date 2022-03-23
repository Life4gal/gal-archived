#pragma once

#ifndef GAL_UTILS_SCOPED_OBJECT_HPP
#define GAL_UTILS_SCOPED_OBJECT_HPP

namespace gal::utils
{
	template<typename Derived>
	struct scoped_object
	{
		constexpr scoped_object() { static_cast<Derived&>(*this).do_construct(); }

		constexpr ~scoped_object() noexcept { static_cast<Derived&>(*this).do_destruct(); }

		scoped_object(const scoped_object&) = delete;
		scoped_object& operator=(const scoped_object&) = delete;
		scoped_object(scoped_object&&) = delete;
		scoped_object& operator=(scoped_object&&) = delete;
	};
}

#endif // GAL_UTILS_SCOPED_OBJECT_HPP
