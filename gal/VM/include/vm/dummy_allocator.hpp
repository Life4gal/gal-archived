#pragma once

#ifndef GAL_LANG_VM_ALLOCATOR_HPP
#define GAL_LANG_VM_ALLOCATOR_HPP

#ifndef GAL_ALLOCATOR_NO_TRACE
#include <iostream>// for std::clog
#include <utils/format.hpp>
#include <utils/source_location.hpp>
#endif

#include <vm/memory.hpp>
#include <config/config.hpp>

namespace gal::vm
{
	template<typename T>
	struct vm_allocator
	{
		using allocator_type = std::allocator<T>;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using value_type = typename allocator_traits::value_type;
		using pointer = typename allocator_traits::pointer;
		using size_type = typename allocator_traits::size_type;

		[[no_unique_address]] allocator_type allocator;

		[[nodiscard]] constexpr auto allocate(
				size_type n
				#ifndef GAL_ALLOCATOR_NO_TRACE
				,
				const std_source_location& location = std_source_location::current()
				#endif
				)
		{
			#ifndef GAL_ALLOCATOR_NO_TRACE
			auto* ret = allocator_traits::allocate(allocator, n);
			std::clog << std_format::format(
					"allocate {} object(s) at {} ({} byte(s) per object), total used {} bytes. allocate at: [file:{}][line:{}, column: {}][function:{}]\n",
					n,
					static_cast<void*>(ret),
					sizeof(value_type),
					sizeof(value_type) * n,
					location.file_name(),
					location.line(),
					location.column(),
					location.function_name());
			return ret;
			#else
			return allocator_traits::allocate(allocator, n);
			#endif
		}

		constexpr void deallocate(
				T* p,
				size_type n
				#ifndef GAL_ALLOCATOR_NO_TRACE
				,
				const std_source_location& location = std_source_location::current()
				#endif
				)
		{
			#ifndef GAL_ALLOCATOR_NO_TRACE
			std::clog << std_format::format(
					"deallocate {} object(s) at {} ({} byte(s) per object), total used {} bytes. allocate at: [file:{}][line:{}, column: {}][function:{}]\n",
					n,
					static_cast<void*>(p),
					sizeof(value_type),
					sizeof(value_type) * n,
					location.file_name(),
					location.line(),
					location.column(),
					location.function_name());
			#endif
			allocator_traits::deallocate(allocator, p, n);
		}

		template<typename U, typename... Args>
		constexpr void construct(U* p, Args&&... args) { allocator_traits::construct(allocator, p, std::forward<Args>(args)...); }

		template<typename U>
		constexpr void destroy(
				U* p
				#ifndef GAL_ALLOCATOR_NO_TRACE
				,
				const std_source_location& location = std_source_location::current()
				#endif
				)
		{
			#ifndef GAL_ALLOCATOR_NO_TRACE
			std::clog << std_format::format(
					"destroy an object at {}. construct at: [file:{}][line:{}, column: {}][function:{}]\n",
					static_cast<void*>(p),
					location.file_name(),
					location.line(),
					location.column(),
					location.function_name());
			#endif
			allocator_traits::destroy(allocator, p);
		}

		friend constexpr bool operator==(const vm_allocator& lhs, const vm_allocator& rhs) noexcept { return lhs.allocator == rhs.allocator; }
	};

	template<typename T1, typename T2>
	constexpr bool operator==(const vm_allocator<T1>& lhs, const vm_allocator<T2>& rhs) { return lhs.allocator == rhs.allocator; };
}

#endif // GAL_LANG_VM_ALLOCATOR_HPP
