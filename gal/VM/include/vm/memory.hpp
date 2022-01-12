#pragma once

#ifndef GAL_LANG_VM_MEMORY_HPP
#define GAL_LANG_VM_MEMORY_HPP

#include <gal.hpp>

namespace gal::vm
{
	struct memory_manager
	{
		/**
		 * @brief Allocate raw memory on the heap.
		 */
		static void* allocate(thread_state& state, std::size_t size);

		/**
		 * @brief Deallocate raw memory on the heap.
		 */
		static void	 deallocate(thread_state& state, void* ptr, size_t size);

		// todo: does our allocator really support re-allocator?
		static void* memory_re_allocate(thread_state& state, void* ptr, std::size_t current_size, std::size_t needed_size);
	};
}

#endif // GAL_LANG_VM_MEMORY_HPP
