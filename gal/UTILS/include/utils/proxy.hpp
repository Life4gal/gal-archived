#pragma once

#ifndef GAL_UTILS_PROXY_HPP
#define GAL_UTILS_PROXY_HPP

#include <tuple>
#include <type_traits>

namespace gal::utils
{
	template<typename Signature>
	struct as_interface;

	template<typename Return, typename... Args>
	struct as_interface<Return(Args ...)>
	{
		using return_type = Return;

		using parameters_type = std::tuple<Args...>;
		constexpr static std::size_t parameters_size = std::tuple_size_v<parameters_type>;
		template<std::size_t Index>
		using parameter_type = std::tuple_element_t<Index, parameters_type>;
	};

	namespace proxy_detail
	{
		template<typename Signature>
		struct is_interface : std::false_type {};

		template<typename Return, typename... Args>
		struct is_interface<as_interface<Return(Args ...)>> : std::true_type {};

		template<typename T, typename Return, typename ArgTuple>
		struct is_derived_from_interface;

		template<typename T, typename Return, typename... Args>
		struct is_derived_from_interface<T, Return, std::tuple<Args...>> : std::is_base_of<as_interface<Return(Args ...)>, T> {};

		template<typename T, typename Return, typename ArgTuple>
		constexpr static bool is_derived_from_interface_v = is_derived_from_interface<T, Return, ArgTuple>::value;
		template<typename T, typename Return, typename ArgTuple>
		concept derived_from_interface_t = is_derived_from_interface_v<T, Return, ArgTuple>;

		template<typename T>
			requires derived_from_interface_t<T, typename T::return_type, typename T::parameters_type>
		struct is_interface<T> : std::true_type {};

		template<typename Signature>
		constexpr static bool is_interface_v = is_interface<Signature>::value;
		template<typename Signature>
		concept interface_t = is_interface_v<Signature>;

		enum class constraint_level
		{
			none,
			nontrivial,
			nothrow,
			trivial
		};
	}

	template<proxy_detail::interface_t... Interfaces>
	struct as_facade
	{
		using interfaces_type = std::tuple<Interfaces...>;
		constexpr static std::size_t interface_size = std::tuple_size_v<interfaces_type>;
		template<std::size_t Index>
		using interface_type = std::tuple_element_t<Index, interfaces_type>;

		using reflection_type = void;

		constexpr static auto maximum_size = sizeof(std::max_align_t) * 2;
		constexpr static auto maximum_alignment = alignof(std::max_align_t);

		constexpr static auto minimum_copyable_level = proxy_detail::constraint_level::none;
		constexpr static auto minimum_moveable_level = proxy_detail::constraint_level::nothrow;
		constexpr static auto minimum_destructible_level = proxy_detail::constraint_level::nothrow;
	};

	namespace proxy_detail
	{
		template<typename... Interfaces>
		struct is_facade : std::false_type {};

		template<interface_t... Interfaces>
		struct is_facade<as_facade<Interfaces...>> : std::true_type {};

		template<typename T, typename InterfaceTuple>
		struct is_derived_from_facade;

		template<typename T, typename... Interfaces>
		struct is_derived_from_facade<T, std::tuple<Interfaces...>> : std::is_base_of<as_facade<Interfaces...>, T> { };

		template<typename T, typename InterfaceTuple>
		constexpr static bool is_derived_from_facade_v = is_derived_from_facade<T, InterfaceTuple>::value;
		template<typename T, typename InterfaceTuple>
		concept derived_from_facade_t = is_derived_from_facade_v<T, InterfaceTuple>;

		template<typename T>
			requires derived_from_facade_t<T, typename T::interfaces_type>
		struct is_facade<T> : std::true_type {};

		template<typename... Interfaces>
		constexpr static bool is_facade_v = is_facade<Interfaces...>::value;
		template<typename... Interfaces>
		concept facade_t = is_facade_v<Interfaces...>;

		using pointer_type = unsigned char*;
		using const_pointer_type = const unsigned char*;

