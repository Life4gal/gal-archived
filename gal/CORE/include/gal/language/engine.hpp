#pragma once

#ifndef GAL_LANG_LANGUAGE_ENGINE_HPP
#define GAL_LANG_LANGUAGE_ENGINE_HPP

// todo
#define GAL_LANG_WINDOWS

#include <fstream>
#include <gal/exception_handler.hpp>
#include <gal/language/common.hpp>
#include <gal/extra/binary_module_windows.hpp>
#include <gal/foundation/string_pool.hpp>

namespace gal::lang::lang
{
	using binary_module_type = std::shared_ptr<extra::binary_module>;
	using engine_module_type = foundation::engine_module_type;

	/**
	 * @brief The main object that user will use.
	 */
	class engine_base
	{
	public:
		// the filename is also stored in the corresponding pool
		using file_contents_type = std::map<foundation::string_view_type, foundation::string_pool_type>;

		using used_files_type = std::set<file_contents_type::key_type>;
		using loaded_modules_type = std::map<file_contents_type::key_type, binary_module_type>;
		using active_loaded_modules = std::set<loaded_modules_type::key_type>;

		using preloaded_paths_type = std::vector<foundation::string_type>;

	private:
		mutable utils::threading::shared_mutex mutex_;
		mutable utils::threading::recursive_mutex load_mutex_;

		file_contents_type file_contents_;

		used_files_type loaded_files_;
		loaded_modules_type loaded_modules_;
		active_loaded_modules active_loaded_modules_;

		preloaded_paths_type preloaded_paths_;

		std::unique_ptr<parser_detail::parser_base> parser_;

		foundation::dispatcher dispatcher_;

		foundation::string_pool_type string_pool_;

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

			auto dest = pool.borrow_raw(size);
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
			try { return parser_->parse(input, filename)->eval(foundation::dispatcher_state{dispatcher_}, parser_->get_visitor()); }
			catch (interrupt_type::return_value& ret) { return std::move(ret.value); }
		}

		/**
		 * @brief Evaluates the given file and looks in the 'use' paths
		 */
		[[nodiscard]] foundation::boxed_value internal_eval_file(const foundation::string_view_type filename)
		{
			for (const auto& path: preloaded_paths_)
			{
				try
				{
					const auto real_path = foundation::string_type{path}.append(filename);
					return do_internal_eval(load_file(filename), filename);
				}
				catch (const exception::file_not_found_error&)
				{
					// failed to load, try the next path
				}
				catch (const exception::eval_error& e)
				{
					// todo
					throw var(e);
				}
			}

			throw exception::file_not_found_error{filename};
		}

		/**
		 * @brief Evaluates the given string, used during eval() inside of a script
		 */
		[[nodiscard]] foundation::boxed_value internal_eval(const foundation::string_view_type input)
		{
			try { return do_internal_eval(input, keyword_inline_eval_filename_name::value); }
			catch (const exception::eval_error& e)
			{
				// todo
				throw var(e);
			}
		}

		/**
		 * @brief Builds all the requirements, including its evaluator and a run of its prelude.
		 */
		void build_system(foundation::engine_module_type&& library)
		{
			if (library) { take_module(std::move(*library)); }

			// todo: other things
		}

	public:
		/**
		 * @param library Standard library to apply to this instance.
		 * @param parser Parser
		 * @param preloaded_paths Vector of paths to search when attempting to "use" an included file
		 */
		engine_base(
				foundation::engine_module_type&& library,
				std::unique_ptr<parser_detail::parser_base> parser,
				preloaded_paths_type preloaded_paths)
			: preloaded_paths_{std::move(preloaded_paths)},
			  parser_{std::move(parser)},
			  dispatcher_{string_pool_, *parser} { build_system(std::move(library)); }

		[[nodiscard]] foundation::boxed_value eval(ast_node& node)
		{
			try { return node.eval(foundation::dispatcher_state{dispatcher_}, parser_->get_visitor()); }
			catch (const exception::eval_error& e) { throw var(e); }
		}

		/**
		 * @brief Evaluates a string.
		 *
		 * @throw exception::eval_error In the case that evaluation fails.
		 */
		[[nodiscard]] foundation::boxed_value eval(
				const foundation::string_view_type input,
				const exception_handler_type& handler = {},
				const foundation::string_view_type filename = keyword_inline_eval_filename_name::value)
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
		[[nodiscard]] foundation::boxed_value eval_file(
				const foundation::string_view_type filename,
				const exception_handler_type& handler = {}) { return eval(load_file(filename), handler, filename); }

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
		decltype(auto) eval_file(
				const foundation::string_view_type filename,
				const exception_handler_type& handler = {}) { return dispatcher_.boxed_cast<T>(eval_file(filename, handler)); }

