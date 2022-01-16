#include <vm/memory.hpp>
#include <config/config.hpp>
#include <array>
#include <algorithm>
#include <utils/assert.hpp>
#include <vm/allocator.hpp>
#include <vm/exception.hpp>
#include <vm/state.hpp>

namespace
{
	using namespace gal;
	using namespace vm;

	constexpr std::size_t max_small_size = 512;
	// slightly under 16KB since that results in less fragmentation due to heap metadata
	constexpr std::size_t per_page_size = 16 * 1024 - 24;
	// used to store a pointer to the page where the block is located
	using block_header_type = std::uintptr_t;
	constexpr std::size_t block_header = sizeof(block_header_type);

	struct size_class_config
	{
		constexpr static std::size_t inappropriate_class_size = static_cast<std::size_t>(-1);

		std::array<int, size_classes> size_of_class;
		std::array<std::int8_t, max_small_size + 1> class_for_size;

		int class_count;

		constexpr size_class_config()
			: size_of_class{},
			  class_for_size{},
			  class_count{0}
		{
			std::ranges::fill(size_of_class, 0);
			std::ranges::fill(class_for_size, static_cast<std::int8_t>(-1));

			// we use a progressive size class scheme:
			// - all size classes are aligned by 8b to satisfy pointer alignment requirements
			// - we first allocate sizes classes in multiples of 8
			// - after the first cutoff we allocate size classes in multiples of 16
			// - after the second cutoff we allocate size classes in multiples of 32
			// this balances internal fragmentation vs external fragmentation
			for (int size = 8; size < 64; size += 8) { size_of_class[class_count++] = size; }

			for (int size = 64; size < 256; size += 16) { size_of_class[class_count++] = size; }

			for (int size = 256; size <= 512; size += 32) { size_of_class[class_count++] = size; }

			// gal_assert(std::cmp_less_equal(class_count, size_classes));

			// fill the lookup table for all classes
			for (int c = 0; c < class_count; ++c) { class_for_size[size_of_class[c]] = static_cast<std::int8_t>(c); }

			// fill the gaps in lookup table
			for (auto size = max_small_size - 1; size != 0; --size) { if (class_for_size[size] < 0) { class_for_size[size] = class_for_size[size + 1]; } }
		}

		/**
		 * @brief Size class for a block of size `size`
		 */
		[[nodiscard]] constexpr std::size_t get_class_size(const std::size_t size) const noexcept { return (size - 1) < max_small_size ? class_for_size[size] : inappropriate_class_size; }
	};

	constexpr size_class_config config{};
}

namespace gal::vm
{
	struct memory_page
	{
		constexpr static std::uint32_t block_run_out = static_cast<std::uint32_t>(-1);

		memory_page* prev{nullptr};
		memory_page* next{nullptr};

		std::uint32_t per_block_size{0};

		std::uint32_t used_block{0};
		std::uint32_t next_free_block{0};

		void* free_list{nullptr};

		union // alignas(std::max_align_t)
		{
			char data[1];

			std::max_align_t dummy;
		};

	private:
		[[nodiscard]] constexpr void* get_next_free_block() noexcept
		{
			auto* ret = &data + next_free_block;
			if (next_free_block == 0) { next_free_block = block_run_out; }
			else { next_free_block -= per_block_size; }
			++used_block;
			return ret;
		}

		[[nodiscard]] void* get_free_list() noexcept
		{
			auto* ret = free_list;
			reinterpret_cast<block_header_type&>(free_list) = reinterpret_cast<block_header_type>(ret);// NOLINT(clang-diagnostic-undefined-reinterpret-cast)
			++used_block;
			return ret;
		}

	public:
		[[nodiscard]] constexpr bool has_free_block() const noexcept { return next_free_block != block_run_out; }

		[[nodiscard]] constexpr bool empty() const noexcept { return used_block == 0; }

		[[nodiscard]] void* get_next_block() noexcept
		{
			auto* ret = has_free_block() ? get_next_free_block() : get_free_list();
			// the first word in a block point back to the page
			reinterpret_cast<block_header_type&>(ret) = reinterpret_cast<block_header_type>(this);// NOLINT(clang-diagnostic-undefined-reinterpret-cast)
			// the user data is right after the metadata
			return static_cast<char*>(ret) + block_header;
		}

		void set_free_list(void* block) noexcept
		{
			// add the block to the free list inside the page
			reinterpret_cast<block_header_type&>(block) = reinterpret_cast<block_header_type>(free_list);// NOLINT(clang-diagnostic-undefined-reinterpret-cast)
			free_list = block;
			--used_block;
		}

		[[nodiscard]] constexpr static void* get_block_real_address(void* block) noexcept
		{
			// // the user data is right after the metadata
			return static_cast<char*>(block) - block_header;
		}

		[[nodiscard]] static memory_page* get_block_located_page(void* block) noexcept { return reinterpret_cast<memory_page*>(reinterpret_cast<block_header_type>(block)); }// NOLINT(performance-no-int-to-ptr)

