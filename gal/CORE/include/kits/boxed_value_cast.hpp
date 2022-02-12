#pragma once

#ifndef GAL_LANG_KITS_BOXED_VALUE_CAST_HPP
#define GAL_LANG_KITS_BOXED_VALUE_CAST_HPP

#include <vector>
#include <set>
#include <algorithm>
#include <atomic>
#include <utils/format.hpp>
#include <kits/boxed_value.hpp>
#include <utils/thread_storage.hpp>

namespace gal::lang::kits
{
	class type_conversion_state;

	namespace detail
	{
		template<typename T>
		constexpr T* verify_pointer(T* ptr)
		{
			if (ptr) { return ptr; }
			throw std::runtime_error{"Attempted to dereference a null boxed_value"};
		}

		template<typename T>
		static const T* verify_type(const boxed_value& object, const std::type_info& ti, const T* ptr)
		{
			if (object.type_info() == ti) { return ptr; }
			throw std::bad_any_cast{};
		}

		template<typename T>
		static T* verify_type(const boxed_value& object, const std::type_info& ti, T* ptr)
		{
			if (not object.is_const() && object.type_info() == ti) { return ptr; }
			throw std::bad_any_cast{};
		}

		template<typename T>
		static const T* verify_bare_type(const boxed_value& object, const std::type_info& ti, const T* ptr)
		{
			if (object.type_info().bare_equal(ti)) { return verify_pointer(ptr); }
			throw std::bad_any_cast{};
		}

		template<typename T>
		static T* verify_bare_type(const boxed_value& object, const std::type_info& ti, T* ptr)
		{
			if (not object.is_const() && object.type_info().bare_equal(ti)) { return verify_pointer(ptr); }
			throw std::bad_any_cast{};
		}

