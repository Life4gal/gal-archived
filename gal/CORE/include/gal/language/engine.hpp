#pragma once

#ifndef GAL_LANG_LANGUAGE_ENGINE_HPP
#define GAL_LANG_LANGUAGE_ENGINE_HPP

// todo
#define GAL_LANG_WINDOWS
#include <fstream>
#include <iostream>
#include <gal/language/binary_module_windows.hpp>

#include <gal/language/common.hpp>
#include <gal/exception_handler.hpp>
#include <utility>
#include <utils/thread_storage.hpp>
#include <utils/string_pool.hpp>
#include <gal/defines.hpp>

namespace gal::lang::lang
{
	namespace engine_detail
	{
		using shared_binary_module = std::shared_ptr<binary_module_detail::binary_module>;
	}

	/**
	 * @brief Alias to provide cleaner and more explicit syntax to users.
	 */
	using namespace_type = foundation::dynamic_object;
	using namespace_maker_type = std::function<namespace_type&()>;

	/**
	 * @brief The main object that user will use.
	 */
	class engine_base
	{
	public:
		enum class engine_option
		{
			dynamic_load_modules = 1 << 0,
			no_dynamic_load_modules = 1 << 1,
			external_scripts = 1 << 2,
			no_external_scripts = 1 << 3,

			default_option = dynamic_load_modules | external_scripts
		};

		// the filename is also stored in the corresponding pool
		using file_contents_type = std::map<foundation::string_view_type, utils::string_pool<foundation::string_view_type::value_type>>;
		using used_files_type = std::set<file_contents_type::key_type>;
		using loaded_modules_type = std::map<file_contents_type::key_type, engine_detail::shared_binary_module>;
		using active_loaded_modules = std::set<loaded_modules_type::key_type>;

	private:
		mutable utils::threading::shared_mutex mutex_;
		mutable utils::threading::recursive_mutex use_mutex_;

		file_contents_type file_contents_;

		used_files_type used_files_;
		loaded_modules_type loaded_modules_;
		active_loaded_modules active_loaded_modules_;

		std::vector<std::string> module_paths_;
		std::vector<std::string> use_paths_;

		std::unique_ptr<parser_detail::parser_base> parser_;

		foundation::dispatcher_detail::dispatcher dispatcher_;

		utils::string_pool<foundation::string_view_type::value_type> namespace_pool_;
		std::map<foundation::string_view_type, namespace_maker_type> namespace_generators_;

		[[nodiscard]] foundation::string_view_type load_file(const std::string_view filename)
		{
			auto& pool = file_contents_[filename];
			pool.append(filename);

			std::ifstream file{filename.data(), std::ios::in | std::ios::ate | std::ios::binary};

			if (not file.is_open()) { throw exception::file_not_found_error{filename}; }

			auto size = file.tellg();
			gal_assert(size >= 0);

			file.seekg(0, std::ios::beg);
			[&file, &size]
			{
				char buff[3]{};
				file.read(buff, 3);

				if (buff[0] == '\xef' && buff[1] == '\xbb' && buff[2] == '\xbf')
				{
					file.seekg(3);
					// decrement the BOM size from file size, otherwise we'll get parsing errors
					size -= 3;
					gal_assert(size >= 0);
				}
				else { file.seekg(0); }
			}();

			if (size == 0) { return {}; }

			auto dest = pool.take(size);
			file.read(dest, size);
			return {dest, static_cast<foundation::string_view_type::size_type>(size)};
		}

		/**
		 * @brief Evaluates the given string in by parsing it and running the results through the evaluator
		 */
		[[nodiscard]] foundation::boxed_value do_internal_eval(
				const foundation::string_view_type input,
				const foundation::string_view_type filename)
		{
			try { return parser_->parse(input, filename)->eval(foundation::dispatcher_detail::dispatcher_state{dispatcher_}, parser_->get_visitor()); }
			catch (interrupt_type::return_value& ret) { return std::move(ret.value); }
		}

		/**
		 * @brief Evaluates the given file and looks in the 'use' paths
		 */
		[[nodiscard]] foundation::boxed_value internal_eval_file(const std::string_view filename)
		{
			for (const auto& path: use_paths_)
			{
				try
				{
					const auto real_path = std::string{path}.append(filename);
					return do_internal_eval(load_file(filename), filename);
				}
				catch (const exception::file_not_found_error&)
				{
					// failed to load, try the next path
				}
				catch (const exception::eval_error& e) { throw var(e); }
			}

			throw exception::file_not_found_error{filename};
		}

