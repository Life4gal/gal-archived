#pragma once

#ifndef GAL_LANG_FOUNDATION_DISPATCHER_HPP
#define GAL_LANG_FOUNDATION_DISPATCHER_HPP

#include <gal/boxed_cast.hpp>
#include <gal/boxed_value.hpp>
#include <gal/foundation/dynamic_object.hpp>
#include <gal/foundation/function_proxy.hpp>
#include <gal/foundation/string_pool.hpp>
#include <gal/foundation/name.hpp>
#include <utils/utility_base.hpp>

namespace gal::lang
{
	namespace ast
	{
		// see gal/foundation/ast.hpp
		class ast_parser_base;
	}

	namespace exception
	{
		/**
		 * @brief Exception thrown in the case that an object name is invalid because it is a reserved word.
		 */
		class reserved_word_error final : public std::runtime_error
		{
		private:
			foundation::string_type word_;

		public:
			explicit reserved_word_error(foundation::string_type word)
				: std::runtime_error{
						  std_format::format("'{}' is a reserved word and not allowed in object name", word)},
				  word_{std::move(word)} {}

			explicit reserved_word_error(const foundation::string_view_type word)
				: reserved_word_error{foundation::string_type{word}} {}

			[[nodiscard]] foundation::string_view_type which() const noexcept { return word_; }
		};

		/**
		 * @brief Exception thrown in the case that an object name is invalid because it contains illegal characters.
		 */
		class illegal_name_error final : public std::runtime_error
		{
		private:
			foundation::string_type name_;

		public:
			explicit illegal_name_error(foundation::string_type name)
				: std::runtime_error{
						  std_format::format("'{}' is a illegal name and not allowed in object name", name)},
				  name_{std::move(name)} {}

			explicit illegal_name_error(const foundation::string_view_type name)
				: illegal_name_error{foundation::string_type{name}} {}

			[[nodiscard]] foundation::string_view_type which() const noexcept { return name_; }
		};

		/**
		 * @brief Exception thrown in the case that an object name is invalid because it already exists in current context.
		 */
		class name_conflict_error final : public std::runtime_error
		{
		private:
			foundation::string_type name_;

		public:
			explicit name_conflict_error(foundation::string_type name)
				: std::runtime_error{
						  std_format::format("'{}' is already defined in the current context", name)},
				  name_{std::move(name)} {}

			explicit name_conflict_error(const foundation::string_view_type name)
				: name_conflict_error{foundation::string_type{name}} {}

			[[nodiscard]] foundation::string_view_type which() const noexcept { return name_; }
		};

		class global_mutable_error final : public std::runtime_error
		{
		private:
			foundation::string_type name_;

		public:
			explicit global_mutable_error(foundation::string_type name)
				: std::runtime_error{
						  std_format::format("global variable '{}' must be immutable", name)},
				  name_{std::move(name)} {}

			explicit global_mutable_error(const foundation::string_view_type name)
				: global_mutable_error{foundation::string_type{name}} {}

			[[nodiscard]] foundation::string_view_type which() const noexcept { return name_; }
		};
	}// namespace exception

	namespace foundation
	{
		/**
		 * @brief Holds a collection of settings which can be applied to the runtime.
		 * @note Used to implement loadable module support.
		 */
		class engine_module
		{
		public:
			// name <=> type_info
			using type_infos_type = std::map<string_view_type, gal_type_info, std::less<>>;
			// name <=> function
			using functions_type = std::multimap<string_view_type, function_proxy_type, std::less<>>;
			// name <=> "global" object
			// note: module level object is global visible
			using objects_type = std::map<string_view_type, boxed_value, std::less<>>;
			// evaluation string
			using evaluations_type = std::vector<string_view_type>;
			// convertor
			using convertors_type = std::set<convertor_type, std::less<>>;

		private:
			// module string pool, store all name
			// todo: GAL will copy all type_info, function, object, evaluation, converter when loading a module,
			// but their names are stored in the string_pool of the target module,
			// GAL will not copy them, we need a way to ensure that we will correctly copy all the names when the module is destroyed!
			string_pool_type pool_;

			type_infos_type types_;
			functions_type functions_;
			objects_type objects_;
			evaluations_type evaluations_;
			convertors_type convertors_;

		public:
			/**
			 * @brief Registers a new named type.
			 */
			engine_module& add_type_info(
					const string_view_type name,
					gal_type_info type
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					)
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add a type_info '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							types_.contains(name) ? "but it was already exist" : "add successed");)

				if (const auto it = types_.find(name);
					it == types_.end()) { types_.emplace_hint(it, pool_.append(name), type); }

				return *this;
			}

			engine_module& add_function(
					const string_view_type name,
					function_proxy_type function
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add a function '{}', {}.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							functions_.contains(name) ? "there are already some functions with the same name" : "this is the first function of this name");)

				functions_.emplace(pool_.append(name), std::move(function));

