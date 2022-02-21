#pragma once

#ifndef GAL_LANG_KITS_DISPATCH_HPP
#define GAL_LANG_KITS_DISPATCH_HPP

#include <gal/defines.hpp>
#include <gal/kits/boxed_value.hpp>
#include <gal/kits/boxed_value_cast.hpp>
#include <gal/kits/dynamic_object.hpp>
#include <gal/kits/proxy_function.hpp>
#include <gal/kits/proxy_constructor.hpp>
#include <gal/utility/flat_continuous_map.hpp>

#include <utils/format.hpp>
#include <utils/enum_utils.hpp>
#include <utils/flat_hash_container.hpp>
#include <utils/unordered_hash_container.hpp>

namespace gal::lang
{
	namespace parser
	{
		class parser_base;
	}

	/**
	 * @brief Exception thrown in the case that an object name is invalid because it is a reserved word.
	 */
	class reserved_word_error final : public std::runtime_error
	{
	public:
		using word_type = std::string;

	private:
		word_type word_;

	public:
		explicit reserved_word_error(word_type word)
			: std::runtime_error{
					  std_format::format("'{}' is a reserved word and not allowed in object name", word)
			  },
			  word_{std::move(word)} {}

		[[nodiscard]] const word_type& which() const noexcept { return word_; }
	};

	/**
	 * @brief Exception thrown in the case that an object name is invalid because it contains illegal characters.
	 */
	class illegal_name_error final : public std::runtime_error
	{
	public:
		using name_type = std::string;

	private:
		name_type name_;

	public:
		explicit illegal_name_error(name_type name)
			: std::runtime_error{
					  std_format::format("'{}' is a reserved name and not allowed in object name", name)
			  },
			  name_{std::move(name)} {}

		[[nodiscard]] const name_type& which() const noexcept { return name_; }
	};

	/**
	 * @brief Exception thrown in the case that an object name is invalid because it already exists in current context.
	 */
	class name_conflict_error final : public std::runtime_error
	{
	public:
		using name_type = std::string;

	private:
		name_type name_;

	public:
		explicit name_conflict_error(name_type name)
			: std::runtime_error{
					  std_format::format("'{}' is already defined in the current context", name)
			  },
			  name_{std::move(name)} {}

		[[nodiscard]] const name_type& which() const noexcept { return name_; }
	};

	class global_mutable_error final : public std::runtime_error
	{
	public:
		global_mutable_error()
			: std::runtime_error{"global variable must be immutable"} {}
	};

	/**
	 * @brief Holds a collection of settings which can be applied to the runtime.
	 * @note Used to implement loadable module support.
	 */
	class engine_module
	{
	public:
		using name_type = std::string;

		using type_info_type = utility::gal_type_info;
		using function_type = proxy_function;
		using variable_type = kits::boxed_value;
		using evaluation_type = std::string;
		using type_conversion_type = kits::type_conversion_type;

		using type_infos_type = utility::flat_continuous_map<name_type, type_info_type>;
		using functions_type = utility::flat_continuous_map<name_type, function_type>;
		using globals_type = utility::flat_continuous_map<name_type, variable_type>;
		using evaluations_type = std::vector<evaluation_type>;
		using type_conversions_type = std::vector<type_conversion_type>;

	private:
		type_infos_type type_infos_;
		functions_type functions_;
		globals_type globals_;
		evaluations_type evaluations_;
		type_conversions_type type_conversions_;

		enum class apply_type
		{
			type_info_t,
			function_t,
			global_t,
			evaluation_t,
			type_conversion_t,
		};

		template<apply_type Type, typename Target>
		void do_apply(Target& target)
		{
			using enum apply_type;
			if constexpr (Type == type_info_t)
			{
				std::ranges::for_each(
						type_infos_,
						[&target](const auto& ti)
						{
							try { target.add_type_info(ti.first, ti.second); }
							catch (const name_conflict_error&)
							{
								// todo: Should we throw an error if there's a name conflict while applying a module?
							}
						});
			}
			else if constexpr (Type == function_t)
			{
				std::ranges::for_each(
						functions_,
						[&target](const auto& function)
						{
							try { target.add_function(function.first, function.second); }
							catch (const name_conflict_error&)
							{
								// todo: Should we throw an error if there's a name conflict while applying a module?
							}
						});
			}
			else if constexpr (Type == global_t)
			{
				std::ranges::for_each(
						globals_,
						[&target](const auto& variable) { target.add_global(variable.first, variable.second); });
			}
			else if constexpr (Type == evaluation_t)
			{
				std::ranges::for_each(
						evaluations_,
						[&target](const auto& string) { target.add_eval(string); });
			}
			else
			{
				static_assert(Type == type_conversion_t);
				std::ranges::for_each(
						type_conversions_,
						[&target](const auto& conversion) { target.add_type_conversion(conversion); });
			}
		}