		/**
		 * @brief Evaluates the given string, used during eval() inside of a script
		 */
		[[nodiscard]] foundation::boxed_value internal_eval(const foundation::string_view_type input)
		{
			try { return do_internal_eval(input, inline_eval_filename_name::value); }
			catch (const exception::eval_error& e) { throw var(e); }
		}

		/**
		 * @brief Builds all the requirements, including its evaluator and a run of its prelude.
		 */
		void build_system(engine_detail::shared_binary_module library, const engine_option option)
		{
			(void)this;
			(void)library;
			(void)option;
			// todo
		}

	public:
		/**
		 * @brief Represents the current state of the system. State and be saved and restored.
		 *
		 * @note State object does not contain the user defined type conversions of the engine.
		 * They are left out due to performance considerations involved in tracking the state.
		 */
		struct engine_state
		{
			used_files_type used_files;
			foundation::dispatcher_detail::dispatcher::state_type state;
			active_loaded_modules active_modules;
		};

		/**
		 * @param library Standard library to apply to this instance.
		 * @param parser Parser
		 * @param module_paths Vector of paths to search when attempting to load a binary module
		 * @param use_paths Vector of paths to search when attempting to "use" an included file
		 * @param option Option for build system
		 */
		engine_base(
				engine_detail::shared_binary_module library,
				std::unique_ptr<parser_detail::parser_base>&& parser,
				std::vector<std::string> module_paths = {},
				std::vector<std::string> use_paths = {},
				const engine_option option = engine_option::default_option)
			: module_paths_{std::move(module_paths)},
			  use_paths_{std::move(use_paths)},
			  parser_{std::move(parser)},
			  dispatcher_{*parser} { build_system(std::move(library), option); }

		[[nodiscard]] foundation::boxed_value eval(const ast_node& node)
		{
			try { return node.eval(foundation::dispatcher_detail::dispatcher_state{dispatcher_}, parser_->get_visitor()); }
			catch (const exception::eval_error& e) { throw var(e); }
		}

		[[nodiscard]] ast_node_ptr parse(const foundation::string_view_type input, const bool debug_print = false) const
		{
			auto result = parser_->parse(input, "engine_base::parse");
			if (debug_print)
			{
				// todo: output to where?
				std::cerr << parser_->debug_print(*result, "");
			}
			return result;
		}

		/**
		 * @brief Loads and parses a file. If the file is already open,
		 * it will not reloaded. The use paths specified at construction
		 * time are searched for the requested file.
		 */
		[[nodiscard]] foundation::boxed_value use(const foundation::string_view_type filename)
		{
			for (const auto& path: use_paths_)
			{
				const auto p = std::string{path}.append(filename);
				try
				{
					utils::threading::unique_lock lock{mutex_};
					utils::threading::unique_lock use_lock{use_mutex_};

					foundation::boxed_value ret{};

					if (not used_files_.contains(filename))
					{
						lock.unlock();
						ret = eval_file(p);
						lock.lock();
						// p is added to the pool after eval_file, we can safely use p (as string_view)
						used_files_.insert(p);
					}

					// return, we loaded it, or it was already loaded
					return ret;
				}
				catch (const exception::file_not_found_error& e)
				{
					if (e.filename != p)
					{
						// a nested file include failed
						throw;
					}
					// failed to load, try the next path
				}
			}

			// failed to load by any name
			throw exception::file_not_found_error{filename};
		}

		/**
		 * @brief Returns a state object that represents the current state of the global system.
		 *
		 * @note The global system includes the reserved words, global const objects, functions and types.
		 * Local variables are thread specific and not included.
		 *
		 * todo: copy?
		 */
		[[nodiscard]] engine_state get_engine_state() const
		{
			utils::threading::shared_lock lock{mutex_};
			utils::threading::scoped_lock use_lock{use_mutex_};

			return {
					.used_files = used_files_,
					.state = dispatcher_.copy_state(),
					.active_modules = active_loaded_modules_
			};
		}

		/**
		 * @brief Sets the state of the system
		 *
		 * The global system includes the reserved words, global objects, functions and types.
		 * Local variables are thread specific and not included.
		 *
		 * todo: copy?
		 */
		void set_engine_state(engine_state&& state)
		{
			utils::threading::shared_lock lock{mutex_};
			utils::threading::scoped_lock use_lock{use_mutex_};

			used_files_ = std::move(state.used_files);
			dispatcher_.set_state(state.state);
			active_loaded_modules_ = std::move(state.active_modules);
		}

