#pragma once

#ifndef GAL_LANG_KITS_DISPATCH_HPP
#define GAL_LANG_KITS_DISPATCH_HPP

#include<defines.hpp>
#include<kits/boxed_value.hpp>
#include<kits/boxed_value_cast.hpp>
#include<kits/dynamic_object.hpp>
#include<kits/proxy_function.hpp>
#include<kits/proxy_constructor.hpp>

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

		using type_info_type = kits::detail::gal_type_info;
		using function_type = proxy_function;
		using variable_type = kits::boxed_value;
		using evaluation_type = std::string;
		using type_conversion_type = kits::type_conversion_type;

		using type_infos_type = std::vector<std::pair<type_info_type, name_type>>;
		using functions_type = std::vector<std::pair<function_type, name_type>>;
		using globals_type = std::vector<std::pair<variable_type, name_type>>;
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
		engine_module& add_type_info(type_info_type ti, name_type name)
		{
			type_infos_.emplace_back(ti, std::move(name));
			return *this;
		}

		engine_module& add_function(function_type function, name_type name)
		{
			functions_.emplace_back(std::move(function), std::move(name));
			return *this;
		}

		engine_module& add_global(variable_type variable, name_type name)
		{
			if (not variable.is_const()) { throw global_mutable_error{}; }

			globals_.emplace_back(std::move(variable), std::move(name));
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
					[&](const auto& func) { return func.second == name && *func.first == *function; });
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
								if (lhs != rhs) { return utils::make_type_info<kits::boxed_value>(); }
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

			template<typename T>
			using internal_stack_type = std::vector<T>;

			using name_type = engine_module::name_type;
			using variable_type = kits::boxed_value;

			using scope_type = utils::flat_hash_map<name_type, variable_type>;
			using stack_data_type = internal_stack_type<scope_type>;
			using stack_type = internal_stack_type<stack_data_type>;
			using param_list_type = internal_stack_type<variable_type>;
			using param_lists_type = internal_stack_type<param_list_type>;
			using call_depth_type = int;

			stack_type stack;
			param_lists_type param_lists;
			call_depth_type depth;

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
			void pop_scope()
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

			void finish_stack()
			{
				gal_assert(not stack.empty());
				stack.pop_back();
			}

			void finish_scope()
			{
				gal_assert(not recent_stack_data().empty());
				recent_stack_data().pop_back();
			}

			void finish_call()
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
			using name_type = engine_module::name_type;
			using object_type = stack_holder::variable_type;

			using type_name_map_type = utils::unordered_hash_map<name_type, kits::detail::gal_type_info>;
			using scope_type = stack_holder::scope_type;
			using stack_data_type = stack_holder::stack_data_type;
			using location_type = std::atomic_uint_fast32_t;

			struct state_type
			{
				using functions_type = utils::flat_hash_map<name_type, std::shared_ptr<dispatch_function::functions_type>>;
				using function_objects_type = utils::flat_hash_map<name_type, proxy_function>;
				using boxed_functions_type = utils::flat_hash_map<name_type, object_type>;
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

				const auto boxed_type = utils::make_type_info<kits::boxed_value>();
				const auto boxed_number_type = utils::make_type_info<kits::boxed_number>();

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
			// void add_type_info(const kits::detail::gal_type_info& ti, const name_type& name) { }

			/**
			 * @brief Add a new named proxy_function to the system.
			 * @throw name_conflict_error if there's a function matching the given one being added.
			 */
			void add_function(const proxy_function& function, const name_type& name)
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
			object_type& add_global(const object_type& object, const name_type& name)
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
			object_type& add_global_mutable(object_type object, name_type name)
			{
				utils::threading::unique_lock lock{mutex_};

				if (const auto [it, inserted] = state_.global_objects.emplace(std::move(name), std::move(object));
					not inserted) { throw name_conflict_error{it->first}; }
				else { return it->second; }
			}

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			object_type& add_global_mutable_no_throw(object_type object, name_type name)
			{
				utils::threading::unique_lock lock{mutex_};
				return state_.global_objects.emplace(std::move(name), std::move(object)).first->second;
			}

			/**
			 * @brief Updates an existing global shared object or adds a new global shared object if not found.
			 */
			void global_assign_or_insert(object_type object, name_type name)
			{
				utils::threading::unique_lock lock{mutex_};
				// todo: insert_or_assign
				if (const auto it = state_.global_objects.find(name); it == state_.global_objects.end()) { state_.global_objects.emplace_hint(it, std::move(name), std::move(object)); }
				else { it->second = std::move(object); }
			}

			/**
			 * @brief Set the value of an object, by name. If the object
			 * is not available in the current scope it is created.
			 */
			object_type& object_assign_or_insert(const name_type& name, object_type object) { return stack_holder_->add_variable(name, std::move(object)); }

			/**
			 * @brief Add a object, if this variable already exists in the current scope, an exception will be thrown.
			 */
			object_type& object_insert_or_throw(const name_type& name, object_type object) { return stack_holder_->add_variable_no_check(name, std::move(object)); }

			object_type get_object(std::string_view name, location_type& location, stack_holder& stack_holder) const
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
					throw std::runtime_error{"todo"};
				}

				// Is the value we are looking for a global or function?
				utils::threading::shared_lock lock{mutex_};

				// todo: transparent
				if (const auto it = state_.global_objects.find(name_type{name});
					it != state_.global_objects.end()) { return it->second; }

				// no? is it a function object?
				// todo
				throw std::runtime_error{"todo"};
			}

			state_type copy_state() const
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

			[[nodiscard]] parser::parser_base& get_parser() const noexcept { return parser_.get(); }

			[[nodiscard]] stack_holder& get_stack_holder() noexcept { return stack_holder_.operator*(); }
		};
	}
}

#endif // GAL_LANG_KITS_DISPATCH_HPP
