#pragma once

#ifndef GAL_UTILS_SMALL_BLOCK_ALLOCATOR_HPP
#define GAL_UTILS_SMALL_BLOCK_ALLOCATOR_HPP

#ifndef GAL_ALLOCATOR_NO_TRACE
#include <iostream>// for std::clog
#include <utils/format.hpp>
#include <utils/source_location.hpp>
#define GAL_ALLOCATOR_TRACE_DO(...) __VA_ARGS__
#else
		#define GAL_ALLOCATOR_TRACE_DO(...)
#endif

#include <utils/assert.hpp>

namespace gal::utils
{
	namespace detail
	{
		template<std::size_t Total, std::size_t Alignment = alignof(std::max_align_t)>
		class arena
		{
		public:
			constexpr static std::size_t total_size = Total;
			constexpr static std::size_t alignment = Alignment;

			static_assert(total_size % alignment == 0, "total_size needs to be a multiple of alignment Align");

		private:
			alignas(alignment) char buffer_[total_size];
			char* current_;

			constexpr static std::size_t align_up(const std::size_t size) noexcept { return (size + (alignment - 1)) & ~(alignment - 1); }

			constexpr bool in_bound(const char* ptr) const noexcept { return buffer_ <= ptr && ptr <= buffer_ + total_size; }

		public:
			arena()
				: buffer_{},
				  current_{buffer_} {}

			arena(const arena&) = delete;
			arena& operator=(const arena&) = delete;
			arena(arena&&) = default;
			arena& operator=(arena&&) = default;
			~arena() = default;

			[[nodiscard]] constexpr const char* begin() const noexcept { return buffer_; }

			[[nodiscard]] constexpr const char* end() const noexcept { return buffer_ + total_size; }

			[[nodiscard]] constexpr std::size_t used() const noexcept { return {current_ - buffer_}; }

			[[nodiscard]] constexpr std::size_t remainder() const noexcept { return total_size - used(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return current_ == buffer_; }

			constexpr void clear() const noexcept { current_ = buffer_; }

			template<std::size_t RequiredAlignment = alignment>
			char* allocate(const std::size_t size)
			{
				static_assert(RequiredAlignment <= alignment, "alignment is too small for this arena");
				gal_assert(in_bound(current_), "allocate pointer has outlived arena");
				if (const auto aligned_size = align_up(size);
					remainder() >= aligned_size)
				{
					char* ret = current_;
					current_ += aligned_size;
					return ret;
				}

				// not enough memory
				static_assert(alignment <= alignof(std::max_align_t),
					"you've chosen an alignment that is larger than alignof(std::max_align_t),"
					"and cannot be guaranteed by normal operator new");

				// return static_cast<char*>(::operator new(size, std::align_val_t{RequiredAlignment}));
				return static_cast<char*>(::operator new(size));
			}

			void deallocate(char* ptr, const std::size_t size) noexcept
			{
				gal_assert(in_bound(current_), "allocate pointer has outlived arena");
				if (in_bound(ptr))
				{
					// Only in the last block can be recycled
					if (const auto aligned_size = align_up(size);
						ptr + aligned_size == current_) { current_ = ptr; }
				}
				else
				{
					// Use global new to acquire memory
					// ::operator delete(ptr, size);
					::operator delete(ptr);
				}
			}
		};
	}

	template<typename T, std::size_t Total, std::size_t Alignment = alignof(std::max_align_t)>
	class small_block_allocator
	{
		template<typename U, std::size_t S, std::size_t A>
		friend class small_block_allocator<U, S, A>;

	public:
		using arena_type = detail::arena<Total, Alignment>;

		constexpr static auto total_size = arena_type::total_size;
		constexpr static auto alignment = arena_type::alignment;

		using allocator_type = std::allocator<T>;
		using allocator_traits = std::allocator_traits<allocator_type>;

		using value_type = typename allocator_traits::value_type;
		using pointer = typename allocator_traits::pointer;
		using size_type = typename allocator_traits::size_type;

	private:
		[[no_unique_address]] allocator_type allocator_;
		arena_type arena_;

