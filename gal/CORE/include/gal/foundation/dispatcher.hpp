#pragma once

#ifndef GAL_LANG_FOUNDATION_DISPATCHER_HPP
	#define GAL_LANG_FOUNDATION_DISPATCHER_HPP

	#include <gal/boxed_cast.hpp>
	#include <gal/boxed_value.hpp>
	#include <gal/foundation/dynamic_object.hpp>
	#include <gal/foundation/proxy_function.hpp>
	#include <utils/enum_utils.hpp>
	#include <utils/format.hpp>
	#include <map>
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

			[[nodiscard]] const word_type which() const noexcept { return word_; }
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

			[[nodiscard]] const name_type& which() const noexcept { return name_; }
		};

		class global_mutable_error final : public std::runtime_error
		{
		public:
			global_mutable_error()
				: std::runtime_error{"global variable must be immutable"} {}
		};
	}// namespace exception

	class paser_base;

	/**
	 * @brief Holds a collection of settings which can be applied to the runtime.
	 * @note Used to implement loadable module support.
	 */
	class engine_core
	{
	public:
		// todo: the function of storing strings should be handed over to string_pool
		using name_type				= string_type;

		using function_type			= proxy_function;
		using variable_type			= boxed_value;
		using evaluation_type		= string_type;

		using type_infos_type		= std::map<name_type, gal_type_info>;
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
				throw exception::global_mutable_error{};
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
			using name_type			   = engine_core::name_type;
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
			variable_type& add_variable_no_check(const name_type& name, variable_type variable)
			{
				if (const auto [it, inserted] = recent_scope().emplace(name, std::move(variable));
					not inserted) { throw exception::name_conflict_error{it->first}; }
				else
				{
					return it->second;
				}
			}

			/**
			 * @brief Adds a named object to the current scope.
			 * @note This version does not check the validity of the name.
			 */
			variable_type& add_variable_no_check(name_type&& name, variable_type variable)
			{
				if (const auto [it, inserted] = recent_scope().emplace(std::move(name), std::move(variable));
					not inserted)
				{
					throw exception::name_conflict_error{it->first};
				}
				else
				{
					return it->second;
				}
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

			variable_type& add_variable(const name_type& name, variable_type variable)
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
	}// namespace dispatcher_detail
}// namespace gal::lang::foundation

#endif//GAL_LANG_FOUNDATION_DISPATCHER_HPP
