#pragma once

#ifndef GAL_LANG_FOUNDATION_TYPE_INFO_HPP
#define GAL_LANG_FOUNDATION_TYPE_INFO_HPP

#include<type_traits>
#include<typeinfo>
#include<memory>

namespace gal::lang::foundation
{
	// MSVC' s stl 's type_info in the global namespace :(
	class gal_type_info
	{
	public:
		using flag_type = std::uint32_t;

		constexpr static flag_type flag_void = 1 << 0;
		constexpr static flag_type flag_arithmetic = 1 << 1;
		constexpr static flag_type flag_const = 1 << 2;
		constexpr static flag_type flag_reference = 1 << 3;
		constexpr static flag_type flag_pointer = 1 << 4;

		constexpr static flag_type flag_undefined = 1 << 5;
		constexpr static const char* undefined_type_name = "unknown-type";

	private:
		struct unknown_type { };

		using real_type_info_type = std::reference_wrapper<const std::type_info>;

		// reference_wrapper are used for CopyConstructible && CopyAssignable
		real_type_info_type ti_;
		real_type_info_type bare_ti_;
		flag_type flag_;

	public:
		constexpr gal_type_info(
				const bool is_void,
				const bool is_arithmetic,
				const bool is_const,
				const bool is_reference,
				const bool is_pointer,
				const std::type_info& ti,
				const std::type_info& bare_ti) noexcept
			: ti_{ti},
			  bare_ti_{bare_ti},
			  flag_{
				(is_void ? flag_void : 0) |
					  (is_arithmetic ? flag_arithmetic : 0) |
					  (is_const ? flag_const : 0) |
					  (is_reference ? flag_reference : 0) |
					  (is_pointer ? flag_pointer : 0)}
		{
		}

		constexpr gal_type_info() noexcept
			: ti_{typeid(unknown_type)},
			  bare_ti_{typeid(unknown_type)},
			  flag_{flag_undefined} {}

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool operator==(const gal_type_info& other) const noexcept { return ti_.get() == other.ti_.get(); }

		[[nodiscard]] constexpr bool operator==(const std::type_info& other) const noexcept { return not is_undefined() && ti_.get() == other; }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool bare_equal(const gal_type_info& other) const noexcept { return bare_ti_.get() == other.bare_ti_.get(); }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool bare_equal(const std::type_info& other) const noexcept { return not is_undefined() && bare_ti_.get() == other; }

		[[nodiscard]] bool before(const gal_type_info& other) const noexcept { return ti_.get().before(other.ti_.get()); }

		[[nodiscard]] const char* name() const noexcept
		{
			if (not is_undefined()) { return ti_.get().name(); }
			return undefined_type_name;
		}

		[[nodiscard]] const char* bare_name() const noexcept
		{
			if (not is_undefined()) { return bare_ti_.get().name(); }
			return undefined_type_name;
		}

		[[nodiscard]] constexpr bool is_void() const noexcept { return flag_ & flag_void; }
		[[nodiscard]] constexpr bool is_arithmetic() const noexcept { return flag_ & flag_arithmetic; }
		[[nodiscard]] constexpr bool is_const() const noexcept { return flag_ & flag_const; }
		[[nodiscard]] constexpr bool is_reference() const noexcept { return flag_ & flag_reference; }
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return flag_ & flag_pointer; }
		[[nodiscard]] constexpr bool is_undefined() const noexcept { return flag_ & flag_undefined; }

		[[nodiscard]] constexpr const std::type_info& bare_type_info() const noexcept { return bare_ti_.get(); }
	};

	static_assert(std::is_copy_constructible_v<gal_type_info>);
	static_assert(std::is_copy_assignable_v<gal_type_info>);
	static_assert(std::is_move_constructible_v<gal_type_info>);
	static_assert(std::is_move_assignable_v<gal_type_info>);

	namespace type_info_detail
	{
		template<typename T>
		struct bare_type
		{
			using type = std::remove_cvref_t<T>;
		};

		template<typename T>
			requires std::is_pointer_v<T>
		struct bare_type<T>
		{
			using type = typename bare_type<std::remove_pointer_t<T>>::type;
		};

		template<typename T>
		using bare_type_t = typename bare_type<T>::type;

		template<typename T>
		struct type_info_factory
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						(std::is_arithmetic_v<T> || std::is_arithmetic_v<std::remove_reference_t<T>>) && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(T),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<std::shared_ptr<T>>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(std::shared_ptr<T>),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<std::shared_ptr<T>&> : type_info_factory<std::shared_ptr<T>> { };

		template<typename T>
		struct type_info_factory<const std::shared_ptr<T>&>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(const std::shared_ptr<T>&),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<std::unique_ptr<T>>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(std::unique_ptr<T>),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<std::unique_ptr<T>&> : type_info_factory<std::unique_ptr<T>> { };

		template<typename T>
		struct type_info_factory<const std::unique_ptr<T>&>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(const std::unique_ptr<T>&),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<std::reference_wrapper<T>>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(std::reference_wrapper<T>),
						typeid(bare_type_t<T>)};
			}
		};

		template<typename T>
		struct type_info_factory<const std::reference_wrapper<T>&>
		{
			constexpr static gal_type_info make() noexcept
			{
				return {
						std::is_void_v<T>,
						std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
						std::is_const_v<T>,
						std::is_reference_v<T>,
						std::is_pointer_v<T>,
						typeid(const std::reference_wrapper<T>&),
						typeid(bare_type_t<T>)};
			}
		};
	}

	constexpr auto make_invalid_type_info() noexcept { return gal_type_info{}; }

	/**
	 * @brief Creates a type_info object representing the templated type.
	 * @tparam T Type of object to get a type_info for.
	 * @return type_info for T
	 */
	template<typename T>
	constexpr auto make_type_info() noexcept { return type_info_detail::type_info_factory<T>::make(); }

	/**
	 * @brief Creates a type_info object representing the type passed in.
	 * @tparam T Type of object to get a type_info for, derived from the passed in parameter.
	 * @return type_info for T
	 */
	template<typename T>
	constexpr auto make_type_info(const T&) noexcept { return type_info_detail::type_info_factory<T>::make(); }
}

#endif // GAL_LANG_FOUNDATION_TYPE_INFO_HPP