	public:
		engine_module& add_type_info(name_type name, type_info_type ti)
		{
			type_infos_.emplace_back(std::move(name), ti);
			return *this;
		}

		engine_module& add_function(name_type name, function_type function)
		{
			functions_.emplace_back(std::move(name), std::move(function));
			return *this;
		}

		engine_module& add_global(name_type name, variable_type variable)
		{
			if (not variable.is_const()) { throw global_mutable_error{}; }

			globals_.emplace_back(std::move(name), std::move(variable));
			return *this;
		}

		engine_module& add_eval(evaluation_type evaluation)
		{
			evaluations_.push_back(std::move(evaluation));
			return *this;
		}

		engine_module& add_type_conversion(type_conversion_type conversion)
		{
			type_conversions_.push_back(std::move(conversion));
			return *this;
		}

		template<typename Eval, typename Engine>
		void apply(Eval& eval, Engine& engine) const
		{
			using enum apply_type;
			do_apply<type_info_t>(engine);
			do_apply<function_t>(engine);
			do_apply<global_t>(globals_, engine);
			do_apply<evaluation_t>(eval);
			do_apply<type_conversion_t>(engine);
		}

		[[nodiscard]] bool has_function(const function_type& function, std::string_view name) noexcept
		{
			return std::ranges::any_of(
					functions_,
					[&](const auto& func) { return func.first == name && *func.second == *function; });
		}
	};

	using shared_engine_module = std::shared_ptr<engine_module>;

	namespace detail
	{
		/**
		 * @brief A proxy_function implementation that is able to take
		 * a vector of proxy_functions and perform a dispatch on them.
		 * It is used specifically in the case of dealing with function object variables.
		 */
		class dispatch_function final : public kits::proxy_function_base
		{
		public:
			// like the contained_functions_type, but mutable(not const_proxy_function).
			using functions_type = std::vector<proxy_function>;

		private:
			functions_type functions_;

			static type_infos_type build_type_infos(const functions_type& function)
			{
				if (function.empty()) { return {}; }

				auto begin = function.begin();
				auto copy_types = (*begin)->types();
				++begin;

				bool size_mismatch = false;

				while (begin != function.end())
				{
					const auto& param_types = (*begin)->types();

					if (param_types.size() != copy_types.size()) { size_mismatch = true; }

					const auto size = static_cast<decltype(copy_types)::difference_type>(std::ranges::min(copy_types.size(), param_types.size()));
					std::transform(
							copy_types.begin(),
							copy_types.begin() + size,
							param_types.begin(),
							copy_types.begin(),
							[](auto&& lhs, auto&& rhs)
							{
								if (lhs != rhs) { return utility::make_type_info<kits::boxed_value>(); }
								return lhs;
							});

					++begin;
				}

				gal_assert(not copy_types.empty(), "type_info vector is empty, this is only possible if something else is broken");

				if (size_mismatch) { copy_types.resize(1); }

				return copy_types;
			}

			[[nodiscard]] kits::boxed_value do_invoke(const kits::function_parameters& params, const kits::type_conversion_state& conversion) const override { return dispatch(functions_, params, conversion); }

		public:
			explicit dispatch_function(functions_type functions)
				: proxy_function_base{build_type_infos(functions), calculate_arity(functions)},
				  functions_{std::move(functions)} {}

			[[nodiscard]] contained_functions_type get_contained_function() const override { return {functions_.begin(), functions_.end()}; }

			[[nodiscard]] bool operator==(const proxy_function_base& other) const noexcept override
			{
				const auto* df = dynamic_cast<const dispatch_function*>(&other);
				return df && df->functions_ == functions_;
			}

			[[nodiscard]] bool match(const kits::function_parameters& params, const kits::type_conversion_state& conversion) const override
			{
				return std::ranges::any_of(
						functions_,
						[&params, &conversion](const auto& function) { return function->match(params, conversion); });
			}

			static arity_size_type calculate_arity(const functions_type& functions) noexcept
			{
				if (functions.empty()) { return no_parameters_arity; }

				const auto arity = functions.front()->get_arity();

				if (std::ranges::any_of(
						functions,
						[arity](const auto a) { return a != arity; },
						[](const auto& function) { return function->get_arity(); }
						)) { return no_parameters_arity; }

				return arity;
			}
		};