		[[nodiscard]] ast_node_ptr parse(const foundation::string_view_type input, const bool debug_print = false) const
		{
			auto result = parser_->parse(input, "engine_base::parse");
			if (debug_print)
			{
				// todo: output to where?
				tools::logger::debug(parser_->debug_print(*result, ""));
			}
			return result;
		}

		/**
		 * @brief Loads and parses a file. If the file is already open,
		 * it will not reloaded. The use paths specified at construction
		 * time are searched for the requested file.
		 */
		[[nodiscard]] foundation::boxed_value load(const foundation::string_view_type filename)
		{
			for (const auto& path: preloaded_paths_)
			{
				const auto p = foundation::string_type{path}.append(filename);
				try
				{
					utils::threading::unique_lock lock{mutex_};
					utils::threading::unique_lock load_lock{load_mutex_};

					foundation::boxed_value ret{};

					if (not loaded_files_.contains(filename))
					{
						lock.unlock();
						ret = eval_file(p);
						lock.lock();
						// p is added to the pool after eval_file, we can safely use p (as string_view)
						loaded_files_.insert(p);
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

		[[nodiscard]] foundation::string_view_type register_global_string(const foundation::string_view_type string) { return string_pool_.append(string); }

		// todo: register local string

		template<typename T>
		decltype(auto) boxed_cast(const foundation::boxed_value& object) const { return dispatcher_.boxed_cast<T>(object); }

		/**
		 * @brief Registers a new named type.
		 */
		engine_base& add_type_info(const foundation::string_view_type name, const foundation::gal_type_info& type)
		{
			dispatcher_.add_type_info(name, type);
			return *this;
		}

		/**
		 * @brief Add a new named proxy_function to the system.
		 */
		engine_base& add_function(const foundation::string_view_type name, foundation::function_proxy_type function)
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
		engine_base& add_global(const foundation::string_view_type name, foundation::boxed_value variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_global(name, std::move(variable));
			return *this;
		}

		/**
		 * @brief Add a new convertor for up-casting to a base class.
		 */
		engine_base& add_convertor(const foundation::convertor_type& convertor)
		{
			dispatcher_.add_convertor(convertor);
			return *this;
		}

		/**
		 * @brief Load a module, copy all type_info, function, object, evaluation, converter, shared their name.
		 *
		 * @note All names of the modules' type_info, function, object, evaluation, converter will be invalid if module been destroyed.
		 */
		engine_base& borrow_module(const foundation::engine_module_type& m)
		{
			m->borrow(*this, dispatcher_);
			return *this;
		}

		/**
		 * @brief Load a module, move away all type_info, function, object, evaluation, converter, also take over the module string_pool, then the module can be safely destroyed.
		 *
		 * @note DO NOT USE THE MODULE AFTER IT BEEN TAKEN
		 */
		engine_base& take_module(foundation::engine_module&& m)
		{
			std::move(m).take(*this, dispatcher_);
			return *this;
		}

		// todo: register binary_module

		/**
		 * @brief Adds a mutable object that is available in all contexts and to all threads
		 *
		 * @param name Name of the value to add
		 * @param variable boxed_value to add as a global
		 */
		engine_base& add_global_mutable(const foundation::string_view_type name, foundation::boxed_value variable)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_global_mutable(name, std::move(variable));
			return *this;
		}

		engine_base& global_assign_or_insert(const foundation::string_view_type name, foundation::boxed_value object)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_global_or_assign(name, std::move(object));
			return *this;
		}

		/**
		 * @brief Objects are added to the local thread state.
		 */
		engine_base& add_local_or_assign(const foundation::string_view_type name, foundation::boxed_value object)
		{
			name_validator::validate_object_name(name);
			dispatcher_.add_local_or_assign(name, std::move(object));
			return *this;
		}

		[[nodiscard]] foundation::string_view_type nameof(const foundation::gal_type_info& type) const { return dispatcher_.nameof(type); }

		template<typename T>
		[[nodiscard]] foundation::string_view_type nameof() const { return this->nameof(foundation::make_type_info<T>()); }
	};
}

#endif // GAL_LANG_LANGUAGE_ENGINE_HPP
