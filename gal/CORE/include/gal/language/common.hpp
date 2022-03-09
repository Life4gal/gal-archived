#pragma once

#ifndef GAL_LANG_LANGUAGE_COMMON_HPP
#define GAL_LANG_LANGUAGE_COMMON_HPP

#include <utils/point.hpp>
#include <utils/hash.hpp>
#include <gal/kits/dispatch.hpp>
#include <utils/unordered_hash_container.hpp>

namespace gal::lang
{
	struct name_validator
	{
		using name_type = std::string_view;

		[[nodiscard]] static bool is_reserved_word(const name_type word) noexcept
		{
			constexpr auto hash = [](const name_type s) constexpr noexcept { return utils::hash_fnv1a<false>(s); };

			const static utils::unordered_hash_set<decltype(utils::hash_fnv1a<false>(name_type{}))> words{
					hash(keyword_define_name::value),
					hash(keyword_function_name::value),
					hash(keyword_variable_name::value),
					hash(keyword_true_name::value),
					hash(keyword_false_name::value),
					hash(keyword_class_name::value),
					hash(keyword_attribute_name::value),
					hash(keyword_global_name::value),
					hash(keyword_placeholder_name::value),
					hash(keyword_comma_name::value),
					hash(keyword_while_name::value),
					hash(keyword_for_name::value),
					hash(keyword_break_name::value),
					hash(keyword_if_name::value),
					hash(keyword_else_name::value),
					hash(keyword_logical_and_name::value),
					hash(keyword_logical_or_name::value),
					hash(keyword_return_name::value),
			};

			return words.count(hash(word));
		}

		[[nodiscard]] static bool is_valid_object_name(const name_type name) noexcept { return not name.contains("::") && not is_reserved_word(name); }

		static void validate_object_name(const name_type name)
		{
			if (is_reserved_word(name)) { throw reserved_word_error{reserved_word_error::word_type{name}}; }

			if (name.contains("::")) { throw illegal_name_error{illegal_name_error::name_type{name}}; }
		}
	};

	/**
	 * @brief Signature of module entry point that all binary loadable modules must implement.
	 */
	using module_creator_function_type = shared_engine_module(*)();

	/**
	 * @brief Types of AST nodes available to the parser and eval
	 */
	enum class ast_node_type
	{
		noop_t,

		id_t,
		constant_t,
		reference_t,
		compiled_t,
		unary_t,
		binary_t,
		fun_call_t,
		array_call_t,
		dot_access_t,
		arg_t,
		arg_list_t,
		equation_t,
		global_decl_t,
		var_decl_t,
		assign_decl_t,
		class_decl_t,
		attribute_decl_t,
		def_t,
		method_t,
		lambda_t,

		no_scope_block_t,
		block_t,

		if_t,
		while_t,
		for_t,
		ranged_for_t,
		break_t,
		continue_t,
		file_t,
		return_t,
		switch_t,
		case_t,
		default_t,

		logical_and_t,
		logical_or_t,

		inline_range_t,
		inline_array_t,
		inline_map_t,
		map_pair_t,
		value_range_t,

		try_t,
		catch_t,
		finally_t,

		ast_node_type_size
	};

	enum class operator_precedence
	{
		ternary_cond,
		logical_or,
		logical_and,
		bitwise_or,
		bitwise_xor,
		bitwise_and,
		equality,
		comparison,
		bitwise_shift,
		plus,
		multiply,
		unary,
	};

	namespace detail
	{
		using ast_node_type_string_type = std::string_view;