		struct stack_holder
		{
			friend class dispatch_engine;
			friend class scoped_holder;

			template<typename T>
			using internal_stack_type = std::vector<T>;

			using name_type = engine_module::name_type;
			using variable_type = kits::boxed_value;

			using scope_type = utility::flat_continuous_map<name_type, variable_type>;
			using stack_data_type = internal_stack_type<scope_type>;
			using stack_type = internal_stack_type<stack_data_type>;
			using param_list_type = internal_stack_type<variable_type>;
			using param_lists_type = internal_stack_type<param_list_type>;
			using call_depth_type = int;

			stack_type stack;
			param_lists_type param_lists;
			call_depth_type depth;

			struct scoped_holder
			{
				std::reference_wrapper<stack_holder> stack;

				scoped_holder(stack_holder& stack, const variable_type& object)
					: stack{stack}
				{
					stack.new_scope();
					// todo: this' s name?
					stack.add_variable_no_check("__this", object);
				}

				~scoped_holder() noexcept { stack.get().pop_scope(); }

				scoped_holder(const scoped_holder&) = delete;
				scoped_holder& operator=(const scoped_holder&) = delete;
				scoped_holder(scoped_holder&&) = delete;
				scoped_holder& operator=(scoped_holder&&) = delete;
			};

			stack_holder()
				: depth{0}
			{
				prepare_new_stack();
				prepare_new_call();
			}

			[[nodiscard]] constexpr bool is_root() const noexcept { return depth == 0; }

			/**
			 * @brief Pushes a new stack on to the list of stacks.
			 */
			void new_stack() { prepare_new_stack(); }

			void pop_stack() { finish_stack(); }

			[[nodiscard]] stack_data_type& recent_stack_data() noexcept { return stack.back(); }

			[[nodiscard]] const stack_data_type& recent_stack_data() const noexcept { return stack.back(); }

			[[nodiscard]] stack_data_type& recent_parent_stack_data() noexcept { return stack[stack.size() - 2]; }

			[[nodiscard]] const stack_data_type& recent_parent_stack_data() const noexcept { return stack[stack.size() - 2]; }

			/**
			 * @return All values in the local thread state.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
				requires std::is_constructible_v<Container<scope_type::value_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::value_type, AnyOther...> recent_locals() const
			{
				const auto& stack = recent_stack_data();
				gal_assert(not stack.empty());
				return Container<scope_type::value_type, AnyOther...>{stack.front().begin(), stack.front().end()};
			}

			/**
			 * @return All values in the local thread state.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
				requires std::is_constructible_v<Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::key_type, scope_type::mapped_type, AnyOther...> recent_locals() const
			{
				const auto& stack = recent_stack_data();
				gal_assert(not stack.empty());
				return Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>{stack.front().begin(), stack.front().end()};
			}

			/**
			 * @return All values in the local thread state in the parent scope,
			 * or if it does not exist, the current scope.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
				requires std::is_constructible_v<Container<scope_type::value_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::value_type, AnyOther...> recent_parent_locals() const
			{
				if (const auto& stack = recent_stack_data();
					stack.size() > 1) { return Container<scope_type::value_type, AnyOther...>{stack[1].begin(), stack[1].end()}; }
				return recent_locals<Container, AnyOther...>();
			}

			/**
			 * @return All values in the local thread state in the parent scope,
			 * or if it does not exist, the current scope.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
				requires std::is_constructible_v<Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>, scope_type::const_iterator, scope_type::const_iterator>
			[[nodiscard]] Container<scope_type::key_type, scope_type::mapped_type, AnyOther...> recent_parent_locals() const
			{
				if (const auto& s = recent_stack_data();
					s.size() > 1) { return Container<scope_type::key_type, scope_type::mapped_type, AnyOther...>{s[1].begin(), s[1].end()}; }
				return recent_locals<Container, AnyOther...>();
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
				s.front().assign(new_locals.begin(), new_locals.end());
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
				s.front().assign(std::make_move_iterator(new_locals.begin()), std::make_move_iterator(new_locals.end()));
			}

			/**
			 * @brief Adds a new scope to the stack.
			 */
			void new_scope()
			{
				prepare_new_scope();
				prepare_new_call();
			}

			[[nodiscard]] scope_type& recent_scope() noexcept { return recent_stack_data().back(); }

			[[nodiscard]] const scope_type& recent_scope() const noexcept { return recent_stack_data().back(); }

			/**
			 * @brief Pops the current scope from the stack.
			 */
			void pop_scope() noexcept
			{
				finish_call();
				finish_scope();
			}