		static memory_page* create_page(main_state& state, const std::size_t size_class)
		{
			gal_assert(size_class < size_classes);

			auto* page = static_cast<memory_page*>(std::malloc(per_page_size));
			if (not page) { throw vm_exception{state, thread_status::error_memory}; }

			const auto block_size = static_cast<std::uint32_t>(config.size_of_class[size_class] + block_header);
			const auto block_count = static_cast<std::uint32_t>((per_page_size - offsetof(memory_page, data)) / block_size);

			page->per_block_size = block_size;

			// note: we start with the last block in the page and move downward
			// either order would work, but that way we don't need to store the block count in the page
			// additionally, GC stores objects in singly linked lists, and this way the GC lists end up in increasing pointer order
			page->free_list = nullptr;
			page->next_free_block = (block_count - 1) * block_size;

			// prepend a page to page free_list (which is empty because we only ever allocate a new page when it is!)
			gal_assert(not state.gc_.free_pages[size_class]);
			state.gc_.free_pages[size_class] = page;

			return page;
		}

		static void destroy_page(main_state& state, memory_page* page, const std::size_t size_class)
		{
			gal_assert(size_class < size_classes);

			// remove page from free_list
			if (page->next) { page->next->prev = page->prev; }

			if (page->prev) { page->prev->next = page->next; }
			else if (state.gc_.free_pages[size_class] == page) { state.gc_.free_pages[size_class] = page->next; }

			delete page;
		}

		static void* create_block(main_state& state, const std::size_t size_class)
		{
			gal_assert(size_class < size_classes);

			auto* page = state.gc_.free_pages[size_class];

			// slow path: no page in the free_list, allocate a new one
			if (not page) { page = create_page(state, size_class); }

			gal_assert(not page->prev);
			gal_assert(page->free_list || page->has_free_block());
			gal_assert(page->per_block_size == config.size_of_class[size_class] + block_header);

			auto* block = page->get_next_block();

			// if we allocate the last block out of a page, we need to remove it from free list
			if (not page->free_list && not page->has_free_block())
			{
				state.gc_.free_pages[size_class] = page->next;
				if (page->next) { page->next->prev = nullptr; }
				page->next = nullptr;
			}

			return block;
		}

		static void destroy_block(main_state& state, void* block, const std::size_t size_class)
		{
			gal_assert(size_class < size_classes);

			gal_assert(block);
			block = get_block_real_address(block);

			auto* page = get_block_located_page(block);
			gal_assert(page && not page->empty());
			gal_assert(page->per_block_size == config.size_of_class[size_class] + block_header);

			// if the page was not in the page free list, it should be now since it got a block!
			if (not page->free_list && not page->has_free_block())
			{
				gal_assert(not page->prev);
				gal_assert(not page->next);

				page->next = state.gc_.free_pages[size_class];
				if (page->next) { page->next->prev = page; }
				state.gc_.free_pages[size_class] = page;
			}

			page->set_free_list(block);

			// if it is the last block in the page, we do not need the page
			if (page->empty()) { destroy_page(state, page, size_class); }
		}
	};

	void* raw_memory::allocate(main_state& state, const std::size_t size)
	{
		const auto n_class = config.get_class_size(size);

		auto* block = n_class != size_class_config::inappropriate_class_size
			              ? memory_page::create_block(state, n_class)
			              : std::malloc(size);

		if (not block && n_class > 0) { throw vm_exception{state, thread_status::error_memory}; }

		state.gc_.total_bytes += size;

		return block;
	}

	void raw_memory::deallocate(main_state& state, void* ptr, const std::size_t size)
	{
		gal_assert((size == 0) == (ptr == nullptr));

		if (const auto n_class = config.get_class_size(size);
			n_class != size_class_config::inappropriate_class_size) { memory_page::destroy_block(state, ptr, n_class); }
		else { std::free(ptr); }

		state.gc_.total_bytes -= size;
	}

	void* raw_memory::memory_re_allocate(main_state& state, void* ptr, std::size_t current_size, std::size_t needed_size)
	{
		gal_assert((current_size == 0) == (ptr == nullptr));

		const auto n_class = config.get_class_size(needed_size);
		const auto o_class = config.get_class_size(current_size);

		void* result;

		// if either block needs to be allocated using a block allocator, we can not use re-allocate directly
		if (n_class != size_class_config::inappropriate_class_size || o_class != size_class_config::inappropriate_class_size)
		{
			result = n_class != size_class_config::inappropriate_class_size
				         ? memory_page::create_block(state, n_class)
				         : std::malloc(needed_size);

			if (not result && needed_size != 0) { throw vm_exception{state, thread_status::error_memory}; }

			if (current_size != 0 && needed_size != 0) { std::memcpy(result, ptr, std::ranges::min(current_size, needed_size)); }

			if (o_class != size_class_config::inappropriate_class_size) { memory_page::destroy_block(state, ptr, current_size); }
			else { std::free(ptr); }
		}
		else
		{
			result = std::realloc(ptr, needed_size);
			if (not result && needed_size != 0) { throw vm_exception{state, thread_status::error_memory}; }
		}

		gal_assert((needed_size == 0) == (result == nullptr));

		state.gc_.total_bytes += needed_size - current_size;

		return result;
	}
}