		template<typename, constraint_level>
		struct is_copyable;
		template<typename, constraint_level>
		struct is_moveable;
		template<typename, constraint_level>
		struct is_destructible;

		template<typename T, constraint_level Level>
		constexpr bool is_copyable_v = is_copyable<T, Level>::value;
		template<typename T, constraint_level Level>
		constexpr bool is_moveable_v = is_moveable<T, Level>::value;
		template<typename T, constraint_level Level>
		constexpr bool is_destructible_v = is_destructible<T, Level>::value;

		template<typename T>
		struct is_copyable<T, constraint_level::none> : std::true_type { };

		template<typename T>
		struct is_copyable<T, constraint_level::nontrivial> : std::is_copy_constructible<T> { };

		template<typename T>
		struct is_copyable<T, constraint_level::nothrow> : std::is_nothrow_copy_constructible<T> { };

		template<typename T>
		struct is_copyable<T, constraint_level::trivial> : std::is_trivially_copy_constructible<T> { };

		template<typename T>
		struct is_moveable<T, constraint_level::none> : std::true_type { };

		template<typename T>
		struct is_moveable<T, constraint_level::nontrivial> : std::bool_constant<std::is_move_constructible_v<T> && is_destructible_v<T, constraint_level::nontrivial>> { };

		template<typename T>
		struct is_moveable<T, constraint_level::nothrow> : std::bool_constant<std::is_nothrow_move_constructible_v<T> && is_destructible_v<T, constraint_level::nothrow>> { };

		template<typename T>
		struct is_moveable<T, constraint_level::trivial> : std::bool_constant<std::is_trivially_move_constructible_v<T> && is_destructible_v<T, constraint_level::trivial>> { };

		template<typename T>
		struct is_destructible<T, constraint_level::none> : std::true_type { };

		template<typename T>
		struct is_destructible<T, constraint_level::nontrivial> : std::is_destructible<T> { };

		template<typename T>
		struct is_destructible<T, constraint_level::nothrow> : std::is_nothrow_destructible<T> { };

		template<typename T>
		struct is_destructible<T, constraint_level::trivial> : std::is_trivially_destructible<T> { };

		template<typename Interface, typename ParametersTuple>
		struct as_interface_trait_helper : std::false_type {};

		template<interface_t Interface, typename... Args>
		struct as_interface_trait_helper<Interface, std::tuple<Args...>> : std::true_type
		{
			using interface_type = Interface;
			using return_type = typename interface_type::template return_type;

			// using invoker_signature = return_type(*)(pointer_type, Args&&...);
			using invoker_signature = return_type(*)(pointer_type, Args ...);

			template<typename SelfPointer>
			constexpr static bool is_invokable_v = requires(SelfPointer&& self, Args&&... args)
			{
				{
					std::invoke(interface_type{}, std::forward<SelfPointer>(self), std::forward<Args>(args)...)
				} -> std::convertible_to<return_type>;
			};

			template<typename SelfPointer>
			// static return_type invoke(pointer_type self, Args&&... args) { return std::invoke(interface_type{}, **reinterpret_cast<SelfPointer*>(self), std::forward<Args>(args)...); }
			static return_type invoke(pointer_type self, Args ... args) { return std::invoke(interface_type{}, **reinterpret_cast<SelfPointer*>(self), std::forward<Args>(args)...); }
		};

		template<typename Interface>
		struct as_interface_trait : std::false_type {};

		template<interface_t Interface>
		struct as_interface_trait<Interface> : as_interface_trait_helper<Interface, typename Interface::parameters_type> { };

		template<interface_t Interface>
		struct meta_interface
		{
			using interface_type = typename as_interface_trait<Interface>::template interface_type;
			using invoker_signature = typename as_interface_trait<Interface>::template invoker_signature;

			invoker_signature invoker;

			template<typename SelfPointer>
			constexpr explicit meta_interface(std::in_place_type_t<SelfPointer>)
				: invoker{as_interface_trait<Interface>::template invoke<SelfPointer>} { }
		};

