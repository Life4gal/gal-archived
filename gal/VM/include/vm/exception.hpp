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
		thread_status status_;

	public:
		vm_exception(main_state& state, const thread_status status)
			: state_{state},
			  status_{status} {}

		[[nodiscard]] const char* what() const override
		{
			// todo
			return "todo";
		}

		[[nodiscard]] constexpr thread_status get_status() const noexcept { return status_; }
	};
}

#endif // GAL_LANG_VM_EXCEPTION_HPP