			[[nodiscard]] scoped_holder make_temp_scope(const variable_type& object) { return {*this, object}; }

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

			void emit_call(kits::type_conversion_manager::conversion_saves& saves)
			{
				if (is_root()) { kits::type_conversion_manager::enable_conversion_saves(saves, true); }
				++depth;

				push_param(kits::type_conversion_manager::take_conversion_saves(saves));
			}

			void finish_call(kits::type_conversion_manager::conversion_saves& saves) noexcept
			{
				--depth;
				gal_assert(depth >= 0);

				if (is_root())
				{
					pop_param();
					kits::type_conversion_manager::enable_conversion_saves(saves, false);
				}
			}

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
					not inserted) { throw name_conflict_error{it->first}; }
				else { return it->second; }
			}

			/**
			 * @brief Adds a named object to the current scope.
			 * @note This version does not check the validity of the name.
			 */
			variable_type& add_variable_no_check(name_type&& name, variable_type variable)
			{
				if (const auto [it, inserted] = recent_scope().emplace(std::move(name), std::move(variable));
					not inserted) { throw name_conflict_error{it->first}; }
				else { return it->second; }
			}

			void push_param(const kits::function_parameters& params)
			{
				auto& current_call = param_lists.back();
				current_call.insert(current_call.end(), params.begin(), params.end());
			}

			void push_param(param_list_type&& params)
			{
				auto& current_call = param_lists.back();
				for (auto&& param: params) { current_call.insert(current_call.end(), std::move(param)); }
			}

