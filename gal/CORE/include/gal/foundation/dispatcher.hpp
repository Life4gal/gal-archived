#pragma once

#ifndef GAL_LANG_FOUNDATION_DISPATCHER_HPP
	#define GAL_LANG_FOUNDATION_DISPATCHER_HPP

	#include <gal/boxed_cast.hpp>
	#include <gal/boxed_value.hpp>
	#include <gal/foundation/dynamic_object.hpp>
	#include <gal/foundation/proxy_function.hpp>
	#include <gal/language/name.hpp>
	#include <utils/enum_utils.hpp>
	#include <utils/format.hpp>
	#include <utils/scoped_object.hpp>
	#include <map>
	#include <array>
	#include <set>

namespace gal::lang::foundation
{
	namespace exception
	{
		/**
		 * @brief Exception thrown in the case that an object name is invalid because it is a reserved word.
		 */
		class reserved_word_error final : public std::runtime_error
		{
		public:
			using word_type = string_view_type;

		private:
			word_type word_;

		public:
			explicit reserved_word_error(word_type word)
				: std::runtime_error{
						  std_format::format("'{}' is a reserved word and not allowed in object name", word)},
				  word_{word} {}

			[[nodiscard]] word_type which() const noexcept { return word_; }
		};

		/**
		 * @brief Exception thrown in the case that an object name is invalid because it contains illegal characters.
		 */
		class illegal_name_error final : public std::runtime_error
		{
		public:
			using name_type = string_type;

		private:
			name_type name_;

		public:
			explicit illegal_name_error(name_type&& name)
				: std::runtime_error{
						  std_format::format("'{}' is a reserved name and not allowed in object name", name)},
				  name_{std::move(name)} {}

			[[nodiscard]] const name_type& which() const noexcept { return name_; }
		};

		/**
		 * @brief Exception thrown in the case that an object name is invalid because it already exists in current context.
		 */
		class name_conflict_error final : public std::runtime_error
		{
		public:
			using name_type = string_type;

		private:
			name_type name_;

		public:
			explicit name_conflict_error(name_type name)
				: std::runtime_error{
						  std_format::format("'{}' is already defined in the current context", name)},
				  name_{std::move(name)} {}

			explicit name_conflict_error(const string_view_type name)
				: name_conflict_error{name_type{name}} {}

			[[nodiscard]] const name_type& which() const noexcept { return name_; }
		};

		class global_mutable_error final : public std::runtime_error
		{
		public:
			using name_type = string_type;

		private:
			name_type name_;

		public:
			explicit global_mutable_error(name_type name)
				: std::runtime_error{
						  std_format::format("global variable '{}' must be immutable", name)},
				  name_{std::move(name)} {}

			explicit global_mutable_error(const string_view_type name)
				: global_mutable_error{name_type{name}} {}

			[[nodiscard]] const name_type& which() const noexcept { return name_; }
		};
	}// namespace exception

	class parser_base;

	/**
	 * @brief Holds a collection of settings which can be applied to the runtime.
	 * @note Used to implement loadable module support.
	 */
	class engine_core
	{
	public:
		// todo: the function of storing strings should be handed over to string_pool
		using name_type				= string_type;
		using name_view_type		= string_view_type;

		using function_type			= proxy_function;
		using variable_type			= boxed_value;
		using evaluation_type		= string_type;

		using type_infos_type		= std::map<name_type, gal_type_info, std::less<>>;
		using functions_type		= std::map<name_type, function_type, std::less<>>;
		using variables_type		= std::map<name_type, variable_type, std::less<>>;
		using evaluations_type		= std::set<evaluation_type, std::less<>>;
		using type_conversions_type = std::set<type_conversion_type, std::less<>>;

	private:
		type_infos_type		  types_;
		functions_type		  functions_;
		variables_type		  variables_;
		evaluations_type	  evaluations_;
		type_conversions_type type_conversions_;

		template<typename Engine>
		void apply_type_info(Engine& engine)
		{
			std::ranges::for_each(
					types_,
					[&engine](const auto& type)
					{
						try
						{
							engine.add_type_info(type.first, type.second);
						}
						catch (const exception::name_conflict_error&)
						{
							// todo: Should we throw an error if there's a name conflict while applying a module?
						}
					});
		}

		template<typename Engine>
		void apply_function(Engine& engine)
		{
			std::ranges::for_each(
					functions_,
					[&engine](const auto& function)
					{
						try
						{
							engine.add_function(function.first, function.second);
						}
						catch (const exception::name_conflict_error&)
						{
							// todo: Should we throw an error if there's a name conflict while applying a module?
						}
					});
		}

		template<typename Engine>
		void apply_variable(Engine& engine)
		{
			std::ranges::for_each(
					variables_,
					[&engine](const auto& variable)
					{
						engine.add_global(variable.first, variable.second);
					});
		}

		template<typename Eval>
		void apply_evaluation(Eval& eval)
		{
			std::ranges::for_each(
					evaluations_,
					[&eval](const auto& evaluation)
					{
						eval.add_evaluation(evaluation);
					});
		}

		template<typename Engine>
		void apply_type_conversion(Engine& engine)
		{
			std::ranges::for_each(
					type_conversions_,
					[&engine](const auto& conversion)
					{
						engine.add_type_conversion(conversion);
					});
		}

	public:
		engine_core& add_type_info(name_type name, gal_type_info type)
		{
			gal_assert(types_.emplace(std::move(name), type).second);
			return *this;
		}

		engine_core& add_function(name_type name, function_type function)
		{
			gal_assert(functions_.emplace(std::move(name), std::move(function)).second);
			return *this;
		}