		struct meta_copyable
		{
			using copy_signature = void(*)(pointer_type, const_pointer_type);

			copy_signature copy_ptr;

			template<typename SelfPointer>
			constexpr explicit meta_copyable(std::in_place_type_t<SelfPointer>)
				: copy_ptr{+[](pointer_type self, const_pointer_type other) { std::construct_at(reinterpret_cast<SelfPointer*>(self), *reinterpret_cast<const SelfPointer*>(other)); }} {}
		};

		struct meta_moveable
		{
			using move_signature = void(*)(pointer_type, pointer_type);

			move_signature move_ptr;

			template<typename SelfPointer>
			constexpr explicit meta_moveable(std::in_place_type_t<SelfPointer>)
				: move_ptr{+[](pointer_type self, pointer_type other)
				{
					std::construct_at(reinterpret_cast<SelfPointer*>(self), std::move(*reinterpret_cast<SelfPointer*>(other)));
					std::destroy_at(reinterpret_cast<SelfPointer*>(other));
				}} {}
		};

		struct meta_destructible
		{
			using destruct_signature = void(*)(pointer_type);

			destruct_signature destruct_ptr;

			template<typename SelfPointer>
			constexpr explicit meta_destructible(std::in_place_type_t<SelfPointer>)
				: destruct_ptr{+[](pointer_type self) { std::destroy_at(reinterpret_cast<SelfPointer*>(self)); }} {}
		};

		template<typename... Metas>
		struct meta_facade : Metas...
		{
			template<typename Signature>
			constexpr explicit meta_facade(std::in_place_type_t<Signature>)
				: Metas{std::in_place_type<Signature>}... { }
		};

		template<constraint_level Level, typename Meta>
		struct meta_tag {};

		template<typename Meta, typename MetaFacade>
		struct meta_facade_trait_helper;

		template<typename Meta, typename... Metas>
		struct meta_facade_trait_helper<Meta, meta_facade<Metas...>>
		{
			using type = meta_facade<Meta, Metas...>;
		};

		template<constraint_level Level, typename Meta, typename... Metas>
		// Level == nontrivial/nothrow
			requires(Level > constraint_level::none && Level < constraint_level::trivial)
		struct meta_facade_trait_helper<meta_tag<Level, Meta>, meta_facade<Metas...>>
		{
			using type = meta_facade<Meta, Metas...>;
		};

		template<constraint_level Level, typename Meta, typename... Metas>
		// Level == none/trivial
			requires(Level < constraint_level::nontrivial || Level > constraint_level::nothrow)
		struct meta_facade_trait_helper<meta_tag<Level, Meta>, meta_facade<Metas...>>
		{
			using type = meta_facade<Metas...>;
		};

		template<typename... Metas>
		struct meta_facade_trait;

		template<typename Meta, typename... Metas>
		struct meta_facade_trait<Meta, Metas...> : meta_facade_trait_helper<Meta, typename meta_facade_trait<Metas...>::type> {};

		template<>
		struct meta_facade_trait<>
		{
			using type = meta_facade<>;
		};

		template<typename Required, typename... Targets>
		struct is_any_of : std::false_type {};

		template<typename Required, typename... Targets>
		struct is_any_of<Required, Required, Targets...> : std::true_type {};

		template<typename Required, typename FirstTarget, typename... Targets>
		struct is_any_of<Required, FirstTarget, Targets...> : is_any_of<Required, Targets...> {};

		template<typename Required, typename... Targets>
		constexpr static bool is_any_of_v = is_any_of<Required, Targets...>::value;
		template<typename Required, typename... Targets>
		concept is_any_of_t = is_any_of_v<Required, Targets...>;

		template<typename T, typename U>
		struct flattening_trait_helper;

		template<typename T>
		struct flattening_trait_helper<std::tuple<>, T>
		{
			using type = T;
		};

		template<typename T, typename... Ts, typename U>
		struct flattening_trait_helper<std::tuple<T, Ts...>, U>
				: flattening_trait_helper<std::tuple<Ts...>, U> {};