		/**
		 * @brief Helper lookup to get the name of each node type
		 */
		constexpr ast_node_type_string_type to_string(const ast_node_type type) noexcept
		{
			// todo: name
			constexpr ast_node_type_string_type node_type_names[]{
					{"noop"},

					{"id"},
					{"constant"},
					{"reference"},
					{"compiled"},
					{"unary_operation"},
					{"binary_operation"},
					{"fun_call"},
					{"array_call"},
					{"dot_access"},
					{"arg"},
					{"arg_list"},
					{"equation"},
					{"global_decl"},
					{"var_decl"},
					{"assign_decl"},
					{"class_decl"},
					{"attribute_decl"},
					{"def"},
					{"method"},
					{"lambda"},

					{"no_scope_block"},
					{"block"},

					{"if"},
					{"while"},
					{"for"},
					{"ranged_for"},
					{"break"},
					{"continue"},
					{"file"},
					{"return"},
					{"switch"},
					{"case"},
					{"default"},

					{"logical_and"},
					{"logical_or"},

					{"inline_range"},
					{"inline_array"},
					{"inline_map"},
					{"map_pair"},
					{"value_range"},

					{"try"},
					{"catch"},
					{"finally"},
			};

			static_assert(std::size(node_type_names) == static_cast<std::size_t>(ast_node_type::ast_node_type_size));

			return node_type_names[static_cast<std::size_t>(type)];
		}
	}

	/**
	 * @brief Convenience type for file positions.
	 */
	using file_position = utils::basic_point<int>;
	using file_location = utils::basic_location<int>;

	struct parse_location
	{
		using filename_type = std::string;
		using shared_filename_type = std::shared_ptr<filename_type>;

		file_location location;
		shared_filename_type filename;

		explicit parse_location(
				shared_filename_type filename,
				file_location location = {})
			: location{location},
			  filename{std::move(filename)} {}

		explicit parse_location(
				filename_type filename = "",
				const file_location location = {})
			: parse_location{std::make_shared<filename_type>(std::move(filename)), location} {}
	};

	class eval_error;

	namespace detail
	{
		template<typename T>
		struct ast_node_base
		{
			template<typename>
			friend struct ast_node_base;

			// todo: type is not necessary
			ast_node_type type;
			// do not modify text.
			std::string text;
			parse_location location;

			ast_node_base(const ast_node_type type,
			              std::string text,
			              parse_location location)
				: type{type},
				  text{std::move(text)},
				  location{std::move(location)} {}

			template<typename U>
				requires (not std::is_same_v<U, T>)
			explicit ast_node_base(const ast_node_base<U>& other)
				: ast_node_base{static_cast<const ast_node_base<T>&>(other)} {}

			[[nodiscard]] const parse_location::filename_type& filename() const noexcept { return *location.filename; }

			[[nodiscard]] file_position location_begin() const noexcept { return location.location.begin; }

			[[nodiscard]] file_position location_end() const noexcept { return location.location.end; }

			void pretty_format_position_to(std::string& target) const
			{
				std_format::format_to(std::back_inserter(target),
				                      "(line: {}, column: {} in file '{}')",
				                      location_begin().line,
				                      location_begin().column,
				                      filename());
			}

			[[nodiscard]] std::string pretty_position_print() const
			{
				std::string ret;
				pretty_format_position_to(ret);
				return ret;
			}

		private:
			template<typename U>
			explicit operator const ast_node_base<U>&() const { return reinterpret_cast<const ast_node_base<U>&>(*this); };

			template<typename Function>
			void format_children_to(std::string& target, Function&& function) const
			{
				const auto& children = static_cast<const T&>(*this).get_children();
				std::ranges::for_each(
						children,
						[&target, function](const auto& child)
						{
							function(child, target);
							target.push_back(' ');
						});
			}

		public:
			void pretty_format_to(std::string& target) const
			{
				target.append(text);
				format_children_to(target, [](const auto& child, std::string& t) { T::unwrap_child(child).pretty_format_to(t); });
			}

			[[nodiscard]] std::string pretty_print() const
			{
				std::string result;
				pretty_format_to(result);
				return result;
			}

