#pragma once

#ifndef GAL_LANG_AST_ALLOCATOR_HPP
#define GAL_LANG_AST_ALLOCATOR_HPP

#include <forward_list>
#include <array>

#ifndef GAL_ALLOCATOR_NO_TRACE
#include <iostream>// for std::clog
#include <utils/format.hpp>
#include <utils/source_location.hpp>
#endif

namespace gal::ast
{
	class ast_allocator
	{
	public:
		using size_type = std::size_t;

		/**
		 * @brief Always assume that we will not allocate more than this amount of memory at one time, if it exceeds, then the behavior is undefined.
		 */
		constexpr static size_type max_bytes_per_page = 1024;
		constexpr static size_type max_bits_per_page = max_bytes_per_page * 8;

	private:
		using page_type = std::forward_list<std::array<std::byte, max_bytes_per_page>>;

		page_type root_{};
		size_type offset_{0};

	public:
		ast_allocator() { root_.emplace_front(); }
		ast_allocator(ast_allocator&&) = default;
		ast_allocator& operator=(ast_allocator&&) = default;

		ast_allocator(const ast_allocator&) = delete;
		ast_allocator& operator=(const ast_allocator&) = delete;

		~ast_allocator() = default;

		void* allocate(
				size_type n
				#ifndef GAL_ALLOCATOR_NO_TRACE
				,
				const std_source_location& location = std_source_location::current()
				#endif
				)
		{
			constexpr auto align = alignof(std::max_align_t) - 1;

			const auto current_page_begin = reinterpret_cast<std::uintptr_t>(root_.front().data());

			auto result = (current_page_begin + offset_ + align) & ~align;
			if (result + n <= current_page_begin + max_bits_per_page)
			{
				// current page is enough
				offset_ = result + n - current_page_begin;
			}
			else
			{
				// we need a new page
				root_.emplace_front();
				offset_ = n;
				result = reinterpret_cast<std::uintptr_t>(root_.front().data());
			}

			#ifndef GAL_ALLOCATOR_NO_TRACE
			std::clog << std_format::format(
					"allocate {} byte(s) at {}. allocate at: [file:{}][line:{}, column: {}][function:{}]\n",
					n,
					result,
					location.file_name(),
					location.line(),
					location.column(),
					location.function_name());
			#endif
			return reinterpret_cast<void*>(result);
		}

		template<typename T, typename... Args>
		// todo: add qualification
		// requires std::is_trivially_constructible_v<T, Args...> && std::is_trivially_destructible_v<T>
		T* new_object(Args&&... args)
		{
			// We have no way (and will not) to destruct the object allocated from this allocator, so the object must be trivial.
			auto* obj = static_cast<T*>(allocate(sizeof(T)));
			std::construct_at(obj, std::forward<Args>(args)...);
			return obj;
		}
	};
}

#endif // GAL_LANG_AST_ALLOCATOR_HPP