		/**
		 * @brief All values in the local thread state, added through the add() function
		 */
		template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<foundation::dispatcher_detail::engine_stack::scope_type::value_type, AnyOther...>,
			                                 foundation::dispatcher_detail::engine_stack::scope_type::const_iterator,
			                                 foundation::dispatcher_detail::engine_stack::scope_type::const_iterator>
		[[nodiscard]] auto get_locals() { return foundation::dispatcher_detail::dispatcher_state{dispatcher_}.stack().copy_recent_locals<Container, AnyOther...>(); }


		/**
		 * @brief All values in the local thread state, added through the add() function
		 */
		template<template<typename...> typename Container, typename... AnyOther>
			requires std::is_constructible_v<Container<foundation::dispatcher_detail::engine_stack::scope_type::key_type, foundation::dispatcher_detail::engine_stack::scope_type::mapped_type, AnyOther...>,
			                                 foundation::dispatcher_detail::engine_stack::scope_type::const_iterator,
			                                 foundation::dispatcher_detail::engine_stack::scope_type::const_iterator>
		[[nodiscard]] auto get_locals() { return foundation::dispatcher_detail::dispatcher_state{dispatcher_}.stack().copy_recent_locals<Container, AnyOther...>(); }

		/**
		 * @brief Sets all of the locals for the current thread state.
		 */
		void set_locals(const foundation::dispatcher_detail::engine_stack::scope_type& new_locals) { foundation::dispatcher_detail::dispatcher_state{dispatcher_}.stack().set_locals(new_locals); }

		/**
		 * @brief Sets all of the locals for the current thread state.
		 */
		void set_locals(foundation::dispatcher_detail::engine_stack::scope_type&& new_locals) { foundation::dispatcher_detail::dispatcher_state{dispatcher_}.stack().set_locals(std::move(new_locals)); }

		template<typename T>
		decltype(auto) boxed_cast(const foundation::boxed_value& object) const { return dispatcher_.boxed_cast<T>(object); }

		/**
		 * @brief Registers a new named type.
		 */
		engine_base& add_type_info(const foundation::dispatcher_detail::dispatcher::name_view_type name, const foundation::gal_type_info& type)
		{
			dispatcher_.add_type_info(name, type);
			return *this;
		}

		/**
		 * @brief Add a new named proxy_function to the system.
		 */
		engine_base& add_function(const foundation::dispatcher_detail::dispatcher::name_view_type name, foundation::dispatcher_detail::dispatcher::state_type::function_type&& function)
		{
			dispatcher_.add_function(name, std::move(function));
			return *this;
		}

		/**
		 * @brief Adds a constant object that is available in all contexts and to all threads
		 *
		 * @param name Name of the value to add
		 * @param variable boxed_value to add as a global
		 *
		 * @throw global_mutable_error variable is not const
		 */
		engine_base& add_global(const foundation::dispatcher_detail::dispatcher::name_view_type name, foundation::boxed_value&& variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_global(name, std::move(variable));
			return *this;
		}

		/**
		 * @brief Add a new conversion for up-casting to a base class.
		 */
		engine_base& add_type_conversion(const foundation::type_conversion_manager::conversion_type& conversion)
		{
			dispatcher_.add_type_conversion(conversion);
			return *this;
		}

		/**
		 * @brief Adds all elements of a module to runtime.
		 */
		engine_base& add_module_ptr(const foundation::shared_engine_core& m)
		{
			m->apply(*this, dispatcher_);
			return *this;
		}

		/**
		 * @brief Adds a mutable object that is available in all contexts and to all threads
		 *
		 * @param name Name of the value to add
		 * @param variable boxed_value to add as a global
		 */
		engine_base& add_global_mutable(const foundation::dispatcher_detail::dispatcher::name_view_type name, foundation::boxed_value&& variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_global_mutable(name, std::move(variable));
			return *this;
		}

		engine_base& global_assign_or_insert(const foundation::dispatcher_detail::dispatcher::name_view_type name, foundation::boxed_value&& variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.global_assign_or_insert(name, std::move(variable));
			return *this;
		}

		/**
		 * @brief Objects are added to the local thread state.
		 */
		engine_base& local_assign_or_insert(const foundation::dispatcher_detail::dispatcher::name_view_type name, foundation::dispatcher_detail::dispatcher::variable_type&& variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.local_assign_or_insert(name, std::move(variable));
			return *this;
		}

		[[nodiscard]] foundation::dispatcher_detail::dispatcher::name_view_type get_type_name(const foundation::gal_type_info& type) const { return dispatcher_.get_type_name(type); }

		template<typename T>
		[[nodiscard]] foundation::dispatcher_detail::dispatcher::name_view_type get_type_name() const { return this->get_type_name(foundation::make_type_info<T>()); }