		template<typename T, typename... Ts, typename... Us>
			requires(not is_any_of_v<T, Us...>)
		struct flattening_trait_helper<std::tuple<T, Ts...>, std::tuple<Us...>>
				: flattening_trait_helper<std::tuple<Ts...>, std::tuple<Us..., T>> {};

		template<typename T>
		struct flattening_trait
		{
			using type = std::tuple<T>;
		};

		template<>
		struct flattening_trait<std::tuple<>>
		{
			using type = std::tuple<>;
		};

		template<typename T, typename... Ts>
		struct flattening_trait<std::tuple<T, Ts...>>
				: flattening_trait_helper<
					typename flattening_trait<T>::type,
					typename flattening_trait<std::tuple<Ts...>>::type
				> {};

		template<typename Facade, typename InterfaceTuple>
		struct facade_checker_helper;

		template<facade_t Facade, interface_t... Interfaces>
		struct facade_checker_helper<Facade, std::tuple<Interfaces...>> : std::true_type
		{
			using meta_type = typename meta_facade_trait<
				meta_tag<Facade::minimum_copyable_level, meta_copyable>,
				meta_tag<Facade::minimum_moveable_level, meta_moveable>,
				meta_tag<Facade::minimum_destructible_level, meta_destructible>,
				meta_tag<std::is_void_v<typename Facade::reflection_type> ? constraint_level::none : constraint_level::nothrow, typename Facade::reflection_type>>::type;

			using default_invoke_signature = std::conditional_t<
				std::tuple_size_v<std::tuple<Interfaces...>> == 1,
				std::tuple_element_t<0, std::tuple<Interfaces...>>,
				void>;

			template<interface_t Interface>
			constexpr static bool is_part_of = is_any_of_v<Interface, Interfaces...>;
		};

		template<typename Facade>
		struct facade_checker : std::false_type {};

		template<facade_t Facade>
		struct facade_checker<Facade> : facade_checker_helper<Facade, typename flattening_trait<typename Facade::interfaces_type>::type> {};

		template<typename PointerType>
		struct pointer_trait : std::false_type {};

		/**
		 * @brief Treat it as a pointer as long as it can be dereferenced.
		 */
		template<typename PointerType>
			requires requires(PointerType& p) { *p; }
		struct pointer_trait<PointerType> : std::true_type
		{
			using element_type = decltype(*std::declval<PointerType&>());
		};

		/**
		 * @brief Basically equivalent to an optional, but never checks if it has a value (always has a value).
		 */
		template<typename T>
		class sbo_ptr
		{
		public:
			using value_type = T;

		private:
			value_type data_;

		public:
			template<typename... Args>
			constexpr explicit sbo_ptr(Args&&... args)
				: data_{std::forward<Args>(args)...} { }

			constexpr sbo_ptr(const sbo_ptr&) noexcept(std::is_nothrow_copy_constructible_v<value_type>) = default;
			constexpr sbo_ptr& operator=(const sbo_ptr&) noexcept(std::is_nothrow_copy_assignable_v<value_type>) = default;
			constexpr sbo_ptr(sbo_ptr&&) noexcept(std::is_nothrow_move_constructible_v<value_type>) = default;
			constexpr sbo_ptr& operator=(sbo_ptr&&) noexcept(std::is_nothrow_move_assignable_v<value_type>) = default;
			constexpr ~sbo_ptr() noexcept(std::is_nothrow_destructible_v<value_type>) = default;

			[[nodiscard]] constexpr T& operator*() { return data_; }
		};

		/**
		 * @brief Basically equivalent to a unique_ptr, but allows copying.
		 */
		template<typename T>
		class deep_ptr
		{
		public:
			using value_type = T;

		private:
			value_type* data_;

		public:
			template<typename... Args>
			constexpr explicit deep_ptr(Args&&... args)
				: data_{new value_type{std::forward<Args>(args)...}} {}

			constexpr deep_ptr(const deep_ptr& other) noexcept(std::is_nothrow_copy_constructible_v<value_type>)
				requires std::is_copy_constructible_v<value_type>
				: data_{other.data_ ? new value_type{*other.data_} : nullptr} {}