		engine_core& add_variable(name_type name, variable_type variable)
		{
			if (not variable.is_const())
			{
				throw exception::global_mutable_error{name};
			}

			gal_assert(variables_.emplace(std::move(name), std::move(variable)).second);
			return *this;
		}

		engine_core& add_evaluation(evaluation_type evaluation)
		{
			gal_assert(evaluations_.emplace(std::move(evaluation)).second);
			return *this;
		}

		engine_core& add_type_conversion(type_conversion_type conversion)
		{
			gal_assert(type_conversions_.emplace(std::move(conversion)).second);
			return *this;
		}

		template<typename Eval, typename Engine>
		void apply(Eval& eval, Engine& engine)
		{
			apply_type_info(engine);
			apply_function(engine);
			apply_variable(engine);
			apply_evaluation(eval);
			apply_type_conversion(engine);
		}

		[[nodiscard]] bool has_function(const string_view_type name, const function_type& function) const noexcept
		{
			if (const auto it = functions_.find(name);
				it != functions_.end() && *it->second == *function)
			{
				return true;
			}

			return false;
		}
	};

	using shared_engine_core = std::shared_ptr<engine_core>;

	namespace dispatcher_detail
	{
		/**
		 * @brief A proxy_function implementation that is able to take
		 * a vector of proxy_functions and perform a dispatch on them.
		 * It is used specifically in the case of dealing with function object variables.
		 */
		class dispatch_function final : public proxy_function_base
		{
		public:
			using function_type	 = mutable_proxy_function;
			using functions_type = mutable_proxy_functions_type;

		private:
			functions_type		   functions_;

			static type_infos_type build_param_types(const functions_type& functions)
			{
				if (functions.empty())
				{
					return {};
				}

				auto copy_types	   = functions.front()->types();
				bool size_mismatch = false;

				for (auto begin = functions.begin() + 1; begin != functions.end(); ++begin)
				{
					const auto& param_types = (*begin)->types();

					if (param_types.size() != copy_types.size())
					{
						size_mismatch = true;
					}

					const auto size = static_cast<decltype(copy_types)::difference_type>(std::ranges::min(copy_types.size(), param_types.size()));
					std::transform(
							copy_types.begin(),
							copy_types.begin() + size,
							param_types.begin(),
							copy_types.begin(),
							[](auto&& lhs, auto&& rhs)
							{
								if (lhs != rhs)
								{
									return make_type_info<boxed_value>();
								}
								return lhs;
							});
				}

				gal_assert(not copy_types.empty(), "type_info vector is empty, this is only possible if something else is broken");

				if (size_mismatch)
				{
					copy_types.resize(1);
				}

				return copy_types;
			}

			[[nodiscard]] boxed_value do_invoke(parameters_view_type params, const type_conversion_state& conversion) const override
			{
				return dispatch(functions_, params, conversion);
			}

		public:
			static arity_size_type calculate_arity(const functions_type& functions) noexcept
			{
				if (functions.empty() || std::ranges::any_of(
												 functions.begin() + 1,
												 functions.end(),
												 [arity = functions.front()->get_arity()](const auto a)
												 {
													 return a != arity;
												 },
												 [](const auto& function)
												 { return function->get_arity(); }))
				{
					return no_parameters_arity;
				}

				return functions.front()->get_arity();
			}

			explicit dispatch_function(functions_type&& functions)
				: proxy_function_base{
						  build_param_types(functions),
						  calculate_arity(functions)},
				  functions_{std::move(functions)} {}

			[[nodiscard]] immutable_proxy_functions_type container_functions() const override
			{
				return {functions_.begin(), functions_.end()};
			}

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
			{
				const auto* d = dynamic_cast<const dispatch_function*>(&other);
				return d && d->functions_ == functions_;
			}

			[[nodiscard]] bool match(parameters_view_type params, const type_conversion_state& conversion) const override
			{
				return std::ranges::any_of(
						functions_,
						[params, &conversion](const auto& function)
						{
							return function->match(params, conversion);
						});
			}
		};

		struct engine_stack
		{
			friend class dispatcher;
			friend class dispatcher_state;

			using name_type			   = engine_core::name_type;
			using name_view_type	   = engine_core::name_view_type;

			using variable_type		   = engine_core::variable_type;

			using scope_type		   = engine_core::variables_type;
			using stack_data_type	   = std::vector<scope_type>;
			using stack_type		   = std::vector<stack_data_type>;

			using param_list_type	   = parameters_type;
			using param_list_view_type = parameters_view_type;
			using param_lists_type	   = std::vector<param_list_type>;

			using call_depth_type	   = int;

			stack_type		 stack;
			param_lists_type param_lists;
			call_depth_type	 depth;

		private:
			void prepare_new_stack()
			{
				// add a new Stack with 1 element
				stack.emplace_back(1);
			}

			void prepare_new_scope() { recent_stack_data().emplace_back(); }
			void prepare_new_call() { param_lists.emplace_back(); }

			void finish_stack() noexcept
			{
				gal_assert(not stack.empty());
				stack.pop_back();
			}

			void finish_scope() noexcept
			{
				gal_assert(not recent_stack_data().empty());
				recent_stack_data().pop_back();
			}

			void finish_call() noexcept
			{
				gal_assert(not param_lists.empty());
				param_lists.pop_back();
			}

			/**
			 * @brief Adds a named object to the current scope.
			 * @note This version does not check the validity of the name.
			 */
			variable_type& add_variable_no_check(const name_view_type name, variable_type variable)
			{
				if (const auto [it, inserted] = recent_scope().try_emplace(name_type{name}, std::move(variable));
					inserted)
				{
					return it->second;
				}
				throw exception::name_conflict_error{name};
			}

