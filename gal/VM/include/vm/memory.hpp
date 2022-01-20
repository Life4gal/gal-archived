#pragma once

#ifndef GAL_LANG_VM_MEMORY_HPP
#define GAL_LANG_VM_MEMORY_HPP

#include <utility>

#ifndef GAL_ALLOCATOR_NO_TRACE
#include <utils/source_location.hpp>
#endif

namespace gal::vm
{
	class main_state;

	struct raw_memory
	{
		// for format
		constexpr static auto object_amount_width	= 4;
		constexpr static auto memory_use_width		 = 8;
		constexpr static auto pointer_address_width = 16;

		/**
		 * @brief Allocate raw memory on the heap.
		 */
		static void* allocate(main_state& state,
		                      std::size_t size
		                      #ifndef GAL_ALLOCATOR_NO_TRACE
		                      ,
		                      const std_source_location& location = std_source_location::current()
		                      #endif
				);

		/**
		 * @brief Deallocate raw memory on the heap.
		 */
		static void deallocate(main_state& state,
		                       void* ptr,
		                       std::size_t size
		                       #ifndef GAL_ALLOCATOR_NO_TRACE
		                       ,
		                       const std_source_location& location = std_source_location::current()
		                       #endif
				);

		// todo: does our allocator really support re-allocator?
		static void* memory_re_allocate(main_state& state,
		                                void* ptr,
		                                std::size_t current_size,
		                                std::size_t needed_size
		                                #ifndef GAL_ALLOCATOR_NO_TRACE
		                                ,
		                                const std_source_location& location = std_source_location::current()
		                                #endif
				);

		#ifndef GAL_ALLOCATOR_NO_TRACE
		static void print_trace_log();
		#endif
	};
}

#endif // GAL_LANG_VM_MEMORY_HPP