			constexpr deep_ptr& operator=(const deep_ptr& other) noexcept(std::is_nothrow_copy_assignable_v<value_type>)
				requires std::is_copy_assignable_v<value_type>
			{
				if (&other == this) { return *this; }

				delete data_;
				data_ = new value_type{*other.data_};
				return *this;
			}

			constexpr deep_ptr(deep_ptr&& other) noexcept(std::is_nothrow_move_constructible_v<value_type>)
				requires std::is_move_constructible_v<value_type>
				: data_{std::exchange(other.data_, nullptr)} {}

			constexpr deep_ptr& operator=(deep_ptr&& other) noexcept(std::is_nothrow_move_assignable_v<value_type>)
				requires std::is_move_assignable_v<value_type>
			{
				data_ = std::exchange(other.data_, nullptr);
				return *this;
			}

			constexpr ~deep_ptr() noexcept(std::is_nothrow_destructible_v<value_type>)
				requires std::is_destructible_v<value_type>
			{
				delete data_;
				data_ = nullptr;
			}
		};

		template<typename Facade, typename InterfaceTuple>
		struct facade_meta_holder_helper : std::false_type { };

		template<facade_t Facade, interface_t... Interfaces>
			requires(as_interface_trait<Interfaces>::value && ...)
		struct facade_meta_holder_helper<Facade, std::tuple<Interfaces...>> : std::true_type
		{
			using interfaces_type = std::tuple<Interfaces...>;
			constexpr static std::size_t interface_size = std::tuple_size_v<interfaces_type>;
			template<std::size_t Index>
			using interface_type = std::tuple_element_t<Index, interfaces_type>;

			using meta_type = meta_facade<
				typename facade_checker<Facade>::meta_type,
				meta_interface<Interfaces>...>;

			template<typename PointerType>
			constexpr static bool is_invokable_v =
					sizeof(PointerType) <= Facade::maximum_size &&
					alignof(PointerType) <= Facade::maximum_alignment &&
					is_copyable_v<PointerType, Facade::minimum_copyable_level> &&
					is_moveable_v<PointerType, Facade::minimum_moveable_level> &&
					is_destructible_v<PointerType, Facade::minimum_destructible_level> &&
					(as_interface_trait<Interfaces>::template is_invokable_v<typename pointer_trait<PointerType>::element_type> && ...) &&
					(std::is_void_v<typename Facade::reflection_type> || std::is_constructible_v<typename Facade::reflection_type, std::in_place_type_t<PointerType>>);

			template<typename PointerType>
			constexpr static meta_type meta{std::in_place_type<PointerType>};
		};

		template<facade_t Facade>
		struct facade_meta_holder : facade_meta_holder_helper<Facade, typename flattening_trait<typename Facade::interfaces_type>::type> {};
	}

	template<typename PointerType, typename Facade>
	concept proxiable =
	proxy_detail::pointer_trait<PointerType>::value &&
	proxy_detail::facade_checker<Facade>::value &&
	proxy_detail::facade_meta_holder<Facade>::value &&
	proxy_detail::facade_meta_holder<Facade>::template is_invokable_v<PointerType>;

	template<typename Facade>
		requires proxy_detail::facade_checker<Facade>::value
	class proxy
	{
	public:
		using facade_checker_type = proxy_detail::facade_checker<Facade>;
		using facade_meta_holder_type = proxy_detail::facade_meta_holder<Facade>;

		template<typename PointerType, typename... Args>
		constexpr static bool is_trivial_polymorphic_constructible = proxiable<PointerType, Facade> && std::is_trivially_constructible_v<PointerType, Args...>;
		template<typename PointerType, typename... Args>
		constexpr static bool is_nothrow_polymorphic_constructible = proxiable<PointerType, Facade> && std::is_nothrow_constructible_v<PointerType, Args...>;
		template<typename PointerType, typename... Args>
		constexpr static bool is_polymorphic_constructible = proxiable<PointerType, Facade> && std::is_constructible_v<PointerType, Args...>;