			void push_param(param_list_type&& params)
			{
				auto& current_call = recent_call_param();
				current_call.insert(current_call.end(), std::make_move_iterator(params.begin()), std::make_move_iterator(params.end()));
			}

			void push_param(const param_list_view_type params)
			{
				auto& current_call = recent_call_param();
				current_call.insert(current_call.end(), params.begin(), params.end());
			}

			void pop_param() noexcept
			{
				auto& current_call = param_lists.back();
				current_call.clear();
			}

		public:
			engine_stack()
				: depth{0}
			{
				prepare_new_stack();
				prepare_new_call();
			}

			[[nodiscard]] constexpr bool is_root() const noexcept
			{
				return depth == 0;
			}

			/**
			 * @brief Pushes a new stack on to the list of stacks.
			 */
			void								 new_stack() { prepare_new_stack(); }

			void								 pop_stack() { finish_stack(); }

			[[nodiscard]] stack_data_type&		 recent_stack_data() noexcept { return stack.back(); }

			[[nodiscard]] const stack_data_type& recent_stack_data() const noexcept { return stack.back(); }

			[[nodiscard]] stack_data_type&		 recent_parent_stack_data() noexcept { return stack[stack.size() - 2]; }

			[[nodiscard]] const stack_data_type& recent_parent_stack_data() const noexcept { return stack[stack.size() - 2]; }

			/**
			 * @return All values in the local thread state.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<scope_type::value_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::value_type, AnyOther...> copy_recent_locals() const
			{
				const auto& s = recent_stack_data();
				gal_assert(not s.empty());
				return Container<scope_type::value_type, AnyOther...>{s.front().begin(), s.front().end()};
			}

			/**
			 * @return All values in the local thread state.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::key_type, scope_type::mapped_type, AnyOther...> copy_recent_locals() const
			{
				const auto& s = recent_stack_data();
				gal_assert(not s.empty());
				return Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>{s.front().begin(), s.front().end()};
			}

			/**
			 * @return All values in the local thread state in the parent scope,
			 * or if it does not exist, the current scope.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<scope_type::value_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::value_type, AnyOther...> copy_recent_parent_locals() const
			{
				if (const auto& s = recent_stack_data();
					s.size() > 1) { return Container<scope_type::value_type, AnyOther...>{s[1].begin(), s[1].end()}; }
				return copy_recent_locals<Container, AnyOther...>();
			}

			/**
			 * @return All values in the local thread state in the parent scope,
			 * or if it does not exist, the current scope.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::key_type, scope_type::mapped_type, AnyOther...> copy_recent_parent_locals() const
			{
				if (const auto& s = recent_stack_data();
					s.size() > 1) { return Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>{s[1].begin(), s[1].end()}; }
				return copy_recent_locals<Container, AnyOther...>();
			}

			/**
			 * @brief Sets all of the locals for the current thread state.
			 *
			 * @param new_locals The set of variables to replace the current state with.
			 *
			 * @note Any existing locals are removed and the given set of variables is added.
			 */
			void set_locals(const scope_type& new_locals)
			{
				auto& s = recent_stack_data();
				s.front().insert(new_locals.begin(), new_locals.end());
			}

			/**
			 * @brief Sets all of the locals for the current thread state.
			 *
			 * @param new_locals The set of variables to replace the current state with.
			 *
			 * @note Any existing locals are removed and the given set of variables is added.
			 */
			void set_locals(scope_type&& new_locals)
			{
				auto& s = recent_stack_data();
				s.front().insert(std::make_move_iterator(new_locals.begin()), std::make_move_iterator(new_locals.end()));
			}

			/**
			 * @brief Adds a new scope to the stack.
			 */
			void new_scope()
			{
				prepare_new_scope();
				prepare_new_call();
			}

			[[nodiscard]] scope_type&		recent_scope() noexcept { return recent_stack_data().back(); }

			[[nodiscard]] const scope_type& recent_scope() const noexcept { return recent_stack_data().back(); }

			/**
			 * @brief Pops the current scope from the stack.
			 */
			void							pop_scope() noexcept
			{
				finish_call();
				finish_scope();
			}

			variable_type& add_variable(const name_view_type name, variable_type variable)
			{
				auto& stack_data = recent_stack_data();

				for (auto it_scope = stack_data.rbegin(); it_scope != stack_data.rend(); ++it_scope)
				{
					if (auto it = it_scope->find(name); it != it_scope->end())
					{
						it->second = std::move(variable);
						return it->second;
					}
				}

				return add_variable_no_check(name, std::move(variable));
			}

			[[nodiscard]] param_list_type&		 recent_call_param() noexcept { return param_lists.back(); }

			[[nodiscard]] const param_list_type& recent_call_param() const noexcept { return param_lists.back(); }

			void								 emit_call(type_conversion_manager::conversion_saves& saves)
			{
				if (is_root()) { type_conversion_manager::enable_conversion_saves(saves, true); }
				++depth;

				push_param(type_conversion_manager::take_conversion_saves(saves));
			}

			void finish_call(type_conversion_manager::conversion_saves& saves) noexcept
			{
				--depth;
				gal_assert(depth >= 0);

				if (is_root())
				{
					pop_param();
					type_conversion_manager::enable_conversion_saves(saves, false);
				}
			}
		};

		class dispatcher_state
		{
		public:
			using dispatcher_type = std::reference_wrapper<dispatcher>;

			using name_type		  = engine_core::name_type;
			using name_view_type  = engine_core::name_view_type;