			void to_string_to(std::string& target, const std::string_view prepend = "") const
			{
				std_format::format_to(
						std::back_inserter(target),
						"{}({}) {} : ",
						prepend,
						detail::to_string(type),
						text);
				pretty_format_position_to(target);
				target.push_back('\n');

				format_children_to(target, [prepend](const auto& child, std::string& t) { T::unwrap_child(child).to_string_to(t, prepend); });
			}

			/**
			 * @brief Prints the contents of an AST node, including its children, recursively
			 */
			[[nodiscard]] std::string to_string(const std::string_view prepend = "") const
			{
				std::string result;
				to_string_to(result, prepend);

				return result;
			}
		};
	}

	struct ast_node : detail::ast_node_base<ast_node>
	{
		friend struct ast_node_base<ast_node>;

	private:
		static ast_node& unwrap_child(const std::reference_wrapper<ast_node>& c) { return c.get(); }

	protected:
		using ast_node_base<ast_node>::ast_node_base;

	public:
		using children_type = std::vector<std::reference_wrapper<ast_node>>;

		virtual ~ast_node() noexcept = default;
		ast_node(const ast_node&) = default;
		ast_node& operator=(const ast_node&) = default;
		ast_node(ast_node&&) = default;
		ast_node& operator=(ast_node&&) = default;

		/**
		 * @throw eval_error
		 */
		static bool get_bool_condition(const kits::boxed_value& object, const detail::dispatch_state& state);

		[[nodiscard]] virtual children_type get_children() const = 0;
		[[nodiscard]] virtual kits::boxed_value eval(const detail::dispatch_state& state) const = 0;
	};

	using ast_node_const_ptr = std::unique_ptr<std::add_const_t<ast_node_ptr::element_type>>;

	struct ast_node_trace : detail::ast_node_base<ast_node_trace>
	{
		using children_type = std::vector<ast_node_trace>;

		children_type children;

		explicit ast_node_trace(const ast_node& node)
			: ast_node_base{node},
			  children{get_children(node)} {}

	private:
		friend struct ast_node_base<ast_node_trace>;

		static const ast_node_trace& unwrap_child(const ast_node_trace& c) { return c; }

		// for base's pretty_print
		[[nodiscard]] children_type get_children() const { return children; }

	public:
		[[nodiscard]] static children_type get_children(const ast_node& node)
		{
			const auto& c = node.get_children();
			return {c.begin(), c.end()};
		}
	};

	/**
	 * @brief Thrown if an error occurs while attempting to load a binary module.
	 */
	class load_module_error final : public std::runtime_error
	{
	public:
		using errors_type = std::vector<load_module_error>;

		static std::string format_errors(const std::string_view name, const errors_type& errors)
		{
			auto ret = std_format::format("Error loading module '{}'\n\tThe following locations were searched: \n", name);

			std::ranges::for_each(
					errors,
					[&ret](const auto& error) { std::format_to(std::back_inserter(ret), "\t\t\n{}", error.what()); });

			return ret;
		}

		using std::runtime_error::runtime_error;

		load_module_error(const std::string_view name, const errors_type& errors)
			: std::runtime_error{format_errors(name, errors)} {}
	};

	/**
	 * @brief Errors generated during parsing or evaluation.
	 */
	class eval_error final : public std::runtime_error
	{
	public:
		std::string reason;
		std::string filename;
		file_position begin_position;
		std::string detail;
		std::vector<ast_node_trace> stack_traces;

		eval_error(
				const std::string_view reason,
				const std::string_view filename,
				const file_position begin_position,
				const kits::function_parameters& params,
				const kits::proxy_function_base::contained_functions_type& functions,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine
				)
			: std::runtime_error{format(reason, filename, begin_position, params, has_dot_notation, engine)},
			  reason{reason},
			  filename{filename},
			  begin_position{begin_position},
			  detail{format_detail(functions, has_dot_notation, engine)} {}

		eval_error(
				const std::string_view reason,
				const kits::function_parameters& params,
				const kits::proxy_function_base::contained_functions_type& functions,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine)
			: std::runtime_error{format(reason, params, has_dot_notation, engine)},
			  reason{reason},
			  detail{format_detail(functions, has_dot_notation, engine)} {}

