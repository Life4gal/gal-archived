#pragma once

#ifndef GAL_LANG_FOUNDATION_BOXED_CAST_HPP
#define GAL_LANG_FOUNDATION_BOXED_CAST_HPP

#include <atomic>
#include <utils/logger.hpp>
#include <gal/foundation/boxed_value.hpp>
#include <gal/foundation/type_info.hpp>
#include <set>
#include <utils/format.hpp>
#include <utils/thread_storage.hpp>

namespace gal::lang
{
	namespace exception
	{
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
			foundation::gal_type_info from;
			// std::type_info of the desired (but failed) result type
			const std::type_info* to;

			bad_boxed_cast(foundation::gal_type_info from, const std::type_info& to, const char* const what) noexcept
				: what_{what},
				  from{from},
				  to{&to} {}

			bad_boxed_cast(const foundation::gal_type_info from, const std::type_info& to) noexcept
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
			foundation::gal_type_info type_to;

			conversion_error(const foundation::gal_type_info t, const foundation::gal_type_info f, const char* const what) noexcept
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
	}// namespace exception

	namespace foundation
	{
		class type_conversion_state;

		namespace boxed_cast_detail
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
				static Result cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_raw())); }
			};

			/**
		 * @brief for casting to any const type
		 */
			template<typename Result>
			struct cast_helper<const Result> : cast_helper<Result> { };

			/**
		 * @brief for casting to pointer type
		 */
			template<typename Result>
			struct cast_helper<Result*>
			{
				static Result* cast(const boxed_value& object, const type_conversion_state*) { return static_cast<Result*>(verify_type(object, typeid(Result), object.get_raw())); }
			};

			/**
		 * @brief for casting to const pointer type
		 */
			template<typename Result>
			struct cast_helper<const Result*>
			{
				static const Result* cast(const boxed_value& object, const type_conversion_state*) { return static_cast<const Result*>(verify_type(object, typeid(Result), object.get_const_raw())); }
			};

			/**
		 * @brief for casting to pointer type
		 */
			template<typename Result>
			struct cast_helper<Result* const&> : cast_helper<Result*> { };

			/**
		 * @brief for casting to const pointer type
		 */
			template<typename Result>
			struct cast_helper<const Result* const&> : cast_helper<const Result*> { };

			/**
		 * @brief for casting to reference type
		 */
			template<typename Result>
			struct cast_helper<Result&>
			{
				static Result& cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_raw())); }
			};

			/**
		 * @brief for casting to const reference type
		 */
			template<typename Result>
			struct cast_helper<const Result&>
			{
				static const Result& cast(const boxed_value& object, const type_conversion_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_raw())); }
			};

			/**
		 * @brief for casting to rvalue-reference type
		 */
			template<typename Result>
			struct cast_helper<Result&&>
			{
				static Result&& cast(const boxed_value& object, const type_conversion_state*) { return std::move(*static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_raw()))); }
			};

			/**
		 * @brief for casting to std::unique_ptr reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
			template<typename Result>
			struct cast_helper<std::unique_ptr<Result>&>
			{
				static std::unique_ptr<Result>& cast(const boxed_value& object, const type_conversion_state*) { return *object.cast<std::shared_ptr<std::unique_ptr<Result>>>(); }
			};

			/**
		 * @brief for casting to std::unique_ptr reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
			template<typename Result>
			struct cast_helper<const std::unique_ptr<Result>&>
			{
				static std::unique_ptr<Result>& cast(const boxed_value& object, const type_conversion_state*) { return *object.cast<std::shared_ptr<std::unique_ptr<Result>>>(); }
			};

			/**
		 * @brief for casting to std::unique_ptr rvalue-reference type
		 * @todo Fix the fact that this has to be in a shared_ptr for now
		 */
			template<typename Result>
			struct cast_helper<std::unique_ptr<Result>&&>
			{
				static std::unique_ptr<Result>&& cast(const boxed_value& object, const type_conversion_state*) { return std::move(*object.cast<std::shared_ptr<std::unique_ptr<Result>>>()); }
			};

			/**
		 * @brief for casting to std::shared_ptr type
		 */
			template<typename Result>
			struct cast_helper<std::shared_ptr<Result>>
			{
				static std::shared_ptr<Result> cast(const boxed_value& object, const type_conversion_state*) { return object.cast<std::shared_ptr<Result>>(); }
			};

			/**
		 * @brief for casting to std::shared_ptr-inner-const type
		 */
			template<typename Result>
			struct cast_helper<std::shared_ptr<const Result>>
			{
				static std::shared_ptr<const Result> cast(const boxed_value& object, const type_conversion_state*)
				{
					if (not object.type_info().is_const()) { return std::const_pointer_cast<const Result>(object.cast<std::shared_ptr<Result>>()); }
					return object.cast<std::shared_ptr<const Result>>();
				}
			};

			/**
		 * @brief for casting to const std::shared_ptr type
		 */
			template<typename Result>
			struct cast_helper<const std::shared_ptr<Result>> : cast_helper<std::shared_ptr<Result>> { };

			/**
		 * @brief for casting to std::shared_ptr reference type
		 */
			template<typename Result>
			struct cast_helper<std::shared_ptr<Result>&>
			{
				static_assert(not std::is_const_v<Result>, "Non-const reference to std::shared_ptr<const T> is not supported");

				static auto cast(const boxed_value& object, const type_conversion_state*)
				{
					// the compiler refuses to accept such a cast
					// auto& result = std::any_cast<std::shared_ptr<Result>&>(object.get());
					// null pointer error, smart pointer has been deleted
					// auto& result = *std::any_cast<std::shared_ptr<Result>>(&const_cast<std::add_lvalue_reference_t<std::remove_cvref_t<decltype(object.get())>>>(object.get()));
					// internal data should not be accessed directly
					// auto& result = std::any_cast<std::shared_ptr<Result>&>(const_cast<boxed_value&>(object).get());

					auto& result = object.cast<std::shared_ptr<Result>&>();
					return object.pointer_sentinel(result);
				}
			};

			/**
		 * @brief for casting to const std::shared_ptr reference type
		 */
			template<typename Result>
			struct cast_helper<const std::shared_ptr<Result>&> : cast_helper<std::shared_ptr<Result>> { };

			/**
		 * @brief for casting to const std::shared_ptr-inner-const type
		 */
			template<typename Result>
			struct cast_helper<const std::shared_ptr<const Result>> : cast_helper<std::shared_ptr<const Result>> { };

			/**
		 * @brief for casting to const std::shared_ptr-inner-const reference type
		 */
			template<typename Result>
			struct cast_helper<const std::shared_ptr<const Result>&> : cast_helper<std::shared_ptr<const Result>> { };

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
			struct cast_helper<const boxed_value> : cast_helper<boxed_value> { };

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
			struct cast_helper<const boxed_value&> : cast_helper<boxed_value> { };

			/**
		 * @brief for casting to std::reference_wrapper type
		 */
			template<typename Result>
			struct cast_helper<std::reference_wrapper<Result>> : cast_helper<Result&> { };

			/**
		 * @brief for casting to const std::reference_wrapper type
		 */
			template<typename Result>
			struct cast_helper<const std::reference_wrapper<Result>> : cast_helper<Result&> { };

			/**
		 * @brief for casting to const std::reference_wrapper reference type
		 */
			template<typename Result>
			struct cast_helper<const std::reference_wrapper<Result>&> : cast_helper<Result&> { };

			/**
		 * @brief for casting to std::reference_wrapper-inner-const type
		 */
			template<typename Result>
			struct cast_helper<std::reference_wrapper<const Result>> : cast_helper<const Result&> { };

			/**
		 * @brief for casting to const std::reference_wrapper-inner-const type
		 */
			template<typename Result>
			struct cast_helper<const std::reference_wrapper<const Result>> : cast_helper<const Result&> { };

			/**
		 * @brief for casting to const std::reference_wrapper-inner-const reference type
		 */
			template<typename Result>
			struct cast_helper<const std::reference_wrapper<const Result>&> : cast_helper<const Result&> { };

			/**
		 * @note internal use only
		 */
			template<typename T>
			struct default_cast_invoker
			{
				/**
			 * @note default conversion method
			 */
				static decltype(auto) cast(const boxed_value& object, const type_conversion_state* conversion) { return cast_helper<T>::cast(object, conversion); }
			};

			template<typename T>
			struct cast_invoker
			{
				static decltype(auto) cast(const boxed_value& object, const type_conversion_state* conversion) { return default_cast_invoker<T>::cast(object, conversion); }
			};

			class type_conversion_base
			{
			private:
				gal_type_info to_;
				gal_type_info from_;

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
					if (from.type_info().bare_equal(make_type_info<From>()))
					{
						if (from.is_pointer())
						{
							// static/dynamic cast out the contained boxed value, which we know is the type we want
							auto do_cast = [&]<typename T, typename F>()
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
							};

							if (from.is_const()) { return boxed_value{do_cast.decltype(do_cast)::template operator()<const To, const From>()}; }
							return boxed_value{do_cast.decltype(do_cast)::template operator()<To, From>()};
						}
						// Pull the reference out of the contained boxed value, which we know is the type we want
						auto do_cast = [&]<typename T, typename F>() -> std::add_lvalue_reference_t<T>
						{
							auto& f = cast_helper<std::add_lvalue_reference_t<F>>::cast(from, nullptr);

							if constexpr (IsStatic) { return static_cast<std::add_lvalue_reference_t<T>>(f); }
							else { return dynamic_cast<std::add_lvalue_reference_t<T>>(f); }
						};

						if (from.is_const()) { return boxed_value{std::cref(do_cast.decltype(do_cast)::template operator()<const To, const From>())}; }
						return boxed_value{std::ref(do_cast.decltype(do_cast)::template operator()<To, From>())};
					}

					if constexpr (IsStatic) { throw exception::bad_boxed_static_cast(from.type_info(), typeid(To), "Unknown static_cast_conversion"); }
					else { throw exception::bad_boxed_dynamic_cast(from.type_info(), typeid(To), "Unknown dynamic_cast_conversion"); }
				}
			};

			template<typename Base, typename Derived>
			class static_conversion final : public type_conversion_base
			{
			public:
				static_conversion()
					: type_conversion_base{make_type_info<Base>(), make_type_info<Derived>()} {}

				[[nodiscard]] constexpr bool is_bidirectional() const noexcept override { return false; }

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return caster<true, Derived, Base>::cast(from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw exception::bad_boxed_static_cast{to.type_info(), typeid(Derived), "Unable to cast down inheritance hierarchy with non-polymorphic types"}; }
			};

			template<typename Base, typename Derived>
			class dynamic_conversion final : public type_conversion_base
			{
			public:
				dynamic_conversion()
					: type_conversion_base{make_type_info<Base>(), make_type_info<Derived>()} {}

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return caster<true, Derived, Base>::cast(from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { return caster<false, Base, Derived>::cast(to); }
			};

			template<typename Callable>
			class type_conversion final : public type_conversion_base
			{
			public:
				using function_type = Callable;

			private:
				function_type function_;

			public:
				type_conversion(const gal_type_info from, const gal_type_info to, function_type function)
					: type_conversion_base{to, from},
					  function_{std::move(function)} {}

				[[nodiscard]] constexpr bool is_bidirectional() const noexcept override { return false; }

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return function_(from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw exception::bad_boxed_type_cast{"No conversion exists"}; }
			};
		}// namespace boxed_cast_detail

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

			using conversion_type = std::shared_ptr<boxed_cast_detail::type_conversion_base>;
			using convertible_types_type = std::set<std::reference_wrapper<const std::type_info>, type_info_compare>;

		private:
			mutable utils::threading::shared_mutex mutex_;
			mutable utils::thread_storage<convertible_types_type> thread_cache_;
			mutable utils::thread_storage<conversion_saves> conversion_saves_;

			std::set<conversion_type> conversions_;
			convertible_types_type convertible_types_;
			std::atomic_size_t num_types_;

			auto bidirectional_find(const gal_type_info& to, const gal_type_info& from) const
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

			auto find(const gal_type_info& to, const gal_type_info& from) const
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

			void add(
					const conversion_type& conversion
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std_source_location& location = std_source_location::current())
					)
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							bidirectional_find(conversion) != conversions_.end() ? "but it was already exist" : "add successed");)

				utils::threading::shared_lock lock(mutex_);

				if (bidirectional_find(conversion) != conversions_.end()) { throw exception::conversion_error{conversion->to(), conversion->from(), "Trying to re-insert an existing conversion"}; }

				conversions_.insert(conversion);
				convertible_types_.insert({conversion->to().bare_type_info(), conversion->from().bare_type_info()});
				num_types_ = convertible_types_.size();
			}

			[[nodiscard]] bool has_conversion(const gal_type_info& to, const gal_type_info& from) const
			{
				utils::threading::shared_lock lock{mutex_};
				return bidirectional_find(to, from) != conversions_.end();
			}

			template<typename T>
			[[nodiscard]] bool is_convertible_type() const noexcept { return thread_cache_->contains(make_type_info<T>().bare_type_info()); }

			[[nodiscard]] bool is_convertible_type(const gal_type_info& to, const gal_type_info& from) const noexcept
			{
				if (const auto& cache = get_cache();
					cache.contains(to.bare_type_info()) && cache.contains(from.bare_type_info())) { return has_conversion(to, from); }
				return false;
			}

			template<typename To, typename From>
			[[nodiscard]] bool is_convertible_type() const noexcept { return is_convertible_type(make_type_info<To>(), make_type_info<From>()); }

			conversion_type get_conversion(const gal_type_info& to, const gal_type_info& from) const
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

			[[nodiscard]] boxed_value boxed_type_conversion(
					const gal_type_info& to,
					conversion_saves& saves,
					const boxed_value& from
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				try
				{
					auto ret = get_conversion(to, from.type_info())->convert(from);
					if (saves.enable) { saves.saves.push_back(ret); }
					return ret;
				}
				catch (const std::out_of_range&) { throw exception::bad_boxed_dynamic_cast{from.type_info(), to.bare_type_info(), "No known conversion"}; }
				catch (const std::bad_cast&) { throw exception::bad_boxed_dynamic_cast{from.type_info(), to.bare_type_info(), "Unable to perform dynamic_cast operation"}; }
			}

			template<typename To>
			[[nodiscard]] boxed_value boxed_type_conversion(
					conversion_saves& saves,
					const boxed_value& from
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std_source_location& location = std_source_location::current())
					) const { return type_conversion_manager::boxed_type_conversion(make_type_info<To>(), saves, from GAL_UTILS_DO_IF_DEBUG(, location)); }

			[[nodiscard]] boxed_value boxed_type_down_conversion(
					const gal_type_info& from,
					conversion_saves& saves,
					const boxed_value& to
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				try
				{
					auto ret = get_conversion(to.type_info(), from)->convert_down(to);
					if (saves.enable) { saves.saves.push_back(ret); }
					return ret;
				}
				catch (const std::out_of_range&) { throw exception::bad_boxed_dynamic_cast{to.type_info(), from.bare_type_info(), "No known conversion"}; }
				catch (const std::bad_cast&) { throw exception::bad_boxed_dynamic_cast{to.type_info(), from.bare_type_info(), "Unable to perform dynamic_cast operation"}; }
			}

			template<typename From>
			[[nodiscard]] boxed_value boxed_type_down_conversion(
					conversion_saves& saves,
					const boxed_value& to
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std_source_location& location = std_source_location::current())
					) const { return type_conversion_manager::boxed_type_down_conversion(make_type_info<From>(), saves, to GAL_UTILS_DO_IF_DEBUG(, location)); }

			static void enable_conversion_saves(
					conversion_saves& saves,
					const bool enable
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current())
					) noexcept
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})) because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				saves.enable = enable;
			}

			[[nodiscard]] static auto take_conversion_saves(
					conversion_saves& saves
					GAL_UTILS_DO_IF_LOG_INFO(
							,
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current())
					) noexcept
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})) because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				decltype(conversion_saves::saves) dummy;
				std::swap(dummy, saves.saves);
				return dummy;
			}

			[[nodiscard]] conversion_saves& get_conversion_saves(
					GAL_UTILS_DO_IF_LOG_INFO(
							const std_source_location& location = std_source_location::current())
					) const noexcept
			{
				GAL_UTILS_DO_IF_LOG_INFO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				return *conversion_saves_;
			}
		};

		class type_conversion_state
		{
		private:
			std::reference_wrapper<const type_conversion_manager> conversions_;

		public:
			explicit type_conversion_state(
					const type_conversion_manager& conversions)
				: conversions_{conversions} { }

			[[nodiscard]] const type_conversion_manager& operator*() const noexcept { return conversions_.get(); }

			[[nodiscard]] const type_conversion_manager* operator->() const noexcept { return &conversions_.get(); }

			[[nodiscard]] auto& saves() const noexcept { return this->operator*().get_conversion_saves(); }
		};

		using type_conversion_type = std::shared_ptr<boxed_cast_detail::type_conversion_base>;

		template<typename T, typename... Args>
			requires std::is_base_of_v<boxed_cast_detail::type_conversion_base, T>
		[[nodiscard]] type_conversion_type make_type_conversion(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }
	}// namespace foundation
}    // namespace gal::lang::foundation

#endif// GAL_LANG_FOUNDATION_BOXED_CAST_HPP