		/**
		 * @brief for casting to any type
		 */
		template<typename Result>
		struct cast_helper
		{
			static Result cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_ptr())); }
		};

		/**
		 * @brief for casting to any const type
		 */
		template<typename Result>
		struct cast_helper<const Result> : cast_helper<Result> {};

		/**
		 * @brief for casting to pointer type
		 */
		template<typename Result>
		struct cast_helper<Result*>
		{
			static Result* cast(const boxed_value& object, const type_conversion_state*) { return static_cast<Result*>(verify_type(object, typeid(Result), object.get_ptr())); }
		};

		/**
		 * @brief for casting to const pointer type
		 */
		template<typename Result>
		struct cast_helper<const Result*>
		{
			static const Result* cast(const boxed_value& object, const type_conversion_state*) { return static_cast<const Result*>(verify_type(object, typeid(Result), object.get_const_ptr())); }
		};

		/**
		 * @brief for casting to pointer type
		 */
		template<typename Result>
		struct cast_helper<Result* const&> : cast_helper<Result*> {};

		/**
		 * @brief for casting to const pointer type
		 */
		template<typename Result>
		struct cast_helper<const Result* const&> : cast_helper<const Result*> {};

		/**
		 * @brief for casting to reference type
		 */
		template<typename Result>
		struct cast_helper<Result&>
		{
			static Result& cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_ptr())); }
		};

		/**
		 * @brief for casting to const reference type
		 */
		template<typename Result>
		struct cast_helper<const Result&>
		{
			static const Result& cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_ptr())); }
		};

		/**
		 * @brief for casting to rvalue-reference type
		 */
		template<typename Result>
		struct cast_helper<Result&&>
		{
			static Result&& cast(const boxed_value& object, const type_conversion_state*) { return std::move(*static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_ptr()))); }
		};

		/**
		 * @brief for casting to std::unique_ptr reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
		template<typename Result>
		struct cast_helper<std::unique_ptr<Result>&>
		{
			static std::unique_ptr<Result>& cast(const boxed_value& object, const type_conversion_state*) { return *std::any_cast<std::shared_ptr<std::unique_ptr<Result>>>(object.get()); }
		};

		/**
		 * @brief for casting to std::unique_ptr reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
		template<typename Result>
		struct cast_helper<const std::unique_ptr<Result>&>
		{
			static std::unique_ptr<Result>& cast(const boxed_value& object, const type_conversion_state*) { return *std::any_cast<std::shared_ptr<std::unique_ptr<Result>>>(object.get()); }
		};

		/**
		 * @brief for casting to std::unique_ptr rvalue-reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
		template<typename Result>
		struct cast_helper<std::unique_ptr<Result>&&>
		{
			static std::unique_ptr<Result>&& cast(const boxed_value& object, const type_conversion_state*) { return std::move(*std::any_cast<std::shared_ptr<std::unique_ptr<Result>>>(object.get())); }
		};

		/**
		 * @brief for casting to std::shared_ptr type
		 */
		template<typename Result>
		struct cast_helper<std::shared_ptr<Result>>
		{
			static std::shared_ptr<Result> cast(const boxed_value& object, const type_conversion_state*) { return std::any_cast<std::shared_ptr<Result>>(object.get()); }
		};

		/**
		 * @brief for casting to const std::shared_ptr type
		 */
		template<typename Result>
		struct cast_helper<const std::shared_ptr<Result>> : cast_helper<std::shared_ptr<Result>> {};

		/**
		 * @brief for casting to std::shared_ptr reference type
		 */
		template<typename Result>
		struct cast_helper<std::shared_ptr<Result>&>
		{
			static_assert(not std::is_const_v<Result>, "Non-const reference to std::shared_ptr<const T> is not supported");

			static auto cast(const boxed_value& object, const type_conversion_state*)
			{
				auto& result = std::any_cast<std::shared_ptr<Result>>(object.get());
				return object.pointer_sentinel(result);
			}
		};

		/**
		 * @brief for casting to const std::shared_ptr reference type
		 */
		template<typename Result>
		struct cast_helper<const std::shared_ptr<Result>&> : cast_helper<std::shared_ptr<Result>> {};

		/**
		 * @brief for casting to std::shared_ptr-inner-const type
		 */
		template<typename Result>
		struct cast_helper<std::shared_ptr<const Result>>
		{
			static std::shared_ptr<const Result> cast(const boxed_value& object, const type_conversion_state*)
			{
				if (not object.type_info().is_const()) { return std::const_pointer_cast<const Result>(std::any_cast<std::shared_ptr<Result>>(object.get())); }
				return std::any_cast<std::shared_ptr<const Result>>(object.get());
			}
		};

		/**
		 * @brief for casting to const std::shared_ptr-inner-const type
		 */
		template<typename Result>
		struct cast_helper<const std::shared_ptr<const Result>> : cast_helper<std::shared_ptr<const Result>> {};

		/**
		 * @brief for casting to const std::shared_ptr-inner-const reference type
		 */
		template<typename Result>
		struct cast_helper<const std::shared_ptr<const Result>&> : cast_helper<std::shared_ptr<const Result>> {};

		/**
		 * @brief for casting to boxed_value type
		 */
		template<>
		struct cast_helper<boxed_value>
		{
			static boxed_value cast(const boxed_value& object, const type_conversion_state*) { return object; }
		};

		/**
		 * @brief for casting to const boxed_value type
		 */
		template<>
		struct cast_helper<const boxed_value> : cast_helper<boxed_value> {};

		/**
		 * @brief for casting to boxed_value reference type
		 */
		template<>
		struct cast_helper<boxed_value&>
		{
			static std::reference_wrapper<boxed_value> cast(const boxed_value& object, const type_conversion_state*) { return std::ref(const_cast<boxed_value&>(object)); }
		};

		/**
		 * @brief for casting to const boxed_value reference type
		 */
		template<>
		struct cast_helper<const boxed_value&> : cast_helper<boxed_value> {};

		/**
		 * @brief for casting to std::reference_wrapper type
		 */
		template<typename Result>
		struct cast_helper<std::reference_wrapper<Result>> : cast_helper<Result&> {};

		/**
		 * @brief for casting to const std::reference_wrapper type
		 */
		template<typename Result>
		struct cast_helper<const std::reference_wrapper<Result>> : cast_helper<Result&> {};

		/**
		 * @brief for casting to const std::reference_wrapper reference type
		 */
		template<typename Result>
		struct cast_helper<const std::reference_wrapper<Result>&> : cast_helper<Result&> {};

		/**
		 * @brief for casting to std::reference_wrapper-inner-const type
		 */
		template<typename Result>
		struct cast_helper<std::reference_wrapper<const Result>> : cast_helper<const Result&> {};

		/**
		 * @brief for casting to const std::reference_wrapper-inner-const type
		 */
		template<typename Result>
		struct cast_helper<const std::reference_wrapper<const Result>> : cast_helper<const Result&> {};

		/**
		 * @brief for casting to const std::reference_wrapper-inner-const reference type
		 */
		template<typename Result>
		struct cast_helper<const std::reference_wrapper<const Result>&> : cast_helper<const Result&> {};

		template<typename T>
		decltype(auto) help_cast(const boxed_value& object, const type_conversion_state* conversion) { return cast_helper<T>::cast(object, conversion); }
	}// namespace detail

	/**
	 * @brief Thrown in the event that a boxed_value cannot be cast to the desired type.
	 *
	 * @note It is used internally during function dispatch and may be used by the end user.
	 */
	class bad_boxed_cast : public std::bad_cast
	{
	private:
		const char* what_;

	public:
		// gal_type_info contained in the boxed_value
		utils::gal_type_info from;
		// std::type_info of the desired (but failed) result type
		const std::type_info* to;

		bad_boxed_cast(utils::gal_type_info from, const std::type_info& to, const char* const what) noexcept
			: what_{what},
			  from{from},
			  to{&to} {}

		bad_boxed_cast(utils::gal_type_info from, const std::type_info& to) noexcept
			: bad_boxed_cast{from, to, "Cannot perform boxed_cast"} {}

		explicit bad_boxed_cast(const char* const what) noexcept
			: what_{what},
			  to{nullptr} {}

		[[nodiscard]] char const* what() const override { return what_; }
	};

	/**
	 * @brief Error thrown when there's a problem with type conversion.
	 */
	class conversion_error final : public bad_boxed_cast
	{
	public:
		detail::gal_type_info type_to;

		conversion_error(const detail::gal_type_info t, const detail::gal_type_info f, const char* const what) noexcept
			: bad_boxed_cast{f, t.bare_type_info(), what},
			  type_to{t} {}
	};

	class bad_boxed_static_cast final : public bad_boxed_cast
	{
	public:
		using bad_boxed_cast::bad_boxed_cast;
	};

	class bad_boxed_dynamic_cast final : public bad_boxed_cast
	{
	public:
		using bad_boxed_cast::bad_boxed_cast;
	};

	class bad_boxed_type_cast final : public bad_boxed_cast
	{
	public:
		using bad_boxed_cast::bad_boxed_cast;
	};

	namespace detail
	{
		class type_conversion_base
		{
		private:
			const gal_type_info to_;
			const gal_type_info from_;

		protected:
			type_conversion_base(gal_type_info to, gal_type_info from)
				: to_{to},
				  from_{from} {}

		public:
			type_conversion_base(const type_conversion_base&) = default;
			type_conversion_base& operator=(const type_conversion_base&) = default;
			type_conversion_base(type_conversion_base&&) = default;
			type_conversion_base& operator=(type_conversion_base&&) = default;

			virtual ~type_conversion_base() = default;

			[[nodiscard]] constexpr virtual bool is_bidirectional() const noexcept { return true; }

			[[nodiscard]] virtual boxed_value convert(const boxed_value& from) const = 0;
			[[nodiscard]] virtual boxed_value convert_down(const boxed_value& to) const = 0;

			[[nodiscard]] const gal_type_info& to() const noexcept { return to_; }
			[[nodiscard]] const gal_type_info& from() const noexcept { return from_; }
		};

		template<bool IsStatic, typename From, typename To>
		struct caster
		{
			static boxed_value cast(const boxed_value& from)
			{
				if (from.type_info().bare_equal(utils::make_type_info<From>()))
				{
					if (from.is_pointer())
					{
						// static/dynamic cast out the contained boxed value, which we know is the type we want
						using caster_type = decltype(
							[&]<typename T, typename F>()
							{
								if constexpr (IsStatic)
								{
									if (auto data = std::static_pointer_cast<T>(cast_helper<std::shared_ptr<F>>::cast(from, nullptr));
										data) { return data; }
									throw std::bad_cast{};
								}
								else
								{
									if (auto data = std::dynamic_pointer_cast<T>(cast_helper<std::shared_ptr<F>>::cast(from, nullptr));
										data) { return data; }
									throw std::bad_cast{};
								}
							});

						if (from.is_const()) { return caster_type::template operator()<const To, const From>(); }
						return caster_type::template operator()<To, From>();
					}
					// Pull the reference out of the contained boxed value, which we know is the type we want
					using caster_type = decltype(
						[&]<typename T, typename F>() -> std::add_lvalue_reference_t<T>
						{
							auto& f = cast_helper<std::add_lvalue_reference_t<F>>::cast(from, nullptr);

							if constexpr (IsStatic) { return static_cast<std::add_lvalue_reference_t<T>>(f); }
							else { return dynamic_cast<std::add_lvalue_reference_t<T>>(f); }
						});

					if (from.is_const()) { return std::cref(caster_type::template operator()<const To, const From>()); }
					return std::ref(caster_type::template operator()<To, From>());
				}

				if constexpr (IsStatic) { throw bad_boxed_static_cast(from.type_info(), typeid(To), "Unknown static_cast_conversion"); }
				else { throw bad_boxed_dynamic_cast(from.type_info(), typeid(To), "Unknown dynamic_cast_conversion"); }
			}
		};

		template<typename Base, typename Derived>
		class static_conversion_impl final : public type_conversion_base
		{
		public:
			static_conversion_impl()
				: type_conversion_base{utils::make_type_info<Base>(), utils::make_type_info<Derived>()} {}

			[[nodiscard]] constexpr bool is_bidirectional() const noexcept override { return false; }

			[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return caster<true, Derived, Base>::cast(from); }

			[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw bad_boxed_static_cast{to.type_info(), typeid(Derived), "Unable to cast down inheritance hierarchy with non-polymorphic types"}; }
		};

		template<typename Base, typename Derived>
		class dynamic_conversion_impl final : public type_conversion_base
		{
		public:
			dynamic_conversion_impl()
				: type_conversion_base{utils::make_type_info<Base>(), utils::make_type_info<Derived>()} {}

			[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return caster<true, Derived, Base>::cast(from); }

			[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { return caster<false, Base, Derived>::cast(to); }
		};

		template<typename Callable>
		class type_conversion_impl final : public type_conversion_base
		{
		public:
			using function_type = Callable;

		private:
			function_type function_;

		public:
			type_conversion_impl(const gal_type_info from, const gal_type_info to, function_type function)
				: type_conversion_base{to, from},
				  function_{std::move(function)} {}

			[[nodiscard]] constexpr bool is_bidirectional() const noexcept override { return false; }

			[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return function_(from); }

			[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw bad_boxed_type_cast{"No conversion exists"}; }
		};
	}

	class type_conversion_manager
	{
	public:
		struct conversion_saves
		{
			bool enable = false;
			std::vector<boxed_value> saves;
		};

		struct type_info_compare
		{
			bool operator()(const std::type_info& lhs, const std::type_info& rhs) const noexcept { return lhs != rhs && lhs.before(rhs); }
		};

		using conversion_type = std::shared_ptr<detail::type_conversion_base>;
		using convertible_types_type = std::set<std::reference_wrapper<const std::type_info>, type_info_compare>;

	private:
		mutable utils::threading::shared_mutex mutex_;
		mutable utils::thread_storage<convertible_types_type> thread_cache_;
		mutable utils::thread_storage<conversion_saves> conversion_saves_;

		std::set<conversion_type> conversions_;
		convertible_types_type convertible_types_;
		std::atomic_size_t num_types_;

		auto bidirectional_find(const utils::gal_type_info& to, const utils::gal_type_info& from) const
		{
			return std::ranges::find_if(
					conversions_,
					[&to, &from](const conversion_type& conversion)
					{
						return (conversion->to().bare_equal(to) && conversion->from().bare_equal(from)) ||
						       (conversion->is_bidirectional() && conversion->from().bare_equal(to) && conversion->to().bare_equal(from));
					});
		}

		auto bidirectional_find(const conversion_type& conversion) const { return bidirectional_find(conversion->to(), conversion->from()); }

		auto find(const utils::gal_type_info& to, const utils::gal_type_info& from) const
		{
			return std::ranges::find_if(
					conversions_,
					[&to, &from](const conversion_type& conversion) { return conversion->to().bare_equal(to) && conversion->from().bare_equal(from); });
		}

		auto find(const conversion_type& conversion) const { return find(conversion->to(), conversion->from()); }

		auto copy_conversions() const
		{
			utils::threading::shared_lock lock{mutex_};
			return conversions_;
		}

	public:
		type_conversion_manager()
			: num_types_{0} {}

		type_conversion_manager(const type_conversion_manager&) = delete;
		type_conversion_manager& operator=(const type_conversion_manager&) = delete;
		type_conversion_manager(type_conversion_manager&&) = delete;
		type_conversion_manager& operator=(type_conversion_manager&&) = delete;
		~type_conversion_manager() = default;

		[[nodiscard]] const convertible_types_type& get_cache() const
		{
			auto& cache = *thread_cache_;
			if (cache.size() != num_types_)
			{
				utils::threading::shared_lock lock(mutex_);
				cache = convertible_types_;
			}

			return cache;
		}

		void add(const conversion_type& conversion)
		{
			utils::threading::shared_lock lock(mutex_);

			if (bidirectional_find(conversion) != conversions_.end()) { throw conversion_error{conversion->to(), conversion->from(), "Trying to re-insert an existing conversion"}; }

			conversions_.insert(conversion);
			convertible_types_.insert({conversion->to().bare_type_info(), conversion->from().bare_type_info()});
			num_types_ = convertible_types_.size();
		}

		[[nodiscard]] bool has_conversion(const utils::gal_type_info& to, const utils::gal_type_info& from) const
		{
			utils::threading::shared_lock lock{mutex_};
			return bidirectional_find(to, from) != conversions_.end();
		}

		template<typename T>
		[[nodiscard]] bool is_convertible_type() const noexcept { return thread_cache_->contains(utils::make_type_info<T>().bare_type_info()); }

		template<typename To, typename From>
		[[nodiscard]] bool is_convertible_type(const utils::gal_type_info& to, const utils::gal_type_info& from) const noexcept
		{
			if (const auto& cache = get_cache();
				cache.contains(to.bare_type_info()) && cache.contains(from.bare_type_info())) { return has_conversion(to, from); }
			return false;
		}

		template<typename To, typename From>
		[[nodiscard]] bool is_convertible_type() const noexcept { return is_convertible_type(utils::make_type_info<To>(), utils::make_type_info<From>()); }

		conversion_type get_conversion(const utils::gal_type_info& to, const utils::gal_type_info& from) const
		{
			utils::threading::shared_lock lock{mutex_};

			if (const auto it = find(to, from);
				it != conversions_.end()) { return *it; }

			throw std::out_of_range{
					std_format::format(
							"No such conversion exists from {} to {}",
							from.bare_name(),
							to.bare_name())};
		}

		[[nodiscard]] boxed_value boxed_type_conversion(const utils::gal_type_info& to, conversion_saves& saves, const boxed_value& from) const
		{
			try
			{
				auto ret = get_conversion(to, from.type_info())->convert(from);
				if (saves.enable) { saves.saves.push_back(ret); }
				return ret;
			}
			catch (const std::out_of_range&) { throw bad_boxed_dynamic_cast(from.type_info(), to.bare_type_info(), "No known conversion"); }
			catch (const std::bad_cast&) { throw bad_boxed_dynamic_cast(from.type_info(), to.bare_type_info(), "Unable to perform dynamic_cast operation"); }
		}

		template<typename To>
		[[nodiscard]] boxed_value boxed_type_conversion(conversion_saves& saves, const boxed_value& from) const { return boxed_type_conversion(utils::make_type_info<To>(), saves, from); }

		[[nodiscard]] boxed_value boxed_type_down_conversion(const utils::gal_type_info& from, conversion_saves& saves, const boxed_value& to) const
		{
			try
			{
				auto ret = get_conversion(to.type_info(), from)->convert_down(to);
				if (saves.enable) { saves.saves.push_back(ret); }
				return ret;
			}
			catch (const std::out_of_range&) { throw bad_boxed_dynamic_cast(to.type_info(), from.bare_type_info(), "No known conversion"); }
			catch (const std::bad_cast&) { throw bad_boxed_dynamic_cast(to.type_info(), from.bare_type_info(), "Unable to perform dynamic_cast operation"); }
		}

		template<typename From>
		[[nodiscard]] boxed_value boxed_type_down_conversion(conversion_saves& saves, const boxed_value& to) const { return boxed_type_down_conversion(utils::make_type_info<From>(), saves, to); }

		constexpr static void enable_conversion_saves(conversion_saves& saves, const bool enable) { saves.enable = enable; }

		[[nodiscard]] constexpr auto take_conversion_saves(conversion_saves& saves)
		{
			decltype(conversion_saves::saves) dummy;
			std::swap(dummy, saves.saves);
			return dummy;
		}

		[[nodiscard]] conversion_saves& get_conversion_saves() noexcept { return *conversion_saves_; }

		[[nodiscard]] const conversion_saves& get_conversion_saves() const noexcept { return *conversion_saves_; }
	};

	class type_conversion_state
	{
	private:
		std::reference_wrapper<const type_conversion_manager> conversions_;
		std::reference_wrapper<type_conversion_manager::conversion_saves> saves_;

	public:
		type_conversion_state(
				const type_conversion_manager& conversions,
				type_conversion_manager::conversion_saves& saves
				)
			: conversions_{conversions},
			  saves_{saves} {}

		[[nodiscard]] const type_conversion_manager& operator*() const noexcept { return conversions_.get(); }

		[[nodiscard]] const type_conversion_manager* operator->() const noexcept { return &this->operator*(); }

		[[nodiscard]] auto& saves() const noexcept { return saves_.get(); }
	};

	using type_conversion_type = std::shared_ptr<detail::type_conversion_base>;

	/**
	 * @brief Used to register a to parent class relationship with GAL.
	 * Necessary if you want automatic conversions up your inheritance hierarchy.
	 *
	 * @note Create a new to class registration for applying to a module or to the
	 * GAL engine.
	 */
	template<typename Base, typename Derived>
	type_conversion_type register_base()
	{
		// Can only be used with related polymorphic types
		// may be expanded some day to support conversions other than child -> parent
		static_assert(std::is_base_of_v<Base, Derived>, "Classes are not related by inheritance");

		if constexpr (std::is_polymorphic_v<Base> && std::is_polymorphic_v<Derived>) { return std::make_shared<detail::type_conversion_base, detail::dynamic_conversion_impl<Base, Derived>>(); }
		else { return std::make_shared<detail::type_conversion_base, detail::static_conversion_impl<Base, Derived>>(); }
	}

	template<typename Callable>
	type_conversion_type register_convert_function(const utils::gal_type_info& from, const utils::gal_type_info& to, const Callable& function) { return std::make_shared<detail::type_conversion_base, detail::type_conversion_impl<Callable>>(from, to, function); }

	template<typename From, typename To, typename Callable>
	type_conversion_type register_convert_function(const Callable& function)
	{
		return register_convert_function(
				utils::make_type_info<From>(),
				utils::make_type_info<To>(),
				[function](const boxed_value& object) -> boxed_value
				{
					// not even attempting to call boxed_cast so that we don't get caught in some call recursion
					return {function(detail::cast_helper<const From&>::cast(object, nullptr))};
				});
	}

	template<typename From, typename To>
		requires std::is_convertible_v<From, To>
	type_conversion_type register_convert_function()
	{
		return register_convert_function<From, To>(
				[](const boxed_value& object) -> boxed_value
				{
					// not even attempting to call boxed_cast so that we don't get caught in some call recursion
					return {static_cast<To>(detail::cast_helper<From>::cast(object, nullptr))};
				});
	}

	template<
		typename ValueType,
		template<typename> typename Container = std::vector,
		void (Container<ValueType>::*PushFunction)(ValueType&&) = &Container<ValueType>::push_back>
		requires requires(Container<ValueType> container)
		{
			container.reserve(container.size());
			container.PushFunction(std::declval<ValueType&&>());
		}
	type_conversion_type register_container_convert_function()
	{
		return register_convert_function(
				utils::make_type_info<Container<boxed_value>>(),
				utils::make_type_info<ValueType>(),
				[](const boxed_value& object) -> boxed_value
				{
					const auto& source = detail::cast_helper<const Container<boxed_value>&>::cast(object, nullptr);

					Container<ValueType> ret;
					ret.reserve(source.size());
					std::ranges::for_each(
							source,
							[&](ValueType&& value) { ret.PushFunction(std::forward<ValueType&&>(value)); },
							[](const boxed_value& bv) { return detail::cast_helper<ValueType>::cast(bv, nullptr); });
					return {std::move(ret)};
				});
	}

	template<
		typename KeyType,
		typename MappedType,
		template<typename, typename> typename Container = std::map,
		void (Container<KeyType, MappedType>::*PushFunction)(std::pair<KeyType, MappedType>) = &Container<KeyType, MappedType>::insert
	>
		requires requires(Container<KeyType, MappedType> container)
		{
			container.PushFunction(std::declval<std::pair<KeyType, MappedType>>());
		}
	type_conversion_type register_associative_container_convert_function()
	{
		return register_convert_function(
				utils::make_type_info<Container<KeyType, MappedType>>(),
				utils::make_type_info<MappedType>(),
				[](const boxed_value& object) -> boxed_value
				{
					const auto& source = detail::cast_helper<const Container<KeyType, boxed_value>&>::cast(object, nullptr);

					Container<KeyType, MappedType> ret;
					std::ranges::for_each(
							source,
							[&](std::pair<KeyType, MappedType>&& pair) { ret.PushFunction(std::forward<std::pair<KeyType, MappedType>>(pair)); },
							[](const auto& pair) { return std::make_pair(pair.first, detail::cast_helper<MappedType>::cast(pair.second, nullptr)); });
					return {std::move(ret)};
				});
	}

	template<typename T>
	decltype(auto) boxed_cast(const boxed_value& object, const type_conversion_state* conversion = nullptr)
	{
		if (not conversion ||
		    object.type_info().bare_equal(utils::make_type_info<T>()) ||
		    (conversion && not conversion->operator*().is_convertible_type<T>()))
		{
			try { return detail::cast_helper<T>::cast(object, conversion); }
			catch (const std::bad_any_cast&) {}
		}

		if (conversion && conversion->operator*().is_convertible_type<T>())
		{
			try
			{
				// We will not catch any bad_boxed_dynamic_cast that is thrown, let the user get it
				// either way, we are not responsible if it doesn't work
				return detail::cast_helper<T>::cast(conversion->operator*().boxed_type_conversion<T>(conversion->saves(), object), conversion);
			}
			catch (...)
			{
				try
				{
					// try going the other way
					return detail::cast_helper<T>::cast(conversion->operator*().boxed_type_down_conversion<T>(conversion->saves(), object), conversion);
				}
				catch (const std::bad_any_cast&) { throw bad_boxed_cast{object.type_info(), typeid(T)}; }
			}
		}

		// If it's not convertible, just throw the error, don't waste the time on the
		// attempted dynamic_cast
		throw bad_boxed_cast{object.type_info(), typeid(T)};
	}
}

#endif // GAL_LANG_KITS_BOXED_VALUE_CAST_HPP