		eval_error(
				const std::string_view reason,
				const std::string_view filename,
				const file_position begin_position
				)
			: std::runtime_error{format(reason, filename, begin_position)},
			  reason{reason},
			  filename{filename},
			  begin_position{begin_position} {}

		explicit eval_error(
				const std::string_view reason
				)
			: std::runtime_error{get_formatted_reason(reason)},
			  reason{reason} {}

		[[nodiscard]] std::string pretty_print() const
		{
			std::string ret{what()};

			if (not stack_traces.empty())
			{
				ret.append(" during evaluation at ");
				stack_traces.front().pretty_format_position_to(ret);
				ret.push_back('\n');
				ret.append(detail).push_back('\n');
				stack_traces.front().pretty_format_to(ret);

				std::ranges::for_each(
						stack_traces.begin() + 1,
						stack_traces.end(),
						[&ret](const auto& trace)
						{
							if (trace.type != ast_node_type::block_t && trace.type != ast_node_type::file_t)
							{
								ret.push_back('\n');
								ret.append(" from ");
								trace.pretty_format_position_to(ret);
								trace.pretty_format_to(ret);
							}
						});
			}

			ret.push_back('\n');
			return ret;
		}

	private:
		static void format_reason(std::string& target, const std::string_view r) { std_format::format_to(std::back_inserter(target), "Error: '{}' ", r); }

		static std::string get_formatted_reason(const std::string_view r)
		{
			std::string ret;
			format_reason(ret, r);
			return ret;
		}

		static void format_parameters(std::string& target, const kits::function_parameters& params, const bool has_dot_notation, const detail::dispatch_engine& engine)
		{
			std_format::format_to(std::back_inserter(target),
			                      "With {} parameters: (",
			                      params.size());

			if (not params.empty())
			{
				for (auto it = params.begin(); it != params.end(); ++it)
				{
					std_format::format_to(std::back_inserter(target),
					                      "{} ({})",
					                      engine.get_type_name(*it),
					                      it->is_const() ? " (immutable)" : " (mutable)");

					if (it == params.begin() && has_dot_notation)
					{
						target.append(").(");
						if (params.size() == 1) { target.append(", "); }
					}
					else { target.append(", "); }
				}

				// ", "
				target.pop_back();
				target.pop_back();
			}

			target.append(") ");
		}

		static void format_filename(std::string& target, const std::string_view f)
		{
			// todo: default eval filename
			if (f != "__EVAL__") { std_format::format_to(std::back_inserter(target), "in '{}' ", f); }
			else { std_format::format_to(std::back_inserter(target), "during evaluation "); }
		}

		static void format_position(std::string& target, const file_position p) { std_format::format_to(std::back_inserter(target), "at ({}, {}) ", p.line, p.column); }

		static std::string format(
				const std::string_view r,
				const std::string_view f,
				const file_position p,
				const kits::function_parameters& params,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine
				)
		{
			std::string ret;

			format_reason(ret, r);
			format_parameters(ret, params, has_dot_notation, engine);
			format_filename(ret, f);
			format_position(ret, p);

			return ret;
		}

		static std::string format(
				const std::string_view r,
				const kits::function_parameters& params,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine
				)
		{
			std::string ret;

			format_reason(ret, r);
			format_parameters(ret, params, has_dot_notation, engine);

			return ret;
		}

		static std::string format(
				const std::string_view r,
				const std::string_view f,
				const file_position p
				)
		{
			std::string ret;

			format_reason(ret, r);
			format_filename(ret, f);
			format_position(ret, p);

			return ret;
		}