			using variable_type	  = engine_core::variable_type;

		private:
			dispatcher_type		  d_;
			type_conversion_state conversion_;

		public:
			explicit dispatcher_state(dispatcher& d);

			[[nodiscard]] dispatcher_type::type*					 operator->() const noexcept { return &d_.get(); }

			[[nodiscard]] dispatcher_type::type&					 operator*() const noexcept { return d_.get(); }

			[[nodiscard]] auto&										 stack() const noexcept;

			[[nodiscard]] const type_conversion_state&				 conversion() const noexcept { return conversion_; }

			[[nodiscard]] type_conversion_manager::conversion_saves& conversion_saves() const noexcept { return conversion_.saves(); }

			variable_type&											 add_object_no_check(name_view_type name, variable_type&& object) const;

			variable_type&											 add_object_no_check(name_view_type name, const variable_type& object) const;

			[[nodiscard]] variable_type&							 get_object(name_view_type name, auto& cache_location) const;
		};

		struct scoped_scope : utils::scoped_object<scoped_scope>
		{
			std::reference_wrapper<dispatcher_state> state;

			explicit scoped_scope(dispatcher_state& state)
				: state{state} {}

		private:
			friend struct utils::scoped_object<scoped_scope>;

			void do_construct() const;
			void do_destruct() const;
		};

		struct scoped_object_scope : scoped_scope
		{
			scoped_object_scope(dispatcher_state& state, engine_core::variable_type&& object);

			scoped_object_scope(dispatcher_state& state, const engine_core::variable_type& object);
		};

		struct scoped_stack_scope : utils::scoped_object<scoped_stack_scope>
		{
			std::reference_wrapper<dispatcher_state> state;

			explicit scoped_stack_scope(dispatcher_state& state)
				: state{state} {}

		private:
			friend struct utils::scoped_object<scoped_stack_scope>;

			void do_construct() const;
			void do_destruct() const;
		};

		struct scoped_function_scope : utils::scoped_object<scoped_function_scope>
		{
			std::reference_wrapper<dispatcher_state> state;

			explicit scoped_function_scope(dispatcher_state& state)
				: state{state} {}

		private:
			friend struct utils::scoped_object<scoped_function_scope>;

			void do_construct() const;
			void do_destruct() const;
		};

		/**
		 * @brief Main class for the dispatch kits.
		 * Handles management of the object stack, functions and registered types.
		 */
		class dispatcher
		{
			friend class dispatcher_state;

		public:
			constexpr static char type_name_format[]	= "@@{}@@";
			constexpr static char method_missing_name[] = "method_missing";

			using name_type								= engine_core::name_type;
			using name_view_type						= engine_core::name_view_type;

			using variable_type							= engine_core::variable_type;

			using type_infos_type						= engine_core::type_infos_type;

			using scope_type							= engine_stack::scope_type;
			using stack_data_type						= engine_stack::stack_data_type;

			using variable_cache_location_type			= std::optional<std::reference_wrapper<variable_type>>;

			struct state_type
			{
				using function_type			= dispatch_function::function_type;
				using function_object_type	= proxy_function;
				using boxed_function_type	= variable_type;

				using functions_type		= std::map<name_type, std::shared_ptr<dispatch_function::functions_type>, std::less<>>;
				using function_objects_type = std::map<name_type, function_object_type, std::less<>>;
				using boxed_functions_type	= std::map<name_type, boxed_function_type, std::less<>>;
				using variables_type		= engine_core::variables_type;

				functions_type		  functions;
				function_objects_type function_objects;
				boxed_functions_type  boxed_functions;
				variables_type		  variables;
				type_infos_type		  types;
			};

			// todo: The lifetime of our cached location may be longer than the actual object (such as returning an empty smart pointer and then automatically destroying it after use)
			using function_cache_location_type = std::optional<state_type::functions_type::mapped_type>;

		private:
			state_type							   state_;
			type_conversion_manager				   manager_;
			std::reference_wrapper<parser_base>	   parser_;

			utils::thread_storage<engine_stack>	   stack_;
			mutable utils::threading::shared_mutex mutex_;
			mutable function_cache_location_type   method_missing_location_;

			struct function_comparator
			{
				bool operator()(const proxy_function& lhs, const proxy_function& rhs)
				{
					const auto real_lhs = std::dynamic_pointer_cast<const dynamic_proxy_function_base>(lhs);
					const auto real_rhs = std::dynamic_pointer_cast<const dynamic_proxy_function_base>(rhs);

					if (real_lhs && real_rhs)
					{
						if (real_lhs->get_guard()) { return real_rhs->get_guard() ? false : true; }
						return false;
					}

					if (real_lhs && not real_rhs) { return false; }
					if (not real_lhs && real_rhs) { return true; }

					const auto& lhs_types		  = lhs->types();
					const auto& rhs_types		  = rhs->types();

					const auto	boxed_type		  = make_type_info<boxed_value>();
					const auto	boxed_number_type = make_type_info<boxed_number>();

					for (decltype(lhs_types.size()) i = 1; i < lhs_types.size() && i < rhs_types.size(); ++i)
					{
						const auto& lhs_ti = lhs_types[i];
						const auto& rhs_ti = rhs_types[i];

						if (lhs_ti.bare_equal(rhs_ti) && lhs_ti.is_const() == rhs_ti.is_const())
						{
							// The first two types are essentially the same, next iteration
							continue;
						}

						if (lhs_ti.bare_equal(rhs_ti) && lhs_ti.is_const() && not rhs_ti.is_const())
						{
							// const is after non-const for the same type
							return false;
						}

						if (lhs_ti.bare_equal(rhs_ti) && not lhs_ti.is_const()) { return true; }

						if (lhs_ti.bare_equal(boxed_type))
						{
							// boxed_values are sorted last
							return false;
						}

						if (rhs_ti.bare_equal(boxed_type)) { return true; }

						if (lhs_ti.bare_equal(boxed_number_type)) { return false; }

						if (rhs_ti.bare_equal(boxed_number_type)) { return true; }

						// otherwise, we want to sort by typeid
						return lhs_ti.before(rhs_ti);
					}

					return false;
				}
			};

