#pragma once

#ifndef GAL_UTILS_UTILITY_BASE_HPP
#define GAL_UTILS_UTILITY_BASE_HPP

namespace gal::utils
{
	struct noncopyable_base
	{
		constexpr noncopyable_base() noexcept = default;
		constexpr noncopyable_base(const noncopyable_base&) = delete;
		constexpr noncopyable_base& operator=(const noncopyable_base&) = delete;
		constexpr noncopyable_base(noncopyable_base&&) noexcept = default;
		constexpr noncopyable_base& operator=(noncopyable_base&&) noexcept = default;
		constexpr ~noncopyable_base() noexcept = default;
	};

	struct nonmovable_base
	{
		constexpr nonmovable_base() noexcept = default;
		constexpr nonmovable_base(const nonmovable_base&) = default;
		constexpr nonmovable_base& operator=(const nonmovable_base&) = default;
		constexpr nonmovable_base(nonmovable_base&&) noexcept = delete;
		constexpr nonmovable_base& operator=(nonmovable_base&&) noexcept = delete;
		constexpr ~nonmovable_base() noexcept = default;
	};

	struct non_copy_move_base
	{
		constexpr non_copy_move_base() noexcept = default;
		constexpr non_copy_move_base(const non_copy_move_base&) = delete;
		constexpr non_copy_move_base& operator=(const non_copy_move_base&) = delete;
		constexpr non_copy_move_base(non_copy_move_base&&) noexcept = delete;
		constexpr non_copy_move_base& operator=(non_copy_move_base&&) noexcept = delete;
		constexpr ~non_copy_move_base() noexcept = default;
	};

	template<typename Derived, typename DataType>
	struct scoped_base : non_copy_move_base
	{
	private:
		using derived_type = Derived;
		using data_type = DataType;

		data_type data_;

	public:
		[[nodiscard]] constexpr data_type& data() noexcept { return data_; }

		[[nodiscard]] constexpr const data_type& data() const noexcept { return data_; }

		constexpr scoped_base()
		noexcept(std::is_nothrow_default_constructible_v<data_type> && noexcept(std::declval<derived_type&>().do_construct()))
			requires requires(derived_type& derived)
			         {
				         derived.do_construct();
			         } &&
			         std::is_default_constructible_v<data_type> { static_cast<derived_type&>(*this).do_construct(); }

		template<typename... Args>
			requires requires(derived_type& derived)
			{
				derived.do_construct();
			} && std::is_constructible_v<data_type, Args...>
		explicit constexpr scoped_base(Args&&... args) noexcept(std::is_nothrow_constructible_v<data_type, Args...> && noexcept(std::declval<derived_type&>().do_construct()))
			: data_{std::forward<Args>(args)...} { static_cast<derived_type&>(*this).do_construct(); }

		// constexpr ~scoped_base() noexcept = delete;

		constexpr ~scoped_base() noexcept
		// requires requires(derived_type& derived)
		// {
		// 	derived.do_destruct();
		// }
		{
			static_cast<derived_type&>(*this).do_destruct();
		}
	};
}

#endif // GAL_UTILS_UTILITY_BASE_HPP