			void pop_param() noexcept
			{
				auto& current_call = param_lists.back();
				current_call.clear();
			}
		};

		/**
		 * @brief Main class for the dispatch kits.
		 * Handles management of the object stack, functions and registered types.
		 */
		class dispatch_engine
		{
		public:
			constexpr static char type_name_format[] = "@@{}@@";
			constexpr static char method_missing_name[] = "method_missing";

			using name_type = engine_module::name_type;
			using object_type = stack_holder::variable_type;

			using type_name_map_type = utils::unordered_hash_map<name_type, utility::gal_type_info>;
			using scope_type = stack_holder::scope_type;
			using stack_data_type = stack_holder::stack_data_type;
			using location_type = std::atomic_uint_fast32_t;

			struct state_type
			{
				using functions_type = utility::flat_continuous_map<name_type, std::shared_ptr<dispatch_function::functions_type>>;
				using function_objects_type = utility::flat_continuous_map<name_type, proxy_function>;
				using boxed_functions_type = utility::flat_continuous_map<name_type, object_type>;

				using global_objects_type = utils::unordered_hash_map<name_type, object_type>;

				functions_type functions;
				function_objects_type function_objects;
				boxed_functions_type boxed_functions;
				global_objects_type global_objects;
				type_name_map_type types;
			};

		private:
			state_type state_;
			kits::type_conversion_manager conversion_manager_;
			std::reference_wrapper<parser::parser_base> parser_;

			utils::thread_storage<stack_holder> stack_holder_;
			mutable utils::threading::shared_mutex mutex_;
			mutable location_type method_missing_location_;

			static bool function_less_than(const proxy_function& lhs, const proxy_function& rhs) noexcept
			{
				const auto real_lhs = std::dynamic_pointer_cast<const kits::base::dynamic_proxy_function_base>(lhs);
				const auto real_rhs = std::dynamic_pointer_cast<const kits::base::dynamic_proxy_function_base>(rhs);

				if (real_lhs && real_rhs)
				{
					if (real_lhs->get_guard()) { return real_rhs->get_guard() ? false : true; }
					return false;
				}

				if (real_lhs && not real_rhs) { return false; }
				if (not real_lhs && real_rhs) { return true; }

				const auto& lhs_types = lhs->types();
				const auto& rhs_types = rhs->types();

				const auto boxed_type = utility::make_type_info<kits::boxed_value>();
				const auto boxed_number_type = utility::make_type_info<kits::boxed_number>();

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

			/**
			 * @return a function object (boxed_value wrapper) if it exists.
			 * @throw std::range_error if it does not.
			 * @warning does not obtain a mutex lock.
			 */
			[[nodiscard]] std::pair<location_type::value_type, object_type> get_function_object(const std::string_view name, const std::size_t hint) const
			{
				const auto& functions = state_.boxed_functions;

				if (const auto it = functions.find(name, hint);
					it != functions.end()) { return {static_cast<location_type::value_type>(std::distance(functions.begin(), it)), it->second}; }
				throw std::range_error{"object not found"};
			}

		public:
			explicit dispatch_engine(parser::parser_base& parser)
				: parser_{parser} {}

			/**
			 * @brief casts an object while applying any dynamic_conversion available
			 */
			template<typename T>
			decltype(auto) boxed_cast(const kits::boxed_value& object) const
			{
				const kits::type_conversion_state state{conversion_manager_, conversion_manager_.get_conversion_saves()};
				return kits::boxed_cast<T>(object, &state);
			}

			/**
			 * @brief Registers a new named type.
			 */
			void add_type_info(const name_type& name, const utility::gal_type_info& ti)
			{
				add_global(std_format::format(type_name_format, name), kits::const_var(ti));

				utils::threading::unique_lock lock{mutex_};

				state_.types.emplace(name, ti);
			}

			/**
			 * @brief Add a new named proxy_function to the system.
			 * @throw name_conflict_error if there's a function matching the given one being added.
			 */
			void add_function(const name_type& name, const proxy_function& function)
			{
				utils::threading::unique_lock lock{mutex_};

				auto new_func = [&]() -> proxy_function
				{
					auto& functions = state_.functions;
					const auto it = functions.find(name);

					if (it != functions.end())
					{
						for (const auto& ref_fs = *it->second;
						     const auto& f: ref_fs) { if (*function == *f) { throw name_conflict_error{name}; } }

						auto copy_fs = *it->second;
						// tightly control vec growth
						copy_fs.reserve(copy_fs.size() + 1);
						copy_fs.push_back(function);
						std::ranges::stable_sort(copy_fs, &function_less_than);
						it->second = std::make_shared<std::decay_t<decltype(copy_fs)>>(copy_fs);
						return std::make_shared<dispatch_function>(std::move(copy_fs));
					}
					if (function->has_arithmetic_param())
					{
						// if the function is the only function, but it also contains
						// arithmetic operators, we must wrap it in a dispatch function
						// to allow for automatic arithmetic type conversions
						std::decay_t<decltype(*it->second)> fs;
						fs.reserve(1);
						fs.push_back(function);
						functions.emplace(name, std::make_shared<std::decay_t<decltype(fs)>>(fs));
						return std::make_shared<dispatch_function>(std::move(fs));
					}
					auto fs = std::make_shared<std::decay_t<decltype(*it->second)>>();
					fs->push_back(function);
					functions.emplace(name, fs);
					return function;
				}();

				state_.boxed_functions.insert_or_assign(name, const_var(new_func));
				state_.function_objects.insert_or_assign(name, std::move(new_func));
			}

			/**
			 * @brief Adds a new global (const) shared object, between all the threads.
			 */
			object_type& add_global(const name_type& name, const object_type& object)
			{
				if (not object.is_const()) { throw global_mutable_error{}; }

				utils::threading::unique_lock lock{mutex_};

				if (state_.global_objects.find(name) != state_.global_objects.end()) { throw name_conflict_error{name}; }
				return state_.global_objects.emplace(name, object).first->second;
			}

			/**
			 * @brief Add a new conversion for up-casting to a base class.
			 */
			void add_type_conversion(const kits::type_conversion_manager::conversion_type& conversion) { conversion_manager_.add(conversion); }

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			object_type& add_global_mutable(name_type name, object_type object)
			{
				utils::threading::unique_lock lock{mutex_};

				if (const auto [it, inserted] = state_.global_objects.emplace(std::move(name), std::move(object));
					not inserted) { throw name_conflict_error{it->first}; }
				else { return it->second; }
			}

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			object_type& add_global_mutable_no_throw(name_type name, object_type object)
			{
				utils::threading::unique_lock lock{mutex_};
				return state_.global_objects.emplace(std::move(name), std::move(object)).first->second;
			}

			/**
			 * @brief Updates an existing global shared object or adds a new global shared object if not found.
			 */
			void global_assign_or_insert(name_type name, object_type object)
			{
				utils::threading::unique_lock lock{mutex_};
				// todo: insert_or_assign
				if (const auto it = state_.global_objects.find(name); it == state_.global_objects.end()) { state_.global_objects.emplace_hint(it, std::move(name), std::move(object)); }
				else { it->second = std::move(object); }
			}

			void new_scope() { stack_holder_->new_scope(); }

			void pop_scope() noexcept { stack_holder_->pop_scope(); }

			void new_stack() { stack_holder_->new_stack(); }

			void pop_stack() noexcept { stack_holder_->pop_stack(); }

			/**
			 * @brief Set the value of an object, by name. If the object
			 * is not available in the current scope it is created.
			 */
			object_type& local_assign_or_insert(const name_type& name, object_type object) { return stack_holder_->add_variable(name, std::move(object)); }

			/**
			 * @brief Add a object, if this variable already exists in the current scope, an exception will be thrown.
			 */
			object_type& local_insert_or_throw(const name_type& name, object_type object) { return stack_holder_->add_variable_no_check(name, std::move(object)); }

			/**
			 * @brief Searches the current stack for an object of the given name
			 * includes a special overload for the _ place holder object to
			 * ensure that it is always in scope.
			 */
			[[nodiscard]] object_type get_object(std::string_view name, location_type& location, stack_holder& stack_holder) const
			{
				using enum_type = location_type::value_type;
				enum class location_t : enum_type
				{
					located = 0x80000000,
					is_local = 0x40000000,
					stack_mask = 0x0FFF0000,
					loc_mask = 0x0000FFFF
				};

				const enum_type loc = location;

				if (loc == 0)
				{
					auto& stack = stack_holder.recent_stack_data();

					// Is it in the stack?
					for (auto it_scope = stack.rbegin(); it_scope != stack.rend(); ++it_scope)
					{
						const auto it = std::ranges::find_if(
								*it_scope,
								[&name](const auto& pair) { return pair.first == name; });

						if (it != it_scope->end())
						{
							location = utils::set_enum_flag_ret(
									static_cast<enum_type>(std::ranges::distance(stack.rbegin(), it_scope)) << 16 | static_cast<enum_type>(std::ranges::distance(it_scope->begin(), it)),
									location_t::located,
									location_t::is_local);
							return it->second;
						}
					}

					location = static_cast<enum_type>(location_t::located);
				}
				else if (utils::check_any_enum_flag(loc, location_t::is_local))
				{
					// todo
					auto& stack = stack_holder.recent_stack_data();

					return stack[
						stack.size() - 1 - (utils::filter_enum_flag_ret(loc, location_t::stack_mask) >> 16)
					].at(utils::filter_enum_flag_ret(loc, location_t::loc_mask));
				}

				// Is the value we are looking for a global or function?
				utils::threading::shared_lock lock{mutex_};

				// todo: transparent
				if (const auto it = state_.global_objects.find(name_type{name});
					it != state_.global_objects.end()) { return it->second; }

				// no? is it a function object?
				auto&& [l, func] = get_function_object(name, loc);
				if (l != loc) { location = l; }
				return func;
			}

			/**
			 * @brief Returns the type info for a named type.
			 */
			[[nodiscard]] utility::gal_type_info get_type_info(const std::string_view name, const bool throw_if_not_exist = true) const
			{
				utils::threading::shared_lock lock{mutex_};

				// todo: transparent
				if (const auto it = state_.types.find(name_type{name});
					it != state_.types.end()) { return it->second; }

				if (throw_if_not_exist) { throw std::range_error{"type not exist"}; }
				return {};
			}

			/**
			 * @brief Returns the registered name of a known type_info object
			 * compares the "bare_type_info" for the broadest possible match.
			 */
			[[nodiscard]] name_type get_type_name(const utility::gal_type_info& ti) const
			{
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = std::ranges::find_if(
							state_.types,
							[&ti](const auto& t) { return t.bare_equal(ti); },
							[](const auto& pair) { return pair.second; });
					it != state_.types.end()) { return it->first; }

				return ti.bare_name();
			}

			[[nodiscard]] name_type get_type_name(const object_type& object) const { return get_type_name(object.type_info()); }

			/**
			 * @brief Return true if a function exists.
			 */
			[[nodiscard]] bool has_function(const std::string_view name) const
			{
				utils::threading::shared_lock lock{mutex_};
				return state_.functions.contain(name);
			}

			/**
			 * @brief Return a function by name.
			 */
			[[nodiscard]] std::pair<location_type::value_type, state_type::functions_type::mapped_type> get_function(const std::string_view name, const std::size_t hint) const
			{
				utils::threading::shared_lock lock{mutex_};

				const auto& functions = state_.functions;

				if (const auto it = functions.find(name, hint);
					it != functions.end()) { return {static_cast<location_type::value_type>(std::ranges::distance(functions.begin(), it)), it->second}; }
				return {0, std::make_shared<state_type::functions_type::mapped_type::element_type>()};
			}

			[[nodiscard]] state_type::functions_type::mapped_type get_method_missing_functions() const
			{
				const location_type::value_type loc = method_missing_location_;
				auto&& [l, functions] = get_function(method_missing_name, method_missing_location_);
				if (l != loc) { method_missing_location_ = l; }

				return std::move(functions);
			}

			/**
			 * @return a function object (boxed_value wrapper) if it exists.
			 * @throw std::range_error if it does not.
			 */
			[[nodiscard]] object_type get_function_object(const std::string_view name) const
			{
				utils::threading::shared_lock lock{mutex_};
				return get_function_object(name, 0).second;
			}

			/**
			 * @brief Get a map of all objects that can be seen from the current scope in a scripting context.
			 */
			[[nodiscard]] state_type::global_objects_type get_scripting_objects() const
			{
				// We don't want the current context, but one up if it exists
				const auto& stack = (stack_holder_->stack.size() == 1) ? stack_holder_->recent_stack_data() : stack_holder_->recent_parent_stack_data();

				state_type::global_objects_type ret{};

				// note: map insert doesn't overwrite existing values, which is why this works
				std::ranges::for_each(
						stack.rbegin(),
						stack.rend(),
						[&ret](const auto& scope) { ret.insert(scope.begin(), scope.end()); });

				// add the global values
				utils::threading::shared_lock lock{mutex_};
				ret.insert(state_.global_objects.begin(), state_.global_objects.end());

				return ret;
			}

			/**
			 * @brief Get a vector of all registered functions.
			 */
			[[nodiscard]] auto get_functions() const
			{
				utils::threading::shared_lock lock{mutex_};

				const auto& functions = state_.functions;

				std::vector<state_type::function_objects_type::value_type> ret{};

				std::ranges::for_each(
						functions,
						[&ret](const auto& pair)
						{
							std::ranges::for_each(
									*pair.second,
									[&ret, &pair](const auto& function) { ret.emplace_back(pair.first, function); });
						});

				return ret;
			}

			/**
			 * @brief Get a map of all functions that can be seen from a scripting context.
			 */
			[[nodiscard]] state_type::global_objects_type get_function_objects() const
			{
				const auto& functions = state_.function_objects;

				state_type::global_objects_type ret{};

				std::ranges::for_each(
						functions,
						[&ret](const auto& function) { ret.emplace(function.first, kits::const_var(function.second)); });

				return ret;
			}

			/**
			 * @brief Return all registered types.
			 */
			template<template<typename...> typename Container, typename... AnyOther>
				requires std::is_constructible_v<Container<type_name_map_type::value_type, AnyOther...>, type_name_map_type::const_iterator, type_name_map_type::const_iterator>
			[[nodiscard]] Container<type_name_map_type::value_type, AnyOther...> copy_types() const
			{
				utils::threading::shared_lock lock{mutex_};
				return Container<type_name_map_type::value_type, AnyOther...>{state_.types.begin(), state_.types.end()};
			}

			[[nodiscard]] state_type copy_state() const
			{
				utils::threading::shared_lock lock{mutex_};
				return state_;
			}

			void set_state(const state_type& state)
			{
				utils::threading::unique_lock lock{mutex_};
				state_ = state;
			}

			void set_state(state_type&& state)
			{
				utils::threading::unique_lock lock{mutex_};
				state_ = std::move(state);
			}

			void emit_call() { stack_holder_->emit_call(conversion_manager_.get_conversion_saves()); }

			void finish_call() noexcept { stack_holder_->finish_call(conversion_manager_.get_conversion_saves()); }

			static bool is_attribute_call(
					const dispatch_function::functions_type& functions,
					const kits::function_parameters& params,
					const bool has_param,
					const kits::type_conversion_state& conversion
					) noexcept
			{
				if (not has_param || params.empty()) { return false; }

				return std::ranges::any_of(
						functions,
						[&params, &conversion](const auto& function) { return function->is_attribute_function() && function->is_first_type_match(params.front(), conversion); });
			}

			[[nodiscard]] object_type call_member(
					const name_type& name,
					location_type& location,
					const kits::function_parameters& params,
					const bool has_params,
					const kits::type_conversion_state& conversion
					)
			{
				const location_type::value_type loc = location;
				const auto& [l, functions] = get_function(name, loc);
				if (l != loc) { location = l; }

				const auto do_attribute_call = [this, &conversion](
						const kits::arity_error::size_type num_params,
						const kits::function_parameters& ps,
						const dispatch_function::functions_type& fs) -> object_type
				{
					const kits::function_parameters attribute_params{ps.begin(), ps.begin() + num_params};
					object_type object = dispatch(fs, attribute_params, conversion);
					if (num_params < static_cast<kits::arity_error::size_type>(ps.size()) || object.type_info().bare_equal(utility::make_type_info<kits::proxy_function_base>()))
					{
						auto guard = stack_holder_->make_temp_scope(ps.front());

						try
						{
							const auto function = boxed_cast<const kits::proxy_function_base*>(object);
							try { return (*function)({ps.begin() + num_params, ps.end()}, conversion); }
							catch (const kits::bad_boxed_cast&) { }
							catch (const kits::arity_error&) { }
							catch (const kits::guard_error&) { }
							throw kits::dispatch_error{
									{ps.begin() + num_params, ps.end()},
									{boxed_cast<const_proxy_function>(object)}};
						}
						catch (const kits::bad_boxed_cast&)
						{
							// unable to convert bv into a proxy_function_base
							throw kits::dispatch_error{
									{ps.begin() + num_params, ps.end()},
									{fs.begin(), fs.end()}};
						}
					}
					return object;
				};

				if (is_attribute_call(*functions, params, has_params, conversion)) { return do_attribute_call(1, params, *functions); }

				std::exception_ptr current_exception;

				if (not functions->empty())
				{
					try { return kits::dispatch(*functions, params, conversion); }
					catch (kits::dispatch_error&) { current_exception = std::current_exception(); }
				}

				// If we get here we know that either there was no method with that name,
				// or there was no matching method

				const auto missing_functions = [this, &params, &conversion]
				{
					dispatch_function::functions_type ret{};

					const auto mmf = get_method_missing_functions();

					std::ranges::for_each(
							*mmf,
							[&ret, &params, &conversion](const auto& f) { if (f->is_first_type_match(params.front(), conversion)) { ret.push_back(f); } });

					return ret;
				}();

				const bool is_no_param = std::ranges::all_of(
						missing_functions,
						[](const auto& f) { return f->get_arity() == 2; });

				if (not missing_functions.empty())
				{
					try
					{
						if (is_no_param)
						{
							auto tmp_params = params.to<std::vector>();
							tmp_params.insert(tmp_params.begin() + 1, kits::var(name));
							return do_attribute_call(2, kits::function_parameters{tmp_params}, missing_functions);
						}
						const std::array tmp_params{params.front(), kits::var(name), var(std::vector(params.begin() + 1, params.end()))};
						return dispatch(missing_functions, kits::function_parameters{tmp_params}, conversion);
					}
					catch (const kits::option_explicit_error& e)
					{
						throw kits::dispatch_error{
								params,
								{functions->begin(), functions->end()},
								e.what()};
					}
				}

				// If we get all the way down here we know there was no "method_missing"
				// method at all.
				if (current_exception) { std::rethrow_exception(current_exception); }
				throw kits::dispatch_error{
						params,
						{functions->begin(), functions->end()}};
			}

			[[nodiscard]] object_type call_function(
					const std::string_view name,
					location_type& location,
					const kits::function_parameters& params,
					const kits::type_conversion_state& conversion
					) const
			{
				const location_type::value_type loc = location;
				const auto [l, functions] = get_function(name, loc);
				if (l != loc) { location = l; }

				return dispatch(*functions, params, conversion);
			}

			/**
			 * @brief Dump type info.
			 */
			[[nodiscard]] name_type dump_type(const utility::gal_type_info& ti) const
			{
				return std_format::format(
						"[{}]{}",
						ti.is_const() ? "immutable" : "mutable",
						get_type_name(ti)
						);
			}

			/**
			 * @brief Dump object info.
			 */
			[[nodiscard]] name_type dump_object(const object_type& object) const
			{
				return std_format::format(
						"[{}]{}",
						object.is_const() ? "immutable" : "mutable",
						get_type_name(object)
						);
			}

			/**
			 * @brief Dump function.
			 */
			[[nodiscard]] name_type dump_function(const state_type::function_objects_type::value_type& pair) const
			{
				const auto [name, function] = pair;

				const auto& types = function->types();

				auto context = dump_type(types.front());
				context.reserve(types.size() * 64);

				context.append(" ").append(name).append("(");
				for (auto it = types.begin() + 1; it != types.end(); ++it)
				{
					context.append(dump_type(*it));

					if (it != types.end()) { context.append(", "); }
				}
				context.append(")");

				return context;
			}

			[[nodiscard]] const kits::type_conversion_manager& get_conversion_manager() const noexcept { return conversion_manager_; }

			[[nodiscard]] parser::parser_base& get_parser() const noexcept { return parser_.get(); }

			[[nodiscard]] stack_holder& get_stack_holder() noexcept { return stack_holder_.operator*(); }
		};
	}
}

#endif // GAL_LANG_KITS_DISPATCH_HPP