		constexpr static bool is_trivial_copyable = Facade::minimum_copyable_level == proxy_detail::constraint_level::trivial;
		constexpr static bool is_nothrow_copyable = Facade::minimum_copyable_level >= proxy_detail::constraint_level::nothrow;
		constexpr static bool is_copyable = Facade::minimum_copyable_level >= proxy_detail::constraint_level::nontrivial;

		constexpr static bool is_trivial_moveable = Facade::minimum_moveable_level == proxy_detail::constraint_level::trivial;
		constexpr static bool is_nothrow_moveable = Facade::minimum_moveable_level >= proxy_detail::constraint_level::nothrow;
		constexpr static bool is_moveable = Facade::minimum_moveable_level >= proxy_detail::constraint_level::nontrivial;

		constexpr static bool is_trivial_destructible = Facade::minimum_destructible_level == proxy_detail::constraint_level::trivial;
		constexpr static bool is_nothrow_destructible = Facade::minimum_destructible_level >= proxy_detail::constraint_level::nothrow;
		constexpr static bool is_destructible = Facade::minimum_destructible_level >= proxy_detail::constraint_level::nontrivial;

		template<typename PointerType, typename... Args>
		constexpr static bool is_trivial_polymorphic_assignment = is_trivial_polymorphic_constructible<PointerType, Args...> && is_trivial_destructible;
		template<typename PointerType, typename... Args>
		constexpr static bool is_nothrow_polymorphic_assignment = is_nothrow_polymorphic_constructible<PointerType, Args...> && is_nothrow_destructible;
		template<typename PointerType, typename... Args>
		constexpr static bool is_polymorphic_assignment = is_polymorphic_constructible<PointerType, Args...> && is_destructible;

		constexpr static bool is_trivial_copy_assignment = is_trivial_copyable && is_trivial_destructible;
		constexpr static bool is_nothrow_copy_assignment = is_nothrow_copyable && is_nothrow_destructible;
		constexpr static bool is_copy_assignment = (is_copyable && is_moveable && is_destructible) || is_nothrow_copy_assignment;

		constexpr static bool is_trivial_move_assignment = is_trivial_moveable && is_trivial_destructible;
		constexpr static bool is_nothrow_move_assignment = is_nothrow_moveable && is_nothrow_destructible;
		constexpr static bool is_move_assignment = is_moveable && is_destructible;

	private:
		const typename facade_checker_type::meta_type* meta_{nullptr};
		alignas(Facade::maximum_alignment) std::pointer_traits<proxy_detail::pointer_type>::element_type ptr_[Facade::maximum_size]{};

	public:
		constexpr proxy() noexcept = default;

		constexpr explicit(false) proxy(std::nullptr_t) noexcept
			: proxy{} {}

		constexpr proxy& operator=(std::nullptr_t) noexcept(is_nothrow_destructible) requires is_destructible
		{
			std::destroy_at(this);
			std::construct_at(this, nullptr);
			return *this;
		}

		constexpr proxy(const proxy& other) noexcept(is_nothrow_copyable) requires(not is_trivial_copyable && is_copyable)
		{
			if (other.meta_)
			{
				other.meta_->copy_ptr(ptr_, other.ptr_);
				meta_ = other.meta_;
			}
			else { meta_ = nullptr; }
		}

		constexpr proxy(const proxy&) noexcept requires(is_trivial_copyable) = default;
		constexpr proxy(const proxy&) requires(not is_copyable) = delete;

		constexpr proxy& operator =(const proxy& other) noexcept requires(not is_trivial_copy_assignment && is_nothrow_copy_assignment)
		{
			if (&other == this) { return *this; }

			std::destroy_at(this);
			std::construct_at(this, other);
			return *this;
		}

		constexpr proxy& operator=(const proxy& other) requires(not is_nothrow_copy_assignment && is_copy_assignment)
		{
			if (&other == this) { return *this; }

			auto tmp{other};
			swap(tmp);
			return *this;
		}

