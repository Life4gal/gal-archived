#pragma once

#ifndef GAL_LANG_FOUNDATION_TYPE_INFO_HPP
#define GAL_LANG_FOUNDATION_TYPE_INFO_HPP

#include<gal/defines.hpp>
#include<type_traits>
#include<typeinfo>
#include<memory>

namespace gal::lang::foundation
{
	// MSVC' s stl 's type_info in the global namespace :(
	class gal_type_info
	{
	public:
		// reference_wrapper are used for CopyConstructible && CopyAssignable
		using type_info_wrapper_type = std::reference_wrapper<const std::type_info>;
		using flag_type = std::uint32_t;

		constexpr static flag_type flag_void = 1 << 0;
		constexpr static flag_type flag_arithmetic = 1 << 1;
		constexpr static flag_type flag_const = 1 << 2;
		constexpr static flag_type flag_reference = 1 << 3;
		constexpr static flag_type flag_pointer = 1 << 4;

		constexpr static flag_type flag_undefined = 1 << 5;
		constexpr static const char* undefined_type_name = GAL_LANG_TYPE_INFO_UNKNOWN_NAME;

		struct info_builder
		{
			bool is_void;
			bool is_arithmetic;
			bool is_const;
			bool is_reference;
			bool is_pointer;

			constexpr flag_type operator()() const noexcept
			{
				return (is_void ? flag_void : 0) |
				       (is_arithmetic ? flag_arithmetic : 0) |
				       (is_const ? flag_const : 0) |
				       (is_reference ? flag_reference : 0) |
				       (is_pointer ? flag_pointer : 0);
			}
		};

	private:
		struct unknown_type { };

		struct internal_type {};

		type_info_wrapper_type type_;
		type_info_wrapper_type bare_type_;
		flag_type flag_;

		[[nodiscard]] static auto get_typename(const std::type_info& type)
		{
			#ifdef COMPILER_MSVC
			return type.raw_name();
			#else
			return type.name();
			#endif
		}


	public:
		GAL_LANG_TYPE_INFO_DEBUG_DO(
				const char* type_name;
				const char* bare_type_name;)

		GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) gal_type_info(
				const info_builder builder,
				const std::type_info& type,
				const std::type_info& bare_type) noexcept
			: type_{type},
			  bare_type_{bare_type},
			  flag_{builder()}
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO(

					type_name = get_typename(type);
					bare_type_name = get_typename(bare_type);
					)
		}

		GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,)gal_type_info() noexcept
			: type_{typeid(unknown_type)},
			  bare_type_{typeid(unknown_type)},
			  flag_{flag_undefined}
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO(
					type_name = get_typename(type_);
					bare_type_name = get_typename(bare_type_);
					)
		}

		// for register internal unique type_info
		GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) explicit gal_type_info(const flag_type flag) noexcept
			: type_{typeid(internal_type)},
			  bare_type_{typeid(internal_type)},
			  flag_{flag_undefined | flag}
		{
			GAL_LANG_TYPE_INFO_DEBUG_DO(
					type_name = get_typename(type_);
					bare_type_name = get_typename(bare_type_);)
		}

		[[nodiscard]] constexpr bool is_void() const noexcept { return flag_ & flag_void; }
		[[nodiscard]] constexpr bool is_arithmetic() const noexcept { return flag_ & flag_arithmetic; }
		[[nodiscard]] constexpr bool is_const() const noexcept { return flag_ & flag_const; }
		[[nodiscard]] constexpr bool is_reference() const noexcept { return flag_ & flag_reference; }
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return flag_ & flag_pointer; }
		[[nodiscard]] constexpr bool is_undefined() const noexcept { return flag_ & flag_undefined; }
		[[nodiscard]] constexpr bool is_internal() const noexcept { return is_undefined() && flag_ != flag_undefined; }
		[[nodiscard]] constexpr bool is_internal(const flag_type flag) const noexcept { return flag == (flag_ & (~flag_undefined)); }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool operator==(const gal_type_info& other) const noexcept { return type_.get() == other.type_.get(); }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool operator==(const std::type_info& other) const noexcept { return not is_undefined() && type_.get() == other; }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool bare_equal(const gal_type_info& other) const noexcept { return bare_type_.get() == other.bare_type_.get(); }

		// `constexpr` requires c++23
		[[nodiscard]] /* constexpr */ bool bare_equal(const std::type_info& other) const noexcept { return not is_undefined() && bare_type_.get() == other; }

		[[nodiscard]] bool before(const gal_type_info& other) const noexcept { return type_.get().before(other.type_.get()); }

		[[nodiscard]] const char* name() const noexcept
		{
			if (not is_undefined()) { return type_.get().name(); }
			return undefined_type_name;
		}

		[[nodiscard]] const char* bare_name() const noexcept
		{
			if (not is_undefined()) { return bare_type_.get().name(); }
			return undefined_type_name;
		}

		[[nodiscard]] constexpr type_info_wrapper_type bare_type_info() const noexcept { return bare_type_; }
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = (std::is_arithmetic_v<T> || std::is_arithmetic_v<std::remove_reference_t<T>>) && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<std::remove_pointer_t<std::remove_reference_t<T>>>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
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
						gal_type_info::info_builder{
								.is_void = std::is_void_v<T>,
								.is_arithmetic = std::is_arithmetic_v<T> && (not std::is_same_v<std::remove_cvref_t<T>, bool>),
								.is_const = std::is_const_v<T>,
								.is_reference = std::is_reference_v<T>,
								.is_pointer = std::is_pointer_v<T>},
						typeid(const std::reference_wrapper<T>&),
						typeid(bare_type_t<T>)};
			}
		};
	}// namespace type_info_detail

	/**
	 * @brief Creates a type_info object representing the invalid type.
	 */
	[[nodiscard]] GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr, inline) gal_type_info make_invalid_type_type() noexcept { return gal_type_info{}; }

	/**
	 * @brief Creates a type_info object representing the internal type.
	 */
	[[nodiscard]] GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr, inline) gal_type_info make_internal_type_type(const gal_type_info::flag_type flag) noexcept { return gal_type_info{flag}; }

	/**
	 * @brief Creates a type_info object representing the templated type.
	 * @tparam T Type of object to get a type_info for.
	 * @return type_info for T
	 */
	template<typename T>
	[[nodiscard]] GAL_LANG_TYPE_INFO_DEBUG_DO_OR(constexpr,) gal_type_info make_type_info() noexcept { return type_info_detail::type_info_factory<T>::make(); }
}

#endif// GAL_LANG_FOUNDATION_TYPE_INFO_HPP