		/**
		 * @brief Load a binary module from a dynamic library.
		 * Works on platforms that support dynamic libraries.
		 *
		 * The module is searched for in the registered module path folders and with standard prefixes and postfixes: ("lib"|"")\<module_name\>(".dll"|".so"|".bundle"|"").
		 *
		 * Once the file is located, the system looks for the symbol binary_module::module_load_function_prefix\<module_name\>".
		 * If no file can be found matching the search criteria and containing the appropriate entry point (the symbol mentioned above), an exception is thrown.
		 *
		 */
		void load_module(const foundation::string_view_type module_name)
		{
			// todo
			(void)this;
			(void)module_name;
		}

		/**
		 * @brief Load a binary module from a dynamic library.
		 * Works on platforms that support dynamic libraries.
		 */
		void load_module(const foundation::string_view_type module_name, const foundation::string_view_type filename)
		{
			utils::threading::scoped_lock lock{use_mutex_};

			if (not loaded_modules_.contains(module_name))
			{
				auto m = std::make_shared<binary_module_detail::binary_module>(module_name, filename);
				loaded_modules_.emplace(module_name, m);
				active_loaded_modules_.emplace(module_name);
				add_module_ptr(m->module_ptr);
			}
			else if (not active_loaded_modules_.contains(module_name))
			{
				active_loaded_modules_.emplace(module_name);
				add_module_ptr(loaded_modules_[module_name]->module_ptr);
			}
		}

		/**
		 * @brief Evaluates a string.
		 *
		 * @throw exception::eval_error In the case that evaluation fails.
		 */
		[[nodiscard]] foundation::boxed_value eval(const foundation::string_view_type input, const exception_handler_type& handler = {}, const foundation::string_view_type filename = inline_eval_filename_name::value)
		{
			try { return do_internal_eval(input, filename); }
			catch (foundation::boxed_value& v)
			{
				if (handler) { handler->handle(v, dispatcher_); }
				throw;
			}
		}

		/**
		 * @brief Loads the file specified by filename, evaluates it, and returns the result.
		 *
		 * @throw exception::eval_error In the case that evaluation fails.
		 */
		[[nodiscard]] foundation::boxed_value eval_file(const foundation::string_view_type filename, const exception_handler_type& handler = {}) { return eval(load_file(filename), handler, filename); }

		/**
		 * @brief Loads the file specified by filename, evaluates it, and returns the result.
		 *
		 * @tparam T Type to extract from the result value of the script execution
		 *
		 * @throw exception::eval_error In the case that evaluation fails.
		 * @throw exception::bad_boxed_cast In the case that evaluation succeeds
		 * but the result value cannot be converted to the requested type.
		 */
		template<typename T>
		decltype(auto) eval_file(const foundation::string_view_type filename, const exception_handler_type& handler = {}) { return dispatcher_.boxed_cast<T>(eval_file(filename, handler)); }

		/**
		 * @brief Imports a namespace object into the global scope of this instance.
		 *
		 * @throw std::runtime_error In the case that the namespace name was never registered.
		 */
		void import_namespace(const foundation::dispatcher_detail::dispatcher::name_view_type namespace_name)
		{
			utils::threading::unique_lock use_lock{use_mutex_};

			if (const auto so = dispatcher_.get_scripting_objects();
				so.contains(namespace_name)) { throw std::runtime_error{std_format::format("namespace '{}' was already defined", namespace_name)}; }

			if (namespace_generators_.contains(namespace_name)) { dispatcher_.add_global_mutable(namespace_name, var(std::ref(namespace_generators_[namespace_name]()))); }

			throw std::runtime_error{std_format::format("namespace '{}' was not registered", namespace_name)};
		}

		/**
		 * @brief Registers a namespace generator, which delays generation of the namespace until it is imported,
		 * saving memory if it is never used.
		 *
		 * @throw std::runtime_error In the case that the namespace name was already registered.
		 */
		void register_namespace(const foundation::dispatcher_detail::dispatcher::name_view_type namespace_name, const std::function<void(namespace_type&)>& generator)
		{
			utils::threading::unique_lock use_lock{use_mutex_};

			if (not namespace_generators_.contains(namespace_name))
			{
				namespace_generators_.emplace(
						namespace_pool_.append(namespace_name),
						[generator, ns = namespace_type{}]() mutable -> namespace_type&
						{
							generator(ns);
							return ns;
						});
			}
			else { throw std::runtime_error{std_format::format("namespace '{}' was already registered", namespace_name)}; }
		}
	};
}

#endif // GAL_LANG_LANGUAGE_ENGINE_HPP