			/**
			 * @return a function object (boxed_value wrapper) if it exists.
			 * @throw std::range_error if it does not.
			 * @warning does not obtain a mutex lock.
			 */
			[[nodiscard]] variable_type& get_function_object(const name_view_type name, variable_cache_location_type& cache_location)
			{
				if (cache_location.has_value())
				{
					return *cache_location;
				}

				auto& functions = state_.boxed_functions;

				if (auto it = functions.find(name);
					it != functions.end())
				{
					cache_location.emplace(std::ref(it->second));
					return it->second;
				}
				throw std::range_error{"object not found"};
			}

		public:
			explicit dispatcher(parser_base& p)
				: parser_{p} {}

			/**
			 * @brief casts an object while applying any dynamic_conversion available.
			 * @throw bad_boxed_cast(std::bad_cast)
			 */
			template<typename T>
			decltype(auto) boxed_cast(const boxed_value& object) const
			{
				const type_conversion_state state{manager_};
				return gal::lang::boxed_cast<T>(object, &state);
			}

			/**
			 * @brief Registers a new named type.
			 */
			void add_type_info(const name_view_type name, const gal_type_info& type)
			{
				add_global(std_format::format(type_name_format, name), const_var(type));

				utils::threading::unique_lock lock{mutex_};

				state_.types.emplace(name, type);
			}

			/**
			 * @brief Add a new named proxy_function to the system.
			 * @throw name_conflict_error if there's a function matching the given one being added.
			 */
			void add_function(const name_view_type name, state_type::function_type&& function)
			{
				utils::threading::unique_lock lock{mutex_};

				auto						  function_object = [name, this](auto&& func) -> state_type::function_object_type
				{
					auto& functions = state_.functions;

					if (const auto it = functions.find(name);
						it != functions.end())
					{
						for (const auto& f: *it->second)
						{
							if (*func == *f)
							{
								throw exception::name_conflict_error{name};
							}
						}

						auto copy_fs = *it->second;
						// tightly control vec growth
						copy_fs.reserve(copy_fs.size() + 1);
						copy_fs.emplace_back(std::move(func));
						std::ranges::stable_sort(copy_fs, function_comparator{});
						it->second = std::make_shared<std::decay_t<decltype(copy_fs)>>(copy_fs);
						return std::make_shared<dispatch_function>(std::move(copy_fs));
					}
					else if (func->has_arithmetic_param())
					{
						// if the function is the only function, but it also contains
						// arithmetic operators, we must wrap it in a dispatch function
						// to allow for automatic arithmetic type conversions
						std::decay_t<decltype(*it->second)> fs;
						fs.reserve(1);
						fs.emplace_back(std::move(func));
						functions.emplace(name, std::make_shared<std::decay_t<decltype(fs)>>(fs));
						return std::make_shared<dispatch_function>(std::move(fs));
					}
					else
					{
						auto fs = std::make_shared<std::decay_t<decltype(*it->second)>>();
						fs->emplace_back(func);
						functions.emplace(name, fs);
						return func;
					}
				}(std::move(function));

				state_.boxed_functions.insert_or_assign(name_type{name}, const_var(function_object));
				state_.function_objects.insert_or_assign(name_type{name}, std::move(function_object));
			}

			/**
			 * @brief Adds a new global (const) shared object, between all the threads.
			 */
			variable_type& add_global(const name_view_type name, variable_type&& variable)
			{
				if (not variable.is_const())
				{
					throw exception::global_mutable_error{name};
				}

				utils::threading::unique_lock lock{mutex_};

				if (auto [it, inserted] = state_.variables.try_emplace(name_type{name}, std::move(variable));
					inserted)
				{
					return it->second;
				}
				throw exception::name_conflict_error{name};
			}

			/**
			 * @brief Add a new conversion for up-casting to a base class.
			 */
			void		   add_type_conversion(const type_conversion_manager::conversion_type& conversion) { manager_.add(conversion); }


			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			variable_type& add_global_mutable(const name_view_type name, variable_type&& object)
			{
				utils::threading::unique_lock lock{mutex_};

				if (const auto [it, inserted] = state_.variables.try_emplace(name_type{name}, std::move(object));
					inserted) { return it->second; }
				throw exception::name_conflict_error{name};
			}

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			variable_type& add_global_mutable_no_throw(const name_view_type name, variable_type&& object)
			{
				utils::threading::unique_lock lock{mutex_};
				return state_.variables.try_emplace(name_type{name}, std::move(object)).first->second;
			}

			/**
			 * @brief Updates an existing global shared object or adds a new global shared object if not found.
			 */
			void global_assign_or_insert(const name_view_type name, variable_type&& object)
			{
				utils::threading::unique_lock lock{mutex_};

				state_.variables.insert_or_assign(name_type{name}, std::move(object));
			}

			/**
			 * @brief Set the value of an object, by name. If the object
			 * is not available in the current scope it is created.
			 */
			variable_type& local_assign_or_insert(const name_view_type name, variable_type&& object)
			{
				return stack_->add_variable(name, std::move(object));
			}

