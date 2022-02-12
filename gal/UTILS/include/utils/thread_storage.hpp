#pragma once

#ifndef GAL_UTILS_THREAD_STORAGE_HPP
#define GAL_UTILS_THREAD_STORAGE_HPP

/**
 * @file thread_storage.hpp
 *
 * @details If the compiler definition GAL_UTILS_NO_THREAD_STORAGE is defined
 * then thread safety is disabled. This has the result that some code is faster,
 * because mutex locks are not required. It also has the side effect that the object
 * may not be accessed from more than one thread simultaneously.
 */

#include <unordered_map>

#ifndef GAL_UTILS_NO_THREAD_STORAGE
#include <mutex>
#include <shared_mutex>
#endif

namespace gal::utils
{
	namespace detail
	{
		template<typename T>
		class unique_lock
		{
		public:
			constexpr explicit unique_lock(T&) noexcept {}
			// ReSharper disable once CppMemberFunctionMayBeConst
			constexpr void lock() noexcept { (void)this; }
			// ReSharper disable once CppMemberFunctionMayBeConst
			constexpr void unlock() noexcept { (void)this; }
		};

		template<typename T>
		class shared_lock
		{
		public:
			constexpr explicit shared_lock(T&) noexcept {}
			// ReSharper disable once CppMemberFunctionMayBeConst
			constexpr void lock() noexcept { (void)this; }
			// ReSharper disable once CppMemberFunctionMayBeConst
			constexpr void unlock() noexcept { (void)this; }
		};

		template<typename... T>
		class scoped_lock
		{
			constexpr explicit scoped_lock(T&...) noexcept {}
		};

		template<typename T>
		class scoped_lock<T>
		{
			constexpr explicit scoped_lock(T&) noexcept {}
		};

		template<>
		class scoped_lock<>
		{
			constexpr explicit scoped_lock() noexcept = default;
		};

		class shared_mutex { };

		class recursive_mutex { };
	}

	namespace threading
	{
		#ifndef GAL_UTILS_NO_THREAD_STORAGE
		template<typename T>
		using unique_lock = std::unique_lock<T>;

		template<typename T>
		using shared_lock = std::shared_lock<T>;

		template<typename... T>
		using scoped_lock = std::scoped_lock<T...>;

		using std::mutex;

		using std::shared_mutex;

		using std::recursive_mutex;
		#else
		template<typename T>
		using unique_lock = detail::unique_lock<T>;

		template<typename T>
		using shared_lock = detail::shared_lock<T>;

		template<typename... T>
		using scoped_lock = detail::scoped_lock<T...>;

		using shared_mutex = detail::shared_mutex;

		using recursive_mutex = detail::recursive_mutex;
		#endif
	}

	template<typename T>
	class thread_storage
	{
	public:
		using storage_type = std::unordered_map<const void*, T>;

		#ifndef GAL_UTILS_NO_THREAD_STORAGE
	private:
		static storage_type& data() noexcept
		{
			static thread_local storage_type d{};
			return d;
		}

	public:
		thread_storage() = default;

		thread_storage(const thread_storage&) = delete;
		thread_storage& operator=(const thread_storage&) = delete;
		thread_storage(thread_storage&&) = delete;
		thread_storage& operator=(thread_storage&&) = delete;

		~thread_storage() noexcept { data().erase(this); }

		T& operator*() noexcept { return data()[this]; }

		const T& operator*() const noexcept { return this->operator*(); }

		T* operator->() noexcept { return &data()[this]; }

		const T* operator->() const noexcept { return this->operator->(); }

		#else
	private:
		mutable T dummy_;

	public:
		constexpr thread_storage() = default;

		constexpr T& operator*() const noexcept { return dummy_; }

		constexpr T* operator->() const noexcept { return &this->operator*(); }

		#endif
	};
}

#endif // GAL_UTILS_THREAD_STORAGE_HPP
