#pragma once

#ifndef GAL_LANG_VM_EXCEPTION_HPP
#define GAL_LANG_VM_EXCEPTION_HPP

#include <gal.hpp>

namespace gal::vm
{
	class vm_exception final : public std::exception
	{
	private:
		main_state& state_;
		vm_status status_;

	public:
		constexpr vm_exception(main_state& state, const vm_status status)
			: state_{state},
			  status_{status} {}

		const char* what() const override
		{
			// todo
		}

		[[nodiscard]] constexpr vm_status get_status() const noexcept { return status_; }
	};
}

#endif // GAL_LANG_VM_EXCEPTION_HPP