		constexpr proxy& operator=(const proxy&) noexcept requires is_trivial_copy_assignment = default;
		constexpr proxy& operator=(const proxy&) requires(not is_copy_assignment) = delete;

		constexpr proxy(proxy&& other) noexcept(is_nothrow_moveable) requires(is_moveable)
		{
			if (other.meta_)
			{
				if constexpr (is_trivial_moveable) { std::memcpy(ptr_, other.ptr_, Facade::maximum_size); }
				else { other.meta_->move_ptr(ptr_, other.ptr_); }

				meta_ = std::exchange(other.meta_, nullptr);
			}
			else { meta_ = nullptr; }
		}

		constexpr proxy(proxy&&) requires(not is_moveable) = delete;

		constexpr proxy& operator =(proxy&& other) noexcept requires is_nothrow_move_assignment
		{
			if (&other == this) { return *this; }

			std::destroy_at(this);
			std::construct_at(this, std::move(other));
			return *this;
		}

		constexpr proxy& operator=(proxy&& other) noexcept(false) requires(not is_nothrow_move_assignment && is_move_assignment)
		{
			auto tmp{std::move(other)};
			swap(tmp);
			return *this;
		}

		constexpr proxy& operator=(proxy&&) requires(not is_move_assignment) = delete;

		// allows implicit conversion of pointers of other types
		template<typename PointerType>
			requires is_polymorphic_constructible<std::decay_t<PointerType>, PointerType>
		constexpr explicit(false) proxy(PointerType&& pointer) noexcept(is_nothrow_polymorphic_constructible<std::decay_t<PointerType>, PointerType>)
			: proxy{std::in_place_type<std::decay_t<PointerType>>, std::forward<PointerType>(pointer)} { }

		template<typename PointerType>
			requires is_nothrow_polymorphic_assignment<std::decay_t<PointerType>, PointerType>
		constexpr proxy& operator=(PointerType&& pointer) noexcept
		{
			std::destroy_at(this);
			std::construct_at(this, std::forward<PointerType>(pointer));
			return *this;
		}

		template<typename PointerType>
			requires(not is_nothrow_polymorphic_assignment<std::decay_t<PointerType>, PointerType> && is_polymorphic_assignment<std::decay_t<PointerType>, PointerType>)
		constexpr proxy& operator=(PointerType&& pointer)
		{
			auto tmp{std::forward<PointerType>(pointer)};
			swap(tmp);
			return *this;
		}

		template<typename PointerType, typename... Args>
			requires is_polymorphic_constructible<PointerType, Args...>
		constexpr explicit proxy(std::in_place_type_t<PointerType>, Args&&... args) noexcept(is_nothrow_polymorphic_constructible<PointerType, Args...>)
		{
			std::construct_at(reinterpret_cast<PointerType*>(ptr_), std::forward<Args>(args)...);
			meta_ = &facade_meta_holder_type::template meta<PointerType>;
		}

		template<typename PointerType, typename U, typename... Args>
			requires is_polymorphic_constructible<PointerType, std::initializer_list<U>&, Args...>
		constexpr explicit proxy(std::in_place_type_t<PointerType>, std::initializer_list<U> list, Args&&... args) noexcept(is_nothrow_polymorphic_constructible<PointerType, std::initializer_list<U>&, Args...>)
			: proxy{std::in_place_type<PointerType>, list, std::forward<Args>(args)...} { }

		constexpr ~proxy() noexcept(is_nothrow_destructible) requires(not is_trivial_destructible && is_destructible) { if (meta_) { meta_->destruct_ptr(ptr_); } }

		constexpr ~proxy() requires is_trivial_destructible = default;

		constexpr ~proxy() requires(not is_destructible) = delete;

		[[nodiscard]] constexpr bool has_value() const noexcept { return meta_; }

		[[nodiscard]] constexpr decltype(auto) reflect() const noexcept
			requires(not std::is_void_v<typename Facade::reflection_type>) { return static_cast<const typename Facade::reflection_type&>(*meta_); }