		static void format_types(
				std::string& target,
				const kits::proxy_function_base::contained_functions_type::value_type& function,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine
				)
		{
			gal_assert(function.operator bool());

			const auto arity = function->get_arity();
			const auto& types = function->types();

			if (arity == kits::proxy_function_base::no_parameters_arity)
			{
				std_format::format_to(
						std::back_inserter(target),
						"{}(...)",
						has_dot_notation ? "Object." : "");
			}
			else if (types.size() <= 1) { target.append("()"); }
			else
			{
				target.push_back('(');

				for (auto it = types.begin() + 1; it != types.end(); ++it)
				{
					std_format::format_to(std::back_inserter(target),
					                      "{} ({})",
					                      engine.get_type_name(*it),
					                      it->is_const() ? " (immutable)" : " (mutable)");

					if (it == types.begin() + 1 && has_dot_notation)
					{
						target.append(").(");
						if (types.size() == 2) { target.append(", "); }
					}
					else { target.append(", "); }
				}

				// ", "
				target.pop_back();
				target.pop_back();

				target.append(") ");
			}

			if (const auto fun = std::dynamic_pointer_cast<const kits::base::dynamic_proxy_function_base>(function);
				fun && fun->has_parse_tree())
			{
				if (const auto guard = fun->get_guard())
				{
					if (const auto guard_fun = std::dynamic_pointer_cast<const kits::base::dynamic_proxy_function_base>(guard);
						guard_fun && guard_fun->has_parse_tree())
					{
						target.append(" : ");
						guard_fun->get_parse_tree().pretty_format_to(target);
					}
				}

				target.append("\n\tDefined at: ");
				fun->get_parse_tree().pretty_format_position_to(target);
			}
		}

		static std::string format_detail(
				const kits::proxy_function_base::contained_functions_type& functions,
				const bool has_dot_notation,
				const detail::dispatch_engine& engine
				)
		{
			std::string ret;

			if (functions.size() == 1)
			{
				gal_assert(functions.front().operator bool());
				ret.append("\tExpected: ");
				format_types(ret, functions.front(), has_dot_notation, engine);
				ret.push_back('\n');
			}
			else
			{
				std_format::format_to(std::back_inserter(ret), "\t{} overload(s) available: \n", functions.size());

				std::ranges::for_each(
						functions,
						[&ret, has_dot_notation, &engine](const auto& function)
						{
							ret.push_back('\t');
							format_types(ret, function, has_dot_notation, engine);
							ret.push_back('\n');
						});
			}

			return ret;
		}
	};

	inline bool ast_node::get_bool_condition(const kits::boxed_value& object, const detail::dispatch_state& state)
	{
		try { return state->boxed_cast<bool>(object); }
		catch (const kits::bad_boxed_cast&) { throw eval_error{"Condition not boolean"}; }
	}

	namespace base
	{
		class parser_base
		{
		public:
			parser_base() = default;
			virtual ~parser_base() noexcept = default;

			parser_base(parser_base&&) = default;

			parser_base& operator=(const parser_base&) = delete;
			parser_base& operator=(parser_base&&) = delete;

		protected:
			parser_base(const parser_base&) = default;

		public:
			template<typename T>
			[[nodiscard]] T& get_tracer() noexcept
			{
				gal_assert(get_tracer_ptr());
				return *static_cast<T*>(get_tracer_ptr());
			}

			[[nodiscard]] virtual ast_node_ptr parse(std::string_view input, std::string_view filename) = 0;
			virtual void debug_print(const ast_node& node, std::string_view prepend = "") const = 0;

		private:
			[[nodiscard]] virtual void* get_tracer_ptr() = 0;
		};
	}

	namespace eval::detail
	{
		/**
		 * @brief Special type for returned values
		 */
		struct return_value
		{
			kits::boxed_value value;
		};

		/**
		 * @brief Special type indicating a call to 'break'
		 */
		struct break_loop {};

		/**
		 * @brief Special type indicating a call to 'continue'
		 */
		struct continue_loop {};
	}
}

#endif // GAL_LANG_LANGUAGE_COMMON_HPP