				return *this;
			}

			engine_module& add_object(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add an global object '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							not object.is_const() ? "but it is not a const object" : objects_.contains(name) ? "but it was already exist" : "add successed");)

				if (not object.is_const()) { throw exception::global_mutable_error{name}; }

				if (const auto it = objects_.find(name);
					it == objects_.end()) { objects_.emplace_hint(it, pool_.append(name), std::move(object)); }

				return *this;
			}

			engine_module& add_evaluation(
					const string_view_type evaluation
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add a evaluation '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							evaluation,
							std::ranges::find(evaluations_, evaluation) != evaluations_.end() ? "there is the same evaluation (still add successed)" : "add successed");)

				evaluations_.push_back(evaluation);

				return *this;
			}

			engine_module& add_convertor(
					convertor_type convertor
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				const auto [it, result] = convertors_.emplace(std::move(convertor));
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add a convertor convert from '{}({})' to '{}({}), {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							(*it)->from().name(),
							(*it)->from().bare_name(),
							(*it)->to().name(),
							(*it)->to().bare_name(),
							not result ? "but it was already exist" : "add successed");
						)

				return *this;
			}

		private:
			template<bool Takeover, typename Engine, typename Dispatcher>
			void do_load_module(Engine& engine, Dispatcher& dispatcher)
			{
				//*********************
				//****  TYPE_INFO  ****
				//*********************

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type type_info_detail{};
						std::ranges::for_each(
							types_ | std::views::keys,
							[&type_info_detail](const auto& key)
							{
							type_info_detail.append(key).push_back('\n');
							});
						utils::logger::debug("There are currently {} type_info(s), details:\n\t{}", types_.size(), type_info_detail);)

				std::ranges::for_each(
						types_,
						[&dispatcher](const auto& type)
						{
							try { dispatcher.add_type_info(type.first, type.second); }
							catch (const exception::name_conflict_error&)
							{
								// todo: Should we throw an error if there's a name conflict while applying a module?
							}
						});

				//*********************
				//****  FUNCTION  ****
				//*********************

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type function_detail{};
						std::ranges::for_each(
							functions_ | std::views::keys,
							[&function_detail](const auto& key)
							{
							function_detail.append(key).push_back('\n');
							});
						utils::logger::debug("There are currently {} function(s), details:\n\t{}", functions_.size(), function_detail);)

				std::ranges::for_each(
						functions_,
						[&dispatcher](auto&& function)
						{
							try
							{
								if constexpr (Takeover) { dispatcher.add_function(function.first, std::move(function.second)); }
								else { dispatcher.add_function(function.first, function.second); }
							}
							catch (const exception::name_conflict_error&)
							{
								// todo: Should we throw an error if there's a name conflict while applying a module?
							}
						});

				//*******************
				//****  OBJECT  ****
				//*******************

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type object_detail{};
						std::ranges::for_each(
							objects_ | std::views::keys,
							[&object_detail](const auto& key)
							{
							object_detail.append(key).push_back('\n');
							});
						utils::logger::debug("There are currently {} object(s), details:\n\t{}", objects_.size(), object_detail);)

				std::ranges::for_each(
						objects_,
						[&dispatcher](auto&& object)
						{
							if constexpr (Takeover) { dispatcher.add_global(object.first, std::move(object.second)); }
							else { dispatcher.add_global(object.first, object.second); }
						});

				//************************
				//****  EVALUATION  ****
				//************************

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						string_type evaluation_detail{};
						std::ranges::for_each(
							evaluations_,
							[&evaluation_detail](const auto& key)
							{
							evaluation_detail.append(key).push_back('\n');
							});
						utils::logger::debug("There are currently {} evaluation(s), details:\n\t{}", evaluations_.size(), evaluation_detail);)

				std::ranges::for_each(
						evaluations_,
						[&engine](auto&& evaluation) { (void)engine.eval(evaluation); });

				//************************
				//****  CONVERTOR  ****
				//************************

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::debug("There are currently {} convertors(s)", convertors_.size());)

				std::ranges::for_each(
						convertors_,
						[&dispatcher]<typename Convertor>(Convertor&& convertor)
						{
							if constexpr (Takeover) { dispatcher.add_convertor(std::forward<Convertor>(convertor)); }
							else { dispatcher.add_convertor(convertor); }
						});

				if constexpr (Takeover) { dispatcher.takeover_pool(std::move(pool_)); }
				else {}
			}

		public:
			/**
			 * @brief Load a module, copy all type_info, function, object, evaluation, converter, shared their name.
			 *
			 * @note All names of the modules' type_info, function, object, evaluation, converter will be invalid if module been destroyed.
			 */
			template<typename Engine, typename Dispatcher>
			void borrow(Engine& engine, Dispatcher& dispatcher) { this->do_load_module<false>(engine, dispatcher); }

			/**
			 * @brief Load a module, move away all type_info, function, object, evaluation, converter, also take over the module string_pool, then the module can be safely destroyed.
			 *
			 * @note DO NOT USE THE MODULE AFTER IT BEEN TAKEN
			 */
			template<typename Engine, typename Dispatcher>
			void take(Engine& engine, Dispatcher& dispatcher) && { this->do_load_module<true>(engine, dispatcher); }
		};

		using engine_module_type = std::shared_ptr<engine_module>;
		/**
		 * @brief Signature of module entry point that all binary loadable modules must implement.
		 */
		using engine_module_maker = engine_module_type(*)();
		[[nodiscard]] inline engine_module_type make_engine_module() { return std::make_shared<engine_module>(); }

		/**
		 * @brief A function_proxy implementation that is able to take
		 * a vector of function_proxies and perform a dispatch on them.
		 * It is used specifically in the case of dealing with function object variables.
		 */
		class dispatch_function final : public function_proxy_base
		{
		public:
			using functions_type = function_proxies_type;

		private:
			functions_type functions_;

			[[nodiscard]] static type_infos_type build_param_types(const function_proxies_view_type functions)
			{
				if (functions.empty()) { return {}; }

				// the first one's types
				const auto types = functions.front()->type_view();

				type_infos_type copy_types{types.begin(), types.end()};
				bool size_mismatch = false;

				std::ranges::for_each(
						// skip the first one
						functions | std::views::drop(1),
						[&copy_types, &size_mismatch](const auto tv)
						{
							if (tv.size() != copy_types.size()) { size_mismatch = true; }

							const auto size = static_cast<decltype(copy_types)::difference_type>(std::ranges::min(copy_types.size(), tv.size()));
							std::ranges::transform(
									copy_types | std::views::take(size),
									tv,
									copy_types.begin(),
									[](auto&& lhs, auto&& rhs)
									{
										if (lhs != rhs) { return boxed_value::class_type(); }
										return lhs;
									});
						},
						[](const auto& function) { return function->type_view(); });

				gal_assert(not copy_types.empty(), "type_info vector is empty, this is only possible if something else is broken");

				if (size_mismatch) { copy_types.resize(1); }

				return copy_types;
			}

			static arity_size_type calculate_arity(const function_proxies_view_type functions) noexcept
			{
				if (functions.empty() || std::ranges::any_of(
						    // skip the first one
						    functions | std::views::drop(1),
						    [arity = functions.front()->arity_size()](const auto a) { return a != arity; },
						    [](const auto& function) { return function->arity_size(); })) { return no_parameters_arity; }

				return functions.front()->arity_size();
			}

			[[nodiscard]] boxed_value do_invoke(const parameters_view_type params, const convertor_manager_state& state) const override { return dispatch(functions_, params, state); }

		public:
			explicit dispatch_function(functions_type functions)
				: function_proxy_base{
						  build_param_types(functions),
						  calculate_arity(functions)},
				  functions_{std::move(functions)} {}

			[[nodiscard]] const_function_proxies_type overloaded_functions() const override { return {functions_.begin(), functions_.end()}; }

			[[nodiscard]] bool operator==(const function_proxy_base& other) const noexcept override
			{
				if (const auto* func = dynamic_cast<const dispatch_function*>(&other)) { return func->functions_ == functions_; }
				return false;
			}

			[[nodiscard]] bool match(const parameters_view_type params, const convertor_manager_state& state) const override
			{
				return std::ranges::any_of(
						functions_,
						[params, &state](const auto& function) { return function->match(params, state); });
			}
		};

		struct engine_stack
		{
			using scope_type = std::map<string_view_type, boxed_value, std::less<>>;
			using stack_type = std::vector<scope_type>;
			using stacks_type = std::vector<stack_type>;

			using parameters_list_type = std::vector<parameters_type>;

			using call_depth_type = int;

		private:
			std::reference_wrapper<string_pool_type> borrowed_pool_;
			std::vector<string_pool_type::block_borrower> borrowed_block_;

		public:
			stacks_type stacks;
			parameters_list_type parameters_list;
			call_depth_type depth;

			[[nodiscard]] stack_type& recent_stack() noexcept { return stacks.back(); }

			[[nodiscard]] const stack_type& recent_stack() const noexcept { return stacks.back(); }

			[[nodiscard]] stack_type& recent_parent_stack() noexcept { return stacks[stacks.size() - 2]; }

			[[nodiscard]] const stack_type& recent_parent_stack() const noexcept { return stacks[stacks.size() - 2]; }

			[[nodiscard]] scope_type& recent_scope() noexcept { return recent_stack().back(); }

			[[nodiscard]] const scope_type& recent_scope() const noexcept { return recent_stack().back(); }

			[[nodiscard]] parameters_type& recent_call() noexcept { return parameters_list.back(); }

			[[nodiscard]] const parameters_type& recent_call() const noexcept { return parameters_list.back(); }

		private:
			[[nodiscard]] string_pool_type::block_borrower& recent_borrowed_block() noexcept { return borrowed_block_.back(); }

			void prepare_new_stack()
			{
				// add a new stack with 1 element
				stacks.emplace_back(1);
				// todo: if a variable is in the scope of the global variable but the user declares the variable directly with 'var' instead of 'global', it will result in the absence of borrowed_block_ to register the variable, and then the vector goes out of bounds (there is no back), how to resolve the conflict?
				borrowed_block_.emplace_back(borrowed_pool_.get());
			}

			void finish_stack() noexcept { stacks.pop_back(); }

			void prepare_new_scope()
			{
				recent_stack().emplace_back();
				borrowed_block_.emplace_back(borrowed_pool_.get());
			}

			void finish_scope() noexcept
			{
				recent_stack().pop_back();
				borrowed_block_.pop_back();
			}

			void prepare_new_call() { parameters_list.emplace_back(); }

			void finish_call() { parameters_list.pop_back(); }

		public:
			explicit engine_stack(string_pool_type& pool)
				: borrowed_pool_{pool},
				  depth{0}
			{
				prepare_new_stack();
				prepare_new_call();
			}

			[[nodiscard]] constexpr bool is_root() const noexcept { return depth == 0; }

			/**
			 * @brief Pushes a new stack on to the list of stacks.
			 */
			void new_stack(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current()
							))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to push a new stack because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				prepare_new_stack();
			}

			/**
			 * @brief Pops a new stack on to the list of stacks.
			 */
			void pop_stack(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to pop a stack because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				finish_stack();
			}

			/**
			 * @brief Adds a new scope to the stack.
			 */
			void new_scope(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to push a new scope because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				prepare_new_scope();
				prepare_new_call();
			}

			/**
			 * @brief Pops the current scope from the stack.
			 */
			void pop_scope(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current())) noexcept
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to pop a scope because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				finish_call();
				finish_scope();
			}

			/**
			 * @brief Adds a named object to the current scope.
			 * @note This version does not check the validity of the name.
			 *
			 * @throw exception::name_conflict_error object already exists
			 */
			boxed_value& add_object_no_check(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add object '{}' without check, {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							recent_scope().contains(name) ? "but it already exists." : "add successed");)

				auto& scope = recent_scope();
				if (const auto it = scope.find(name);
					it == scope.end()) { return scope.emplace(recent_borrowed_block().append(name), std::move(object)).first->second; }

				throw exception::name_conflict_error{name};
			}

			/**
			 * @brief Adds a named object to the current scope, assign it if it already exist.
			 */
			boxed_value& add_object(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add object '{}' with check, {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							std::ranges::any_of(recent_stack() | std::views::reverse, [name](const auto scope)
								{ return scope.contains(name); })
							? "but it already exists."
							: "add successed");)

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						int scope_no = 0;)

				for (auto& stack = recent_stack();
				     auto& scope: stack | std::views::reverse)
				{
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							utils::logger::info("searching object '{}' in '{}'th scope",
								name,
								scope_no);)
					if (const auto it = scope.find(name);
						it != scope.end())
					{
						GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
								utils::logger::info("found object '{}' in '{}'th scope",
									name,
									scope_no);)

						return it->second = std::move(object);
					}

					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							++scope_no);
				}

				return add_object_no_check(name, std::move(object) GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location));
			}

			void push_params(
					parameters_type&& params
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), with '{}' params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							params.size());)

				auto& current_call = recent_call();
				current_call.insert(current_call.end(), std::make_move_iterator(params.begin()), std::make_move_iterator(params.end()));
			}

			void push_params(
					const parameters_view_type params
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' params from (file: '{}' function: '{}' position: ({}:{})), with '{}' params.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							params.size());)

				auto& current_call = recent_call();
				current_call.insert(current_call.end(), params.begin(), params.end());
			}

			void pop_params(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "no reason",
							const std_source_location& location = std_source_location::current())) noexcept
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), clear recent call's params because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)

				recent_call().clear();
			}

			void emit_call(
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std::string_view reason = "emit a call",
							const std_source_location& location = std_source_location::current()))
			{
				if (is_root()) { state->enable_conversion_saves(true); }
				++depth;

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})) because '{}', current depth: '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason,
							depth);)

				push_params(state->exchange_conversion_saves());
			}

			void finish_call(
					const convertor_manager_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std::string_view reason = "finish a call",
							const std_source_location& location = std_source_location::current())) noexcept
			{
				--depth;
				gal_assert(depth >= 0);

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})) because '{}', current depth: '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason,
							depth);)

				if (is_root())
				{
					pop_params(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(reason, location));
					state->enable_conversion_saves(false);
				}
			}
		};

		class dispatcher;

		class dispatcher_state
		{
		public:
			using dispatcher_type = std::reference_wrapper<dispatcher>;

		private:
			dispatcher_type d_;
			convertor_manager_state state_;

		public:
			explicit dispatcher_state(dispatcher& d);

			[[nodiscard]] dispatcher_type::type* operator->() const noexcept { return &d_.get(); }

			[[nodiscard]] dispatcher_type::type& operator*() const noexcept { return d_.get(); }

			[[nodiscard]] engine_stack& stack(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(const std_source_location& location = std_source_location::current())) const noexcept;

			[[nodiscard]] const convertor_manager_state& convertor_state() const noexcept { return state_; }
		};

		struct scoped_scope : utils::scoped_base<scoped_scope, std::reference_wrapper<const dispatcher_state>>
		{
			friend struct scoped_base<scoped_scope, std::reference_wrapper<const dispatcher_state>>;

			explicit scoped_scope(
					const dispatcher_state& s
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const string_view_type reason = "initial a new scoped_scope",
							const std_source_location& location = std_source_location::current()))
				: scoped_base{s}
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), create a scoped_scope because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)
			}

		private:
			void do_construct() const;
			void do_destruct() const;
		};

		struct scoped_object_scope : scoped_scope
		{
			friend struct scoped_base<scoped_scope, std::reference_wrapper<const dispatcher_state>>;

			scoped_object_scope(
					const dispatcher_state& s,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							string_view_type reason = "initial a new scoped_object_scope",
							const std_source_location& location = std_source_location::current()));
		};

		struct scoped_stack_scope : utils::scoped_base<scoped_stack_scope, std::reference_wrapper<const dispatcher_state>>
		{
			friend struct scoped_base<scoped_stack_scope, std::reference_wrapper<const dispatcher_state>>;

			explicit scoped_stack_scope(
					const dispatcher_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const string_view_type reason = "initial a new scoped_stack_scope",
							const std_source_location& location = std_source_location::current()))
				: scoped_base{state}
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), create a scoped_stack_scope because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)
			}

		private:
			void do_construct() const;
			void do_destruct() const;
		};

		struct scoped_function_scope : utils::scoped_base<scoped_function_scope, std::reference_wrapper<const dispatcher_state>>
		{
			friend struct scoped_base<scoped_function_scope, std::reference_wrapper<const dispatcher_state>>;

			explicit scoped_function_scope(
					const dispatcher_state& state
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const string_view_type reason = "initial a new scoped_function_scope",
							const std_source_location& location = std_source_location::current()))
				: scoped_base{state}
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("{} from (file: '{}' function: '{}' position: ({}:{})), create a scoped_function_scope because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							reason);)
			}

		private:
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
			// todo: the lifetime of our cached location may be longer than the actual object (such as when doing a for loop, the loop variable will be re-added to the scope each time the loop is looped (even each loop is a new scope))
			// using object_cache_location_type = std::optional<std::reference_wrapper<boxed_value>>;
			using object_cache_location_type = std::optional<boxed_value>;
			// todo: the lifetime of our cached location may be longer than the actual object (such as returning an empty smart pointer and then automatically destroying it after use)
			using function_cache_location_type = std::optional<std::shared_ptr<function_proxies_type>>;

			using type_infos_type = engine_module::type_infos_type;

			struct function_pack
			{
				std::shared_ptr<dispatch_function::functions_type> overloaded;
				function_proxy_type dispatched;
				boxed_value boxed;
			};

			using functions_type = std::map<string_view_type, function_pack, std::less<>>;

			using objects_type = engine_module::objects_type;

			struct state_type
			{
				type_infos_type types;
				functions_type functions;
				objects_type global_objects;
			};

		private:
			std::reference_wrapper<string_pool_type> borrowed_pool_;

			state_type state_;
			convertor_manager convertor_manager_;
			std::reference_wrapper<ast::ast_parser_base> parser_;

			utils::thread_storage<engine_stack> stack_;
			mutable utils::threading::shared_mutex mutex_;
			mutable function_cache_location_type method_missing_location_;

			struct function_comparator
			{
				bool operator()(const function_proxy_type& lhs, const function_proxy_type& rhs) const
				{
					const auto real_lhs = std::dynamic_pointer_cast<const dynamic_function_proxy_base>(lhs);
					const auto real_rhs = std::dynamic_pointer_cast<const dynamic_function_proxy_base>(rhs);

					if (real_lhs && real_rhs)
					{
						if (real_lhs->get_guard()) { return real_rhs->get_guard() ? false : true; }
						return false;
					}

					if (real_lhs && not real_rhs) { return false; }
					if (not real_lhs && real_rhs) { return true; }

					const auto lhs_types = lhs->type_view();
					const auto rhs_types = rhs->type_view();

					for (decltype(lhs_types.size()) i = 1; i < lhs_types.size() && i < rhs_types.size(); ++i)
					{
						const auto& lhs_type = lhs_types[static_cast<std::ranges::range_difference_t<decltype(lhs_types)>>(i)];
						const auto& rhs_type = rhs_types[static_cast<std::ranges::range_difference_t<decltype(rhs_types)>>(i)];

						if (lhs_type.bare_equal(rhs_type) && lhs_type.is_const() == rhs_type.is_const())
						{
							// The first two types are essentially the same, next iteration
							continue;
						}

						if (lhs_type.bare_equal(rhs_type) && lhs_type.is_const() && not rhs_type.is_const())
						{
							// const is after non-const for the same type
							return false;
						}

						if (lhs_type.bare_equal(rhs_type) && not lhs_type.is_const()) { return true; }

						if (lhs_type.bare_equal(boxed_value::class_type()))
						{
							// boxed_values are sorted last
							return false;
						}

						if (rhs_type.bare_equal(boxed_value::class_type())) { return true; }

						if (lhs_type.bare_equal(types::number_type::class_type())) { return false; }

						if (rhs_type.bare_equal(types::number_type::class_type())) { return true; }

						// otherwise, we want to sort by typeid
						return lhs_type.before(rhs_type);
					}

					return false;
				}
			};

		public:
			explicit dispatcher(string_pool_type& pool, ast::ast_parser_base& p)
				: borrowed_pool_{pool},
				  parser_{p} { stack_.construct(pool); }

			void takeover_pool(string_pool_type&& pool) const { borrowed_pool_.get().takeover(std::move(pool)); }

			/**
			 * @brief casts an object while applying any dynamic_conversion available.
			 * @throw bad_boxed_cast(std::bad_cast)
			 */
			template<typename T>
			decltype(auto) boxed_cast(const boxed_value& object) const
			{
				const convertor_manager_state state{convertor_manager_};
				return gal::lang::boxed_cast<T>(object, &state);
			}

			/**
			 * @brief Registers a new named type.
			 */
			void add_type_info(
					const string_view_type name,
					const gal_type_info& type
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				utils::threading::unique_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::debug("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add type_info '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							state_.types.contains(name) ? "but it was already exist" : "add successed");)

				if (const auto it = state_.types.find(name);
					it != state_.types.end()) { throw exception::name_conflict_error{name}; }
				else { state_.types.emplace_hint(it, borrowed_pool_.get().append(name), type); }
			}

			/**
			 * @brief Add a new named proxy_function to the system.
			 *
			 * @throw exception::name_conflict_error if there's a function matching the given one being added.
			 */
			void add_function(
					const string_view_type name,
					function_proxy_type function
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				utils::threading::unique_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add a function '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							state_.functions.contains(name) ? "but it was already exist" : "add successed");)

				string_view_type pool_name = name;

				auto function_object = [&pool_name, this]<typename Fun>(Fun&& func) -> function_proxy_type
				{
					auto& functions = state_.functions;

					if (const auto it = functions.find(pool_name);
						it != functions.end())
					{
						// name already registered
						if (std::ranges::any_of(*it->second.overloaded, [&func](const auto& f) { return *func == *f; })) { throw exception::name_conflict_error{pool_name}; }

						auto copy_fs = *it->second.overloaded;
						// tightly control vec growth
						copy_fs.reserve(1 + copy_fs.size());
						copy_fs.emplace_back(std::forward<Fun>(func));
						std::ranges::stable_sort(copy_fs, function_comparator{});

						it->second.overloaded = std::make_shared<std::decay_t<decltype(copy_fs)>>(copy_fs);
						return std::make_shared<dispatch_function>(std::move(copy_fs));
					}
					else
					{
						// need register name
						pool_name = borrowed_pool_.get().append(pool_name);

						if (func->has_arithmetic_param())
						{
							// if the function is the only function, but it also contains
							// arithmetic operators, we must wrap it in a dispatch function
							// to allow for automatic arithmetic type conversions
							std::decay_t<decltype(*it->second.overloaded)> fs;
							fs.reserve(1);
							fs.emplace_back(std::forward<Fun>(func));
							functions.emplace(pool_name, std::make_shared<std::decay_t<decltype(fs)>>(fs));
							return std::make_shared<dispatch_function>(std::move(fs));
						}
						auto fs = std::make_shared<std::decay_t<decltype(*it->second.overloaded)>>();
						fs->emplace_back(func);
						functions.emplace(pool_name, fs);
						return func;
					}
				}(std::move(function));

				auto& [_, dispatched, boxed] = state_.functions[pool_name];
				boxed = const_var(function_object);
				dispatched = std::move(function_object);
			}

			/**
			 * @brief Adds a new global (const) shared object, between all the threads.
			 *
			 * @throw global_mutable_error object is not const
			 */
			boxed_value& add_global(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add an global object '{}', {}.",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							not object.is_const() ? "but it is not a const object" : "const test passed, forward to add_global_mutable");)

				if (not object.is_const()) { throw exception::global_mutable_error{name}; }

				return add_global_mutable(name, std::move(object) GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location));
			}

			/**
			 * @brief Add a new convertor for up-casting to a base class.
			 */
			void add_convertor(
					const convertor_type& conversion
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) { convertor_manager_.add_convertor(conversion GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location)); }

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			boxed_value& add_global_mutable(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				utils::threading::unique_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add an global {} object '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							object.is_const() ? "const" : "mutable",
							name,
							state_.global_objects.contains(name) ? "but it was already exist" : "add successed");)

				if (const auto it = state_.global_objects.find(name);
					it == state_.global_objects.end()) { return state_.global_objects.emplace_hint(it, borrowed_pool_.get().append(name), std::move(object))->second; }

				throw exception::name_conflict_error{name};
			}

			/**
			 * @brief Adds a new global (non-const) shared object, between all the threads.
			 */
			boxed_value& add_global_mutable_no_throw(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				utils::threading::unique_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add an global {} object '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							object.is_const() ? "const" : "mutable",
							name,
							state_.global_objects.contains(name) ? "but it was already exist" : "add successed");)

				if (const auto it = state_.global_objects.find(name);
					it != state_.global_objects.end()) { return it->second; }
				else { return state_.global_objects.emplace_hint(it, borrowed_pool_.get().append(name), std::move(object))->second; }
			}

			/**
			 * @brief Updates an existing global shared object or adds a new global shared object if not found.
			 */
			boxed_value& add_global_or_assign(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				utils::threading::unique_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to add an global {} object '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							object.is_const() ? "const" : "mutable",
							name,
							state_.global_objects.contains(name) ? "but it was already exist, assign it" : "add successed");)

				if (const auto it = state_.global_objects.find(name);
					it != state_.global_objects.end()) { return it->second = std::move(object); }
				else { return state_.global_objects.emplace_hint(it, borrowed_pool_.get().append(name), std::move(object))->second; }
			}

			/**
			 * @brief Set the value of an object, by name. If the object
			 * is not available in the current scope it is created.
			 */
			boxed_value& add_local_or_assign(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) { return stack_->add_object(name, std::move(object) GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location)); }

			/**
			 * @brief Add a object, if this variable already exists in the current scope, an exception will be thrown.
			 *
			 * @throw exception::name_conflict_error object already exist.
			 */
			boxed_value& add_local_or_throw(
					const string_view_type name,
					boxed_value object
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) { return stack_->add_object_no_check(name, std::move(object) GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, location)); }

			/**
			 * @brief Returns the type info for a named type.
			 * @throw std::range_error
			 */
			[[nodiscard]] gal_type_info get_type_info(
					const string_view_type name,
					const bool throw_if_not_exist = true
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				utils::threading::shared_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to get type_info '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							not state_.types.contains(name) ? "but it was not exist" : "found it");)

				if (const auto it = state_.types.find(name);
					it != state_.types.end()) { return it->second; }

				if (throw_if_not_exist) { throw std::range_error{"type_info not exist"}; }
				return {};
			}

			/**
			 * @brief Returns the registered name of a known type_info object compares the "bare_type_info" for the broadest possible match
			 */
			[[nodiscard]] string_view_type get_type_name(
					const gal_type_info& type
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())
					) const
			{
				utils::threading::shared_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to get type_name '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							type.bare_name(),
							std::ranges::find_if(state_.types | std::views::values, [&type](const auto& t)
								{ return t.bare_equal(type); }).base() == state_.types.end()
							? "but it was not exist"
							: "found it");)

				if (const auto it = std::ranges::find_if(state_.types | std::views::values, [&type](const auto& t) { return t.bare_equal(type); }).base();
					it != state_.types.end()) { return it->first; }

				return type.bare_name();
			}

			/**
			 * @brief Searches the current stack for an object of the given name
			 * includes a special overload for the _ place holder object to
			 * ensure that it is always in scope.
			 *
			 * @throw std::range_error object not found.
			 */
			[[nodiscard]] boxed_value& get_object(
					const string_view_type name,
					object_cache_location_type& cache_location
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to get object '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							cache_location.has_value() ? "it was already cached" : "try to find it");)

				if (cache_location.has_value()) { return *cache_location; }

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						int scope_no = 0;)

				// Is it in the stack?
				for (auto& stack = stack_->recent_stack();
				     auto& scope: stack | std::views::reverse)
				{
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							utils::logger::info("searching object '{}' in '{}'th scope",
								name,
								scope_no);)

					if (auto it = scope.find(name);
						it != scope.end())
					{
						GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
								utils::logger::info("found object '{}' in '{}'th scope",
									name,
									scope_no);)

						cache_location.emplace(it->second);
						return it->second;
					}

					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							++scope_no);
				}

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("can not find local variable '{}', try to find it in global scope or function scope", name);)

				// Is the value we are looking for a global?
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = state_.global_objects.find(name);
					it != state_.global_objects.end())
				{
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							utils::logger::info("find variable '{}' in global scope", name);
							)

					cache_location.emplace(it->second);
					return it->second;
				}

				// no? is it a function object?
				return get_function_object(name, cache_location);
			}

		private:
			/**
			 * @return a function object (boxed_value wrapper) if it exists.
			 * @throw std::range_error if it does not.
			 * @warning does not obtain a mutex lock.
			 */
			[[nodiscard]] boxed_value& get_function_object(const string_view_type name, object_cache_location_type& cache_location)
			{
				auto& functions = state_.functions;

				if (const auto it = functions.find(name);
					it != functions.end())
				{
					cache_location.emplace(it->second.boxed);
					return it->second.boxed;
				}
				throw std::range_error{"object not found"};
			}

		public:
			/**
			 * @brief Return true if a function exists.
			 */
			[[nodiscard]] bool has_function(const string_view_type name) const
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
			[[nodiscard]] auto get_function(
					const string_view_type name
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				utils::threading::shared_lock lock{mutex_};

				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to get function '{}', {}",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							not state_.functions.contains(name) ? "but it was not exist" : "found it");)

				const auto& functions = state_.functions;
				if (const auto it = functions.find(name); it != functions.end()) { return it->second.overloaded; }
				else { return std::make_shared<std::decay_t<decltype(it->second.overloaded)>::element_type>(); }
			}

			[[nodiscard]] auto get_method_missing_functions() const
			{
				if (method_missing_location_.has_value()) { return *method_missing_location_; }

				auto result = get_function(dynamic_object::missing_method_name);
				method_missing_location_.emplace(result);
				return result;
			}

			/**
			 * @brief Returns true if a call can be made that consists of the first
			 * parameter (the function) with the remaining parameters as its arguments.
			 */
			[[nodiscard]] boxed_value invokable(const parameters_view_type params) const
			{
				if (params.empty()) { throw exception::arity_error{1, static_cast<exception::arity_error::size_type>(params.size())}; }

				const auto& fun = this->boxed_cast<const_function_proxy_type>(params.front());
				const convertor_manager_state state{convertor_manager_};
				// skip the first one
				return const_var(fun->match(params.sub_list(1), state));
			}

			/**
			 * @brief return true if the object matches the registered type by name.
			 */
			[[nodiscard]] bool is_typeof(const string_view_type name, const boxed_value& object) const noexcept
			{
				try { if (get_type_info(name).bare_equal(object.type_info())) { return true; } }
				catch (const std::range_error&) { }

				try
				{
					const auto& o = boxed_cast<const dynamic_object&>(object);
					return o.nameof() == name;
				}
				catch (const std::bad_cast&) { }

				return false;
			}

			/**
			 * @brief Returns the registered name of a known type_info object
			 * compares the "bare_type_info" for the broadest possible match.
			 */
			[[nodiscard]] string_view_type nameof(const gal_type_info& type) const
			{
				utils::threading::shared_lock lock{mutex_};

				if (const auto it = std::ranges::find_if(
							state_.types,
							[&type](const auto& t) { return t.bare_equal(type); },
							[](const auto& pair) { return pair.second; });
					it != state_.types.end()) { return it->first; }

				return type.bare_name();
			}

			/**
			 * @brief Returns the registered name of a known type_info object
			 * compares the "bare_type_info" for the broadest possible match.
			 */
			[[nodiscard]] string_view_type nameof(const boxed_value& object) const { return nameof(object.type_info()); }

			void emit_call(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "emit a call",
							const std_source_location& location = std_source_location::current())) { stack_->emit_call(convertor_manager_state{convertor_manager_} GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, reason, location)); }

			void finish_call(
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							const std::string_view reason = "finish a call",
							const std_source_location& location = std_source_location::current())) noexcept { stack_->finish_call(convertor_manager_state{convertor_manager_} GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, reason, location)); }

			[[nodiscard]] bool is_member_function_call(
					const function_proxies_view_type functions,
					const parameters_view_type params,
					const bool has_param) const noexcept
			{
				if (not has_param || params.empty()) { return false; }

				return std::ranges::any_of(
						functions,
						[&params, cms = convertor_manager_state{convertor_manager_}](const auto& function) { return function->is_member_function() && function->is_first_type_match(params.front(), cms); });
			}

			boxed_value call_member_function(
					const string_view_type name,
					function_cache_location_type& cache_location,
					const parameters_view_type params,
					const bool has_params
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to call member function '{}' with '{}' params",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							params.size());)

				gal_assert(not cache_location.has_value());
				const auto& functions = get_function(name);
				cache_location.emplace(functions);

				const convertor_manager_state cms{convertor_manager_};

				const auto do_member_function_call = [this, cms](
						const function_proxy_base::arity_size_type num_params,
						const parameters_view_type ps,
						const auto& fs) -> boxed_value
				{
					const auto member_params = ps.front_sub_list(num_params);
					auto object = dispatch(fs, member_params, cms);
					if (num_params < static_cast<function_proxy_base::arity_size_type>(ps.size()) || object.type_info().bare_equal(function_proxy_base::class_type()))
					{
						const dispatcher_state state{*this};
						scoped_object_scope object_scope{state, ps.front()};

						try
						{
							const auto function = boxed_cast<const function_proxy_base*>(object);
							try { return (*function)(ps.sub_list(num_params), cms); }
							catch (const exception::bad_boxed_cast&) { }
							catch (const exception::arity_error&) { }
							catch (const exception::guard_error&) { }
							throw exception::dispatch_error{
									ps.sub_list(num_params).to<parameters_type>(),
									{boxed_cast<const_function_proxy_type>(object)}};
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

				if (is_member_function_call(*functions, params, has_params)) { return do_member_function_call(1, params, *functions); }

				std::exception_ptr current_exception;

				if (not functions->empty())
				{
					try { return dispatch(*functions, params, cms); }
					catch (exception::dispatch_error&) { current_exception = std::current_exception(); }
				}

				// If we get here we know that either there was no method with that name,
				// or there was no matching method

				const auto missing_functions = [this, params, &cms]
				{
					decltype(get_method_missing_functions())::element_type ret{};

					const auto mmf = get_method_missing_functions();

					std::ranges::for_each(
							*mmf,
							[&ret, params, &cms](const auto& f) { if (f->is_first_type_match(params.front(), cms)) { ret.push_back(f); } });

					return ret;
				}();

				const bool is_no_param = std::ranges::all_of(
						missing_functions,
						[](const auto& f) { return f->arity_size() == 2; });

				if (not missing_functions.empty())
				{
					try
					{
						if (is_no_param)
						{
							auto tmp_params = params.to<parameters_type>();
							tmp_params.insert(tmp_params.begin() + 1, var(name));
							return do_member_function_call(2, tmp_params, missing_functions);
						}
						const std::array tmp_params{params.front(), var(name), var(parameters_type{params.begin() + 1, params.end()})};
						return dispatch(missing_functions, tmp_params, cms);
					}
					catch (const std::exception& e)
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

			boxed_value call_function(
					const string_view_type name,
					function_cache_location_type& cache_location,
					const parameters_view_type params
					GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const std_source_location& location = std_source_location::current())) const
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to call function '{}' with '{}' params",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							name,
							params.size());)

				const convertor_manager_state state{convertor_manager_};

				if (cache_location.has_value()) { return dispatch(**cache_location, params, state); }

				auto functions = get_function(name);
				cache_location.emplace(functions);

				return dispatch(*functions, params, state);
			}

			[[nodiscard]] const convertor_manager& get_conversion_manager() const noexcept { return convertor_manager_; }

			[[nodiscard]] ast::ast_parser_base& get_parser() const noexcept { return parser_.get(); }
		};

		inline dispatcher_state::dispatcher_state(dispatcher& d)
			: d_{d},
			  state_{d.get_conversion_manager()} { }

		inline engine_stack& dispatcher_state::stack(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(const std_source_location& location)) const noexcept
		{
			GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
					utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), gained control of stack.",
						__func__,
						location.file_name(),
						location.function_name(),
						location.line(),
						location.column());)

			return *d_.get().stack_;
		}

		inline void scoped_scope::do_construct() const { data().get().stack().new_scope(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO("scoped_scope do_construct")); }

		inline void scoped_scope::do_destruct() const { data().get().stack().pop_scope(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO("scoped_scope do_destruct")); }

		inline scoped_object_scope::scoped_object_scope(
				const dispatcher_state& s,
				boxed_value object
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						,
						string_view_type reason,
						const std_source_location& location
						))
			: scoped_scope{s GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, reason, location)} { s.stack().add_object_no_check(object_self_type_name::value, std::move(object)); }

		inline void scoped_stack_scope::do_construct() const { data().get().stack().new_stack(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO("scoped_stack_scope do_construct")); }

		inline void scoped_stack_scope::do_destruct() const { data().get().stack().pop_stack(GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO("scoped_stack_scope do_destruct")); }

		inline void scoped_function_scope::do_construct() const { data().get().stack().emit_call(data().get().convertor_state() GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, "scoped_function_scope do_construct -> emit_call")); }

		inline void scoped_function_scope::do_destruct() const { data().get().stack().finish_call(data().get().convertor_state() GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(, "scoped_function_scope do_destruct -> finish_call")); }
	}
}

#endif // GAL_LANG_FOUNDATION_DISPATCHER_HPP