		constexpr void reset() noexcept(is_nothrow_destructible) requires is_destructible
		{
			std::destroy_at(this);
			meta_ = nullptr;
		}

		constexpr void swap(proxy& other) noexcept(is_nothrow_moveable) requires is_moveable
		{
			if constexpr (Facade::minimum_moveable_level == proxy_detail::constraint_level::trivial)
			{
				using std::swap;
				swap(meta_, other.meta_);
				swap(ptr_, other.ptr_);
			}
			else
			{
				if (meta_)
				{
					if (other.meta_)
					{
						auto tmp{std::move(*this)};
						std::construct_at(this, std::move(other));
						std::construct_at(&other, std::move(tmp));
					}
					else { std::construct_at(&other, std::move(*this)); }
				}
				else if (other.meta_) { std::construct_at(this, std::move(other)); }
			}
		}

		template<typename PointerType, typename... Args>
			requires is_polymorphic_assignment<PointerType, Args...>
		constexpr PointerType& emplace(Args&&... args) noexcept(is_nothrow_polymorphic_assignment<PointerType, Args...>)
		{
			reset();
			std::construct_at(this, std::in_place_type<PointerType>, std::forward<Args>(args)...);
			return *static_cast<PointerType*>(ptr_);
		}

		template<typename PointerType, typename U, typename... Args>
			requires is_polymorphic_assignment<PointerType, std::initializer_list<U>&, Args...>
		constexpr PointerType& emplace(std::initializer_list<U> list, Args&&... args) noexcept(is_nothrow_polymorphic_assignment<PointerType, std::initializer_list<U>&, Args...>) { return this->template emplace<PointerType>(list, std::forward<Args>(args)...); }

		template<typename InvokeSignature = typename facade_checker_type::default_invoke_signature, typename... Args>
		constexpr decltype(auto) invoke(Args&&... args) requires(
			facade_meta_holder_type::value &&
			facade_checker_type::template is_part_of<InvokeSignature> &&
			std::is_convertible_v<std::tuple<Args...>, typename InvokeSignature::parameters_type>
		) { return static_cast<const typename facade_meta_holder_type::meta_type*>(meta_)->template meta_interface<InvokeSignature>::invoker(ptr_, std::forward<Args>(args)...); }
	};

	namespace proxy_detail
	{
		template<facade_t Facade, typename T, typename... Args>
			requires std::is_constructible_v<proxy<Facade>, std::in_place_type_t<std::conditional_t<proxiable<sbo_ptr<T>, Facade>, sbo_ptr<T>, deep_ptr<T>>>, Args...>
		[[nodiscard]] constexpr proxy<Facade> make_proxy(Args&&... args)
		{
			return proxy<Facade>{std::in_place_type<
				                     std::conditional_t<
					                     proxiable<sbo_ptr<T>, Facade>,
					                     sbo_ptr<T>,
					                     deep_ptr<T>>>,
			                     std::forward<Args>(args)...};
		}
	}

	template<proxy_detail::facade_t Facade, typename T, typename... Args>
	[[nodiscard]] constexpr proxy<Facade> make_proxy(Args&&... args) { return proxy_detail::make_proxy<Facade, T>(std::forward<Args>(args)...); }

	template<proxy_detail::facade_t Facade, typename T, typename U, typename... Args>
	[[nodiscard]] constexpr proxy<Facade> make_proxy(std::initializer_list<U> list, Args&&... args) { return proxy_detail::make_proxy<Facade, T>(list, std::forward<Args>(args)); }

	template<proxy_detail::facade_t Facade, typename T>
	[[nodiscard]] constexpr proxy<Facade> make_proxy(T&& value) { return proxy_detail::make_proxy<Facade, std::decay_t<T>>(std::forward<T>(value)); }

	template<proxy_detail::facade_t Facade>
	constexpr void swap(proxy<Facade>& lhs, proxy<Facade>& rhs) noexcept(noexcept(lhs.swap(rhs))) { lhs.swap(rhs); }
}

#endif // GAL_UTILS_PROXY_HPP