	public:
		[[nodiscard]] constexpr auto allocate(
				size_type n
				GAL_ALLOCATOR_TRACE_DO(
						,
						const std_source_location& location = std_source_location::current()
						)
				)
		{
			auto* ret = arena_.template allocate<alignof(T)>(n * sizeof(T));

			GAL_ALLOCATOR_TRACE_DO(
					std::clog << std_format::format(
						"Allocate {} object(s) at {} ({} byte(s) per object), total used {} bytes. Allocate at: [file:{}][line:{}, column: {}][function:{}]\n",
						n,
						static_cast<void*>(ret),
						sizeof(value_type),
						sizeof(value_type) * n,
						location.file_name(),
						location.line(),
						location.column(),
						location.function_name());
					)

			return ret;
		}

		constexpr void deallocate(
				T* p,
				size_type n
				GAL_ALLOCATOR_TRACE_DO(
						,
						const std_source_location& location = std_source_location::current())
				)
		{
			GAL_ALLOCATOR_TRACE_DO(
					std::clog << std_format::format(
						"Deallocate {} object(s) at {} ({} byte(s) per object), total used {} bytes. Deallocate at: [file:{}][line:{}, column: {}][function:{}]\n",
						n,
						static_cast<void*>(p),
						sizeof(value_type),
						sizeof(value_type) * n,
						location.file_name(),
						location.line(),
						location.column(),
						location.function_name());
					)
			arena_.deallocate(p, n * sizeof(T));
		}

		template<typename U, typename... Args>
		constexpr void construct(U* p, Args&&... args) { allocator_traits::construct(allocator_, p, std::forward<Args>(args)...); }

		template<typename U>
		constexpr void destroy(U* p) { allocator_traits::destroy(allocator_, p); }

		template<typename U1, std::size_t T1, std::size_t A1, typename U2, std::size_t T2, std::size_t A2>
		friend constexpr bool operator==(const small_block_allocator& lhs, const small_block_allocator& rhs) noexcept { return T1 == T2 && A1 == A2 && lhs.arena_ == rhs.arena_; }
	};
}

template<typename T, std::size_t Total, std::size_t Alignment>
struct std::allocator_traits<::gal::utils::small_block_allocator<T, Total, Alignment>>
{
	using allocator_type = ::gal::utils::small_block_allocator<T, Total, Alignment>;
	using internal_allocator_traits = typename allocator_type::allocator_traits;

	using value_type = typename internal_allocator_traits::value_type;
	using pointer = typename internal_allocator_traits::pointer;
	using const_pointer = typename internal_allocator_traits::const_pointer;
	using void_pointer = typename internal_allocator_traits::void_pointer;
	using const_void_pointer = typename internal_allocator_traits::const_void_pointer;
	using difference_type = typename internal_allocator_traits::difference_type;
	using size_type = typename internal_allocator_traits::size_type;

	using propagate_on_container_copy_assignment = std::false_type;
	using propagate_on_container_move_assignment = typename internal_allocator_traits::propagate_on_container_move_assignment;
	using propagate_on_container_swap = typename internal_allocator_traits::propagate_on_container_swap;
	using is_always_equal = std::false_type;

	template<typename T1, std::size_t Total1>
	using rebind_alloc = ::gal::utils::small_block_allocator<T1, Total1, Alignment>;
	template<typename T1, std::size_t Total1>
	using rebind_traits = allocator_traits<rebind_alloc<T, Total1>>;

	[[nodiscard]] constexpr static pointer allocate(
			allocator_type& a,
			size_type n
			GAL_ALLOCATOR_TRACE_DO(
					,
					const std_source_location& location = std_source_location::current())
			)
	{
		return a.allocate(n
		                  GAL_ALLOCATOR_TRACE_DO(, location)
				);
	}

	constexpr static void deallocate(
			allocator_type& a,
			pointer p,
			size_type n
			GAL_ALLOCATOR_TRACE_DO(
					,
					const std_source_location& location = std_source_location::current())
			)
	{
		return a.deallocate(
				p,
				n
				GAL_ALLOCATOR_TRACE_DO(, location)
				);
	}

	template<typename U, typename... Args>
	constexpr static void construct(allocator_type& a, U* p, Args&&... args) { a.construct(p, std::forward<Args>(args)...); }

	template<typename U>
	constexpr static void destroy(allocator_type& a, U* p) { a.destroy(p); }

	constexpr static size_type max_size(const allocator_type& a) noexcept { return internal_allocator_traits::max_size(a.allocator); }

	constexpr static allocator_type select_on_container_copy_construction(const allocator_type& a) { return a; }
};

#endif // GAL_UTILS_SMALL_BLOCK_ALLOCATOR_HPP