			/**
			 * @brief Add a object, if this variable already exists in the current scope, an exception will be thrown.
			 */
			variable_type&				 local_insert_or_throw(const name_view_type name, variable_type&& object) { return stack_->add_variable_no_check(name, std::move(object)); }

			/**
			 * @brief Searches the current stack for an object of the given name
			 * includes a special overload for the _ place holder object to
			 * ensure that it is always in scope.
			 */
			[[nodiscard]] variable_type& get_object(const name_view_type name, variable_cache_location_type& cache_location)
			{
				if (not cache_location.has_value())
				{
					auto& stack_data = stack_->recent_stack_data();

					// Is it in the stack?
					for (auto it_scope = stack_data.rbegin(); it_scope != stack_data.rend(); ++it_scope)
					{
						if (const auto it = std::ranges::find_if(
									*it_scope,
									[name](const auto& pair)
									{ return pair.first == name; });
							it != it_scope->end())
						{
							cache_location.emplace(it->second);
							return it->second;
						}
					}
				}

				// Is the value we are looking for a global or function?
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = state_.variables.find(name);
					it != state_.variables.end())
				{
					cache_location.emplace(it->second);
					return it->second;
				}

				// no? is it a function object?
				return get_function_object(name, cache_location);
			}

			/**
			 * @brief Returns the type info for a named type.
			 * @throw std::range_error
			 */
			[[nodiscard]] gal_type_info get_type_info(const name_view_type name, const bool throw_if_not_exist = true) const
			{
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = state_.types.find(name);
					it != state_.types.end())
				{
					return it->second;
				}

				if (throw_if_not_exist)
				{
					throw std::range_error{"type_info not exist"};
				}
				return {};
			}

			/**
			 * @brief return true if the object matches the registered type by name.
			 */
			[[nodiscard]] bool is_type_match(const name_view_type name, const variable_type& object) const noexcept
			{
				try
				{
					if (get_type_info(name).bare_equal(object.type_info())) { return true; }
				}
				catch (const std::range_error&)
				{
				}

				try
				{
					const auto& o = boxed_cast<const dynamic_object&>(object);
					return o.type_name() == name;
				}
				catch (const std::bad_cast&)
				{
				}

				return false;
			}

			/**
			 * @brief Returns the registered name of a known type_info object
			 * compares the "bare_type_info" for the broadest possible match.
			 */
			[[nodiscard]] name_view_type get_type_name(const gal_type_info& ti) const
			{
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = std::ranges::find_if(
							state_.types,
							[&ti](const auto& t)
							{ return t.bare_equal(ti); },
							[](const auto& pair)
							{ return pair.second; });
					it != state_.types.end())
				{
					return it->first;
				}

