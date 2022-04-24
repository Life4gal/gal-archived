#pragma once

#ifndef GAL_LANG_FOUNDATION_BOXED_CAST_HPP
#define GAL_LANG_FOUNDATION_BOXED_CAST_HPP

#include <set>
#include <atomic>
#include <gal/tools/logger.hpp>
#include <gal/foundation/boxed_value.hpp>
#include <gal/foundation/type_info.hpp>
#include <gal/foundation/parameters.hpp>
#include <utils/format.hpp>
#include <utils/thread_storage.hpp>
#include <utils/assert.hpp>

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
			GAL_LANG_TYPE_INFO_DEBUG_DO_OR(const char*, foundation::string_type) what_;

		public:
			// gal_type_info contained in the boxed_value
			foundation::gal_type_info from;
			// std::type_info of the desired (but failed) result type
			const std::type_info* to;

			bad_boxed_cast(
					foundation::gal_type_info from,
					const std::type_info& to,
					const char* const what)
			noexcept
				: what_{
						  GAL_LANG_TYPE_INFO_DEBUG_DO_OR(
								  what,
								  std_format::format("Cast from '{}({})' to '{}', detail: {}.", from.type_name, from.bare_type_name, to.name(), what)
								  )
				  },
				  from{from},
				  to{&to} {}

			bad_boxed_cast(const foundation::gal_type_info from, const std::type_info& to) noexcept
				: bad_boxed_cast{
						from,
						to,
						"Cannot perform boxed_cast"
				} {}

			explicit bad_boxed_cast(const char* const what) noexcept
				: what_{what},
				  to{nullptr} {}

			[[nodiscard]] const char* what() const override { return what_ GAL_LANG_TYPE_INFO_DEBUG_DO(.c_str()); }
		};

		/**
		 * @brief Error thrown when there's a problem with type convertor.
		 */
		class convertor_error final : public bad_boxed_cast
		{
		public:
			foundation::gal_type_info type_to;

			convertor_error(
					const foundation::gal_type_info conversion_from,
					const foundation::gal_type_info conversion_to,
					const char* const what
					)
				: bad_boxed_cast{conversion_from, conversion_to.bare_type_info(), what},
				  type_to{conversion_to} {}

			convertor_error(
					const foundation::gal_type_info conversion_from,
					const foundation::gal_type_info conversion_to)
				: bad_boxed_cast{conversion_from, conversion_to.bare_type_info()},
				  type_to{conversion_to} {}
		};

		// static_cast
		class bad_boxed_static_cast final : public bad_boxed_cast
		{
		public:
			using bad_boxed_cast::bad_boxed_cast;
		};

		// dynamic_cast
		class bad_boxed_dynamic_cast final : public bad_boxed_cast
		{
		public:
			using bad_boxed_cast::bad_boxed_cast;
		};

		// function(from) -> to
		class bad_boxed_explicit_cast final : public bad_boxed_cast
		{
		public:
			using bad_boxed_cast::bad_boxed_cast;
		};
	}// namespace exception

	namespace foundation
	{
		class convertor_manager_state;

		namespace boxed_cast_detail
		{
			template<typename T>
			constexpr T* verify_pointer(T* ptr)
			{
				if (ptr) { return ptr; }
				throw std::runtime_error{"Attempted to dereference a null boxed_value"};
			}

			template<typename T>
			static const T* verify_type(const boxed_value& object, const std::type_info& type, const T* ptr)
			{
				if (object.type_info() == type) { return ptr; }
				throw std::bad_any_cast{};
			}

			template<typename T>
			static T* verify_type(const boxed_value& object, const std::type_info& type, T* ptr)
			{
				if (not object.is_const() && object.type_info() == type) { return ptr; }
				throw std::bad_any_cast{};
			}

			template<typename T>
			static const T* verify_bare_type(const boxed_value& object, const std::type_info& type, const T* ptr)
			{
				if (object.type_info().bare_equal(type)) { return boxed_cast_detail::verify_pointer(ptr); }
				throw std::bad_any_cast{};
			}

			template<typename T>
			static T* verify_bare_type(const boxed_value& object, const std::type_info& type, T* ptr)
			{
				if (not object.is_const() && object.type_info().bare_equal(type)) { return boxed_cast_detail::verify_pointer(ptr); }
				throw std::bad_any_cast{};
			}

			/**
			 * @brief for casting to any type
			 */
			template<typename Result>
			struct cast_helper
			{
				static Result cast(const boxed_value& object, const convertor_manager_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_raw())); }
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
				static Result* cast(const boxed_value& object, const convertor_manager_state*) { return static_cast<Result*>(verify_type(object, typeid(Result), object.get_raw())); }
			};

			/**
			 * @brief for casting to const pointer type
			 */
			template<typename Result>
			struct cast_helper<const Result*>
			{
				static const Result* cast(const boxed_value& object, const convertor_manager_state*) { return static_cast<const Result*>(verify_type(object, typeid(Result), object.get_const_raw())); }
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
				static Result& cast(const boxed_value& object, const convertor_manager_state*) { return *static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_raw())); }
			};

			/**
			 * @brief for casting to const reference type
			 */
			template<typename Result>
			struct cast_helper<const Result&>
			{
				static const Result& cast(const boxed_value& object, const convertor_manager_state*) { return *static_cast<const Result*>(verify_bare_type(object, typeid(Result), object.get_const_raw())); }
			};

			/**
			 * @brief for casting to rvalue-reference type
			 */
			template<typename Result>
			struct cast_helper<Result&&>
			{
				static Result&& cast(const boxed_value& object, const convertor_manager_state*) { return std::move(*static_cast<Result*>(verify_bare_type(object, typeid(Result), object.get_raw()))); }
			};

			/**
			 * @brief for casting to std::unique_ptr reference type
			 * @todo Fix the fact that this has to be in a shared_ptr for now
			 */
			template<typename Result>
			struct cast_helper<std::unique_ptr<Result>&>
			{
				static std::unique_ptr<Result>& cast(const boxed_value& object, const convertor_manager_state*) { return *object.cast<std::shared_ptr<std::unique_ptr<Result>>>(); }
			};

			/**
			 * @brief for casting to std::unique_ptr reference type
			 * @todo Fix the fact that this has to be in a shared_ptr for now
			 */
			template<typename Result>
			struct cast_helper<const std::unique_ptr<Result>&>
			{
				static std::unique_ptr<Result>& cast(const boxed_value& object, const convertor_manager_state*) { return *object.cast<std::shared_ptr<std::unique_ptr<Result>>>(); }
			};

			/**
			 * @brief for casting to std::unique_ptr rvalue-reference type
			 * @todo Fix the fact that this has to be in a shared_ptr for now
			 */
			template<typename Result>
			struct cast_helper<std::unique_ptr<Result>&&>
			{
				static std::unique_ptr<Result>&& cast(const boxed_value& object, const convertor_manager_state*) { return std::move(*object.cast<std::shared_ptr<std::unique_ptr<Result>>>()); }
			};

			/**
			 * @brief for casting to std::shared_ptr type
			 */
			template<typename Result>
			struct cast_helper<std::shared_ptr<Result>>
			{
				static std::shared_ptr<Result> cast(const boxed_value& object, const convertor_manager_state*) { return object.cast<std::shared_ptr<Result>>(); }
			};

			/**
			 * @brief for casting to std::shared_ptr-inner-const type
			 */
			template<typename Result>
			struct cast_helper<std::shared_ptr<const Result>>
			{
				static std::shared_ptr<const Result> cast(const boxed_value& object, const convertor_manager_state*)
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

				static auto cast(const boxed_value& object, const convertor_manager_state*)
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
				static boxed_value cast(const boxed_value& object, const convertor_manager_state*) { return object; }
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
				static std::reference_wrapper<boxed_value> cast(const boxed_value& object, const convertor_manager_state*) { return std::ref(const_cast<boxed_value&>(object)); }
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
				static decltype(auto) cast(const boxed_value& object, const convertor_manager_state* conversion) { return cast_helper<T>::cast(object, conversion); }
			};

			template<typename T>
			struct cast_invoker
			{
				static decltype(auto) cast(const boxed_value& object, const convertor_manager_state* conversion) { return default_cast_invoker<T>::cast(object, conversion); }
			};

			class convertor_base
			{
			public:
				GAL_LANG_TYPE_INFO_DEBUG_DO(
						string_type conversion_detail;
						)

			private:
				gal_type_info from_;
				gal_type_info to_;

			protected:
				convertor_base(const gal_type_info from, const gal_type_info to)
					: GAL_LANG_TYPE_INFO_DEBUG_DO(
							conversion_detail{std_format::format("convert from '{}({})' to '{}({})'", from.name(), from.bare_name(), to.name(), to.bare_name())},
							)
					  from_{from},
					  to_{to} { }

			public:
				convertor_base(const convertor_base&) = default;
				convertor_base& operator=(const convertor_base&) = default;
				convertor_base(convertor_base&&) = default;
				convertor_base& operator=(convertor_base&&) = default;
				virtual ~convertor_base() = default;

				[[nodiscard]] constexpr virtual bool is_bidirectional_convert() const noexcept { return true; }

				[[nodiscard]] virtual boxed_value convert(const boxed_value& from) const = 0;
				[[nodiscard]] virtual boxed_value convert_down(const boxed_value& to) const = 0;

				[[nodiscard]] const gal_type_info& from() const noexcept { return from_; }
				[[nodiscard]] const gal_type_info& to() const noexcept { return to_; }
			};

			template<bool IsStatic, typename From, typename To>
			struct conversion_invoker
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
			class static_convertor final : public convertor_base
			{
			public:
				static_convertor()
					: convertor_base{
							make_type_info<Derived>(),
							make_type_info<Base>()
					} {}

				[[nodiscard]] constexpr bool is_bidirectional_convert() const noexcept override { return false; }

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return conversion_invoker<true, Derived, Base>::cast(from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw exception::bad_boxed_static_cast{to.type_info(), typeid(Derived), "Unable to cast down inheritance hierarchy with non-polymorphic types"}; }
			};

			template<typename Base, typename Derived>
			class dynamic_convertor final : public convertor_base
			{
			public:
				dynamic_convertor()
					: convertor_base
					{
							make_type_info<Derived>(),
							make_type_info<Base>()
					} { }

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return conversion_invoker<true, Derived, Base>::cast(from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { return conversion_invoker<false, Base, Derived>::cast(to); }
			};

			template<typename Callable>
				requires std::is_invocable_r_v<boxed_value, Callable, const boxed_value&>
			class explicit_convertor final : public convertor_base
			{
			public:
				using function_type = Callable;

			private:
				function_type function_;

			public:
				explicit_convertor(
						const gal_type_info from,
						const gal_type_info to,
						function_type function
						)
					: convertor_base{from, to},
					  function_{std::move(function)} {}

				[[nodiscard]] boxed_value convert(const boxed_value& from) const override { return std::invoke(function_, from); }

				[[nodiscard]] boxed_value convert_down(const boxed_value& to) const override { throw exception::bad_boxed_explicit_cast{"No conversion exists"}; }
			};
		}

		using convertor_type = std::shared_ptr<boxed_cast_detail::convertor_base>;

		template<typename T, typename... Args>
			requires std::is_base_of_v<boxed_cast_detail::convertor_base, T>
		[[nodiscard]] convertor_type make_convertor(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

		class convertor_manager
		{
		public:
			struct conversion_saves
			{
				bool enable = false;
				parameters_type saves;
			};

			struct type_info_comparator
			{
				[[nodiscard]] bool operator()(const std::type_info& lhs, const std::type_info& rhs) const noexcept { return lhs.before(rhs); }
			};

			using convertors_type = std::set<convertor_type>;
			using convertible_types_type = std::set<std::reference_wrapper<const std::type_info>, type_info_comparator>;

		private:
			mutable utils::threading::shared_mutex mutex_;
			mutable utils::thread_storage<convertible_types_type> thread_cache_;
			mutable utils::thread_storage<conversion_saves> conversion_saves_;

			convertors_type convertors_;
			convertible_types_type convertibles_;
			std::atomic<convertible_types_type::size_type> type_size_;

			[[nodiscard]] auto find_bidirectional_convertor(const gal_type_info& from, const gal_type_info& to) const
			{
				return std::ranges::find_if(
						convertors_,
						[&from, &to](const auto& convertor)
						{
							return (
								       convertor->to().bare_equal(to) &&
								       convertor->from().bare_equal(from)
							       ) ||
							       (
								       convertor->is_bidirectional_convert() &&
								       convertor->from().bare_equal(to) &&
								       convertor->to().bare_equal(from)
							       );
						});
			}

			[[nodiscard]] auto find_bidirectional_convertor(const convertor_type& convertor) const { return find_bidirectional_convertor(convertor->from(), convertor->to()); }

			[[nodiscard]] auto find_convertor(const gal_type_info& from, const gal_type_info& to) const
			{
				return std::ranges::find_if(
						convertors_,
						[&from, &to](const auto& convertor) { return convertor->to().bare_equal(to) && convertor->from().bare_equal(from); });
			}

			[[nodiscard]] auto find_convertor(const convertor_type& convertor) const { return find_convertor(convertor->from(), convertor->to()); }

			[[nodiscard]] bool is_bidirectional_convertible(const gal_type_info& from, const gal_type_info& to) const
			{
				utils::threading::shared_lock lock{mutex_};
				return find_bidirectional_convertor(from, to) != convertors_.end();
			}

		public:
			convertor_manager()
				: type_size_{0} {}

			convertor_manager(const convertor_manager&) = delete;
			convertor_manager& operator=(const convertor_manager&) = delete;
			convertor_manager(convertor_manager&&) = delete;
			convertor_manager& operator=(convertor_manager&&) = delete;
			~convertor_manager() = default;

			[[nodiscard]] const auto& cached_convertible_types() const
			{
				auto& cache = *thread_cache_;

				if (cache.size() != type_size_)
				{
					utils::threading::shared_lock lock{mutex_};
					cache = convertibles_;
				}

				return cache;
			}

			void add_convertor(
					const convertor_type& convertor
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()
							)
					)
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							find_bidirectional_convertor(convertor) != convertors_.end() ? "but it was already exist" : "add successed");)

				utils::threading::shared_lock lock{mutex_};

				if (find_bidirectional_convertor(convertor) != convertors_.end()) { throw exception::convertor_error{convertor->from(), convertor->to(), "Trying to re-insert an existing conversion"}; }

				[[maybe_unused]] const auto result = convertors_.insert(convertor).second;
				gal_assert(result);
				convertibles_.insert({convertor->to().bare_type_info(), convertor->from().bare_type_info()});
				type_size_ = convertibles_.size();
			}

			template<typename T>
			[[nodiscard]] bool is_convertible() const noexcept { return thread_cache_->contains(make_type_info<T>().bare_type_info()); }

			[[nodiscard]] bool is_convertible(const gal_type_info& from, const gal_type_info& to) const
			{
				if (const auto& cache = cached_convertible_types();
					cache.contains(to.bare_type_info()) && cache.contains(from.bare_type_info())) { return is_bidirectional_convertible(from, to); }
				return false;
			}

			template<typename From, typename To>
			[[nodiscard]] bool is_convertible() const { return convertor_manager::is_convertible(make_type_info<From>(), make_type_info<To>()); }

			/**
			 * @throw std::out_of_range No such convertor exists that support conversion from 'from' to 'to'.
			 */
			[[nodiscard]] convertor_type get_convertor(const gal_type_info& from, const gal_type_info& to) const
			{
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = find_convertor(from, to);
					it != convertors_.end()) { return *it; }

				throw std::out_of_range{
						std_format::format(
								"No such convertor exists that support conversion from {} to {}",
								from.bare_name(),
								to.bare_name())};
			}

			/**
			 * @throw exception::bad_boxed_cast No known conversion
			 * @throw exception::bad_boxed_cast Unable to perform cast operation
			 */
			[[nodiscard]] boxed_value boxed_convert(
					const boxed_value& from,
					const gal_type_info& to
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()
							)
					) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				try
				{
					auto ret = get_convertor(from.type_info(), to)->convert(from);
					if (auto& [enable, saves] = *conversion_saves_;
						enable) { saves.push_back(ret); }
					return ret;
				}
				catch (const std::out_of_range&) { throw exception::bad_boxed_cast{from.type_info(), to.bare_type_info(), "No known conversion"}; }
				catch (const std::bad_cast&) { throw exception::bad_boxed_cast{from.type_info(), to.bare_type_info(), "Unable to perform cast operation"}; }
			}

			/**
			 * @throw exception::bad_boxed_cast No known conversion
			 * @throw exception::bad_boxed_cast Unable to perform cast operation
			 */
			template<typename To>
			[[nodiscard]] boxed_value boxed_convert(
					const boxed_value& from
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				return this->boxed_convert(
						from,
						make_type_info<To>()
						GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location)
						);
			}

			/**
			 * @throw exception::bad_boxed_cast No known conversion
			 * @throw exception::bad_boxed_cast Unable to perform cast operation
			 */
			[[nodiscard]] boxed_value boxed_convert_down(
					const gal_type_info& from,
					const boxed_value& to
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				try
				{
					auto ret = get_convertor(from, to.type_info())->convert_down(to);
					if (auto& [enable, saves] = *conversion_saves_;
						enable) { saves.push_back(ret); }
					return ret;
				}
				catch (const std::out_of_range&) { throw exception::bad_boxed_cast{to.type_info(), from.bare_type_info(), "No known conversion"}; }
				catch (const std::bad_cast&) { throw exception::bad_boxed_cast{to.type_info(), from.bare_type_info(), "Unable to perform cast operation"}; }
			}

			/**
			 * @throw exception::bad_boxed_cast No known conversion
			 * @throw exception::bad_boxed_cast Unable to perform cast operation
			 */
			template<typename From>
			[[nodiscard]] boxed_value boxed_convert_down(
					const boxed_value& to
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				return this->boxed_convert_down(
						make_type_info<From>(),
						to
						GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location));
			}

			void enable_conversion_saves(
					const bool enable
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				conversion_saves_->enable = enable;
			}

			[[nodiscard]] conversion_saves exchange_conversion_saves(
					const conversion_saves& new_value = {} GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						tools::logger::info("{} from (file: '{}' function: '{}' position: ({}:{}))",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column());)

				return std::exchange(*conversion_saves_, new_value);
			}
		};

		class convertor_manager_state
		{
		public:
			using manager_type = std::reference_wrapper<const convertor_manager>;

		private:
			manager_type manager_;

		public:
			explicit convertor_manager_state(const convertor_manager& manager)
				: manager_{manager} {}

			[[nodiscard]] const convertor_manager& operator*() const noexcept { return manager_.get(); }

			[[nodiscard]] const convertor_manager* operator->() const noexcept { return &manager_.get(); }
		};
	}
}

#endif // GAL_LANG_FOUNDATION_BOXED_CAST_HPP