				return ti.bare_name();
			}

			[[nodiscard]] name_view_type get_type_name(const variable_type& object) const { return get_type_name(object.type_info()); }

			/**
			 * @brief Return true if a function exists.
			 */
			[[nodiscard]] bool			 has_function(const name_view_type name) const
			{
				utils::threading::shared_lock lock{mutex_};
				return state_.functions.contains(name);
			}

			/**
			 * @brief Return a function by name.
			 *
			 * @note Returns a valid pointer (instead of a null pointer) even if not found.
			 *
			 * @todo Do we really need return a valid pointer?
			 */
			[[nodiscard]] state_type::functions_type::mapped_type get_function(const name_view_type name) const
			{
				utils::threading::shared_lock lock{mutex_};

				const auto&					  functions = state_.functions;
				if (const auto it = functions.find(name);
					it != functions.end())
				{
					return it->second;
				}
				return std::make_shared<state_type::functions_type::mapped_type::element_type>();
			}

			[[nodiscard]] state_type::functions_type::mapped_type get_method_missing_functions() const
			{
				if (method_missing_location_.has_value())
				{
					return *method_missing_location_;
				}

				auto result = get_function(method_missing_name);
				method_missing_location_.emplace(result);
				return result;
			}

			/**
			 * @return a function object (boxed_value wrapper) if it exists.
			 * @throw std::range_error if it does not.
			 */
			[[nodiscard]] variable_type& get_function_object(const name_view_type name)
			{
				utils::threading::shared_lock lock{mutex_};

				variable_cache_location_type  dummy{};
				return get_function_object(name, dummy);
			}

			/**
			 * @brief Get a map of all objects that can be seen from the current scope in a scripting context.
			 */
			[[nodiscard]] state_type::variables_type copy_scripting_objects() const
			{
				// We don't want the current context, but one up if it exists
				const auto&				   stack = (stack_->stack.size() == 1) ? stack_->recent_stack_data() : stack_->recent_parent_stack_data();

				state_type::variables_type ret{};

				// note: map insert doesn't overwrite existing values, which is why this works
				std::ranges::for_each(
						stack.rbegin(),
						stack.rend(),
						[&ret](const auto& scope)
						{ ret.insert(scope.begin(), scope.end()); });

				// add the global values
				utils::threading::shared_lock lock{mutex_};
				ret.insert(state_.variables.begin(), state_.variables.end());

				return ret;
			}

			/**
			 * @brief Get a map of all registered functions.
			 */
			[[nodiscard]] auto copy_functions() const
			{
				utils::threading::shared_lock											   lock{mutex_};

				const auto&																   functions = state_.functions;

				// todo: return type?
				std::map<state_type::functions_type ::key_type, state_type::function_type> ret{};

				std::ranges::for_each(
						functions,
						[&ret](const auto& pair)
						{
							std::ranges::for_each(
									*pair.second,
									[&ret, &pair](const auto& function)
									{ ret.emplace(pair.first, function); });
						});

				return ret;
			}

			/**
			 * @brief Get a map of all functions that can be seen from a scripting context.
			 */
			[[nodiscard]] state_type::variables_type copy_function_objects() const
			{
				const auto&				   functions = state_.function_objects;

				state_type::variables_type ret{};

				std::ranges::for_each(
						functions,
						[&ret](const auto& function)
						{ ret.emplace(function.first, const_var(function.second)); });

				return ret;
			}

			/**
			 * @brief Return all registered types.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<type_infos_type::value_type, AnyOther...>, type_infos_type::const_iterator, type_infos_type::const_iterator>
			[[nodiscard]] Container<type_infos_type::value_type, AnyOther...> copy_types() const
			{
				utils::threading::shared_lock lock{mutex_};
				return Container<type_infos_type::value_type, AnyOther...>{state_.types.begin(), state_.types.end()};
			}

			[[nodiscard]] state_type copy_state() const
			{
				utils::threading::shared_lock lock{mutex_};
				return state_;
			}

			void swap_state(state_type& other) noexcept
			{
				using std::swap;
				swap(state_, other);
			}

			void set_state(const state_type& state)
			{
				utils::threading::unique_lock lock{mutex_};
				state_ = state;
			}

			void		emit_call() { stack_->emit_call(manager_.get_conversion_saves()); }

			void		finish_call() noexcept { stack_->finish_call(manager_.get_conversion_saves()); }

			static bool is_member_function_call(
					const dispatch_function::functions_type& functions,
					const parameters_view_type				 params,
					const bool								 has_param,
					const type_conversion_state&			 conversion) noexcept
			{
				if (not has_param || params.empty()) { return false; }

				return std::ranges::any_of(
						functions,
						[&params, &conversion](const auto& function)
						{ return function->is_member_function() && function->is_first_type_match(params.front(), conversion); });
			}

			[[nodiscard]] variable_type call_member_function(
					const name_view_type		  name,
					function_cache_location_type& cache_location,
					const parameters_view_type	  params,
					const bool					  has_params,
					const type_conversion_state&  conversion)
			{
				gal_assert(not cache_location.has_value());
				const auto& functions = get_function(name);
				cache_location.emplace(functions);

				const auto do_member_function_call = [this, &conversion](
															 const exception::arity_error::size_type  num_params,
															 const parameters_view_type				  ps,
															 const dispatch_function::functions_type& fs) -> variable_type
				{
					const auto	  member_params = ps.front_sub_list(num_params);
					variable_type object		= dispatch(fs, member_params, conversion);
					if (num_params < static_cast<exception::arity_error::size_type>(ps.size()) || object.type_info().bare_equal(make_type_info<proxy_function_base>()))
					{
						dispatcher_state	state{*this};
						scoped_object_scope object_scope{state, ps.front()};

						try
						{
							const auto function = boxed_cast<const proxy_function_base*>(object);
							try
							{
								return (*function)(ps.sub_list(num_params), conversion);
							}
							catch (const exception::bad_boxed_cast&)
							{
							}
							catch (const exception::arity_error&)
							{
							}
							catch (const exception::guard_error&)
							{
							}
							throw exception::dispatch_error{
									ps.sub_list(num_params).to<parameters_type>(),
									{boxed_cast<immutable_proxy_function>(object)}};
						}
						catch (const exception::bad_boxed_cast&)
						{
							// unable to convert bv into a proxy_function_base
							throw exception::dispatch_error{
									ps.sub_list(num_params).to<parameters_type>(),
									{fs.begin(), fs.end()}};
						}
					}
					return object;
				};

				if (is_member_function_call(*functions, params, has_params, conversion)) { return do_member_function_call(1, params, *functions); }

				std::exception_ptr current_exception;

				if (not functions->empty())
				{
					try
					{
						return dispatch(*functions, params, conversion);
					}
					catch (exception::dispatch_error&)
					{
						current_exception = std::current_exception();
					}
				}

				// If we get here we know that either there was no method with that name,
				// or there was no matching method

				const auto missing_functions = [this, params, &conversion]
				{
					dispatch_function::functions_type ret{};

					const auto						  mmf = get_method_missing_functions();

					std::ranges::for_each(
							*mmf,
							[&ret, params, &conversion](const auto& f)
							{ if (f->is_first_type_match(params.front(), conversion)) { ret.push_back(f); } });

					return ret;
				}();

				const bool is_no_param = std::ranges::all_of(
						missing_functions,
						[](const auto& f)
						{ return f->get_arity() == 2; });

				if (not missing_functions.empty())
				{
					try
					{
						if (is_no_param)
						{
							auto tmp_params = params.to<parameters_type>();
							tmp_params.insert(tmp_params.begin() + 1, var(name));
							return do_member_function_call(2, parameters_view_type{tmp_params}, missing_functions);
						}
						const std::array tmp_params{params.front(), var(name), var(parameters_type{params.begin() + 1, params.end()})};
						return dispatch(missing_functions, parameters_view_type{tmp_params}, conversion);
					}
					catch (const exception::option_explicit_error& e)
					{
						throw exception::dispatch_error{
								params.to<parameters_type>(),
								{functions->begin(), functions->end()},
								e.what()};
					}
				}

				// If we get all the way down here we know there was no "method_missing" method at all.
				if (current_exception) { std::rethrow_exception(current_exception); }
				throw exception::dispatch_error{
						params.to<parameters_type>(),
						{functions->begin(), functions->end()}};
			}

			[[nodiscard]] variable_type call_function(
					const name_view_type		  name,
					function_cache_location_type& cache_location,
					const parameters_view_type	  params,
					const type_conversion_state&  conversion) const
			{
				if (cache_location.has_value())
				{
					return dispatch(**cache_location, params, conversion);
				}

				auto functions = get_function(name);
				cache_location.emplace(functions);

				return dispatch(*functions, params, conversion);
			}

			/**
			 * @brief Dump type info.
			 */
			void dump_type_to(const gal_type_info& type, string_type& dest) const
			{
				std_format::format_to(
						std::back_inserter(dest),
						"[{}]{}",
						type.is_const() ? "immutable" : "mutable",
						get_type_name(type));
			}

			/**
			 * @brief Dump type info.
			 */
			[[nodiscard]] string_type dump_type(const gal_type_info& type) const
			{
				string_type ret{};
				dump_type_to(type, ret);
				return ret;
			}

			/**
			 * @brief Dump object info.
			 */
			void dump_object_to(const variable_type& object, string_type& dest) const
			{
				dump_type_to(object.type_info(), dest);
			}

			/**
			 * @brief Dump object info.
			 */
			[[nodiscard]] string_type dump_object(const variable_type& object) const
			{
				return dump_type(object.type_info());
			}

			void dump_function_to(const name_view_type name, const state_type::function_objects_type::mapped_type& function, string_type& dest) const
			{
				const auto& types = function->types();

				dest.reserve(dest.size() + types.size() * 64);

				dump_type_to(types.front(), dest);

				dest.append(" ").append(name).append("(");
				for (auto it = types.begin() + 1; it != types.end(); ++it)
				{
					dump_type_to(*it, dest);

					if (it != types.end())
					{
						dest.append(", ");
					}
				}
				dest.append(")");
			}

			[[nodiscard]] string_type dump_function(const name_view_type name, const state_type::function_objects_type::mapped_type& function) const
			{
				string_type ret{};
				dump_function_to(name, function, ret);
				return ret;
			}

			void dump_function_to(const state_type::function_objects_type::value_type& pair, string_type& dest) const
			{
				dump_function_to(pair.first, pair.second, dest);
			}

			[[nodiscard]] string_type dump_function(const state_type::function_objects_type::value_type& pair) const
			{
				string_type ret{};
				dump_function_to(pair, ret);
				return ret;
			}

			void dump_everything_to(string_type& dest) const
			{
				dest.append("Registered type: \n");

				// todo: copy or lock?
				{
					// const auto types = copy_types<std::vector>();
					utils::threading::shared_lock lock{mutex_};
					std::ranges::for_each(
							state_.types,
							[&dest](const auto& pair)
							{ dest.append(pair.first).append(": ").append(pair.second.bare_name()).append("\n"); });
				}
				dest.push_back('\n');

				// todo: copy or lock?
				{
					// const auto functions = copy_functions();
					utils::threading::shared_lock lock{mutex_};
					std::ranges::for_each(
							state_.functions,
							[&dest, this](const auto& pair)
							{
								std::ranges::for_each(
										*pair.second,
										[&dest, &pair, this](const auto& function)
										{
											dump_function_to(pair.first, function, dest);
										});
							});
				}
				dest.push_back('\n');
			}

			[[nodiscard]] string_type dump_everything() const
			{
				string_type ret{};
				dump_everything_to(ret);
				return ret;
			}

			[[nodiscard]] const type_conversion_manager& get_conversion_manager() const noexcept { return manager_; }

			[[nodiscard]] parser_base&					 get_parser() const noexcept { return parser_.get(); }
		};

		dispatcher_state::dispatcher_state(dispatcher& d)
			: d_{d},
			  conversion_{d.manager_} {}

		auto& dispatcher_state::stack() const noexcept
		{
			return this->operator*().stack_;
		}

		dispatcher_state::variable_type& dispatcher_state::add_object_no_check(const dispatcher_state::name_view_type name, dispatcher_state::variable_type&& object) const
		{
			return stack()->add_variable_no_check(name, std::move(object));
		}

		dispatcher_state::variable_type& dispatcher_state::add_object_no_check(const name_view_type name, const variable_type& object) const
		{
			return stack()->add_variable_no_check(name, object);
		}

		dispatcher_state::variable_type& dispatcher_state::get_object(const dispatcher_state::name_view_type name, auto& cache_location) const
		{
			return this->operator*().get_object(name, cache_location);
		}

		void scoped_scope::do_construct() const
		{
			state.get().stack()->new_scope();
		}

		void scoped_scope::do_destruct() const
		{
			state.get().stack()->pop_scope();
		}

		scoped_object_scope::scoped_object_scope(dispatcher_state& state, engine_core::variable_type&& object)
			: scoped_scope{state}
		{
			state.add_object_no_check(lang::object_self_type_name::value, std::move(object));
		}

		scoped_object_scope::scoped_object_scope(dispatcher_state& state, const engine_core::variable_type& object)
			: scoped_scope{state}
		{
			state.add_object_no_check(lang::object_self_type_name::value, object);
		}

		void scoped_stack_scope::do_construct() const
		{
			state.get().stack()->new_stack();
		}

		void scoped_stack_scope::do_destruct() const
		{
			state.get().stack()->pop_stack();
		}

		void scoped_function_scope::do_construct() const
		{
			state.get().stack()->emit_call(state.get().conversion_saves());
		}

		void scoped_function_scope::do_destruct() const
		{
			state.get().stack()->finish_call(state.get().conversion_saves());
		}

	}// namespace dispatcher_detail
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_DISPATCHER_HPP
