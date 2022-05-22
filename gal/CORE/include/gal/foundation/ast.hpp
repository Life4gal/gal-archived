#pragma once

#ifndef GAL_LANG_LANGUAGE_AST_HPP
#define GAL_LANG_LANGUAGE_AST_HPP

#include <unordered_set>
#include <utils/hash.hpp>
#include <utils/point.hpp>
#include <gal/foundation/dispatcher.hpp>

namespace gal::lang
{
	namespace foundation
	{
		struct name_validator
		{
			[[nodiscard]] constexpr static auto hash_name(const string_view_type name) noexcept { return utils::hash_fnv1a<false>(name); }

			[[nodiscard]] static bool is_reserved_name(const string_view_type name) noexcept
			{
				const static std::unordered_set names{
						hash_name(keyword_define_name::value),
						hash_name(keyword_class_name::value),
						hash_name(keyword_variable_declare_name::value),
						hash_name(keyword_true_name::value),
						hash_name(keyword_false_name::value),
						hash_name(keyword_global_name::value),
						hash_name(keyword_and_name::value),
						hash_name(keyword_or_name::value),
						hash_name(keyword_if_name::value),
						hash_name(keyword_else_name::value),
						hash_name(keyword_for_in_name::subtype<0>::value),
						hash_name(keyword_for_in_name::subtype<1>::value),
						hash_name(keyword_while_name::value),
						hash_name(keyword_continue_break_return_name::subtype<0>::value),
						hash_name(keyword_continue_break_return_name::subtype<1>::value),
						hash_name(keyword_continue_break_return_name::subtype<2>::value),
						hash_name(keyword_match_case_default_name::subtype<1>::value),
						hash_name(keyword_match_case_default_name::subtype<2>::value),
						hash_name(keyword_match_fallthrough_name::value),
						hash_name(keyword_function_argument_placeholder_name::value),
						hash_name(keyword_try_catch_finally_name::subtype<0>::value),
						hash_name(keyword_try_catch_finally_name::subtype<1>::value),
						hash_name(keyword_try_catch_finally_name::subtype<2>::value),
						hash_name(keyword_function_guard_name::value),
						hash_name(keyword_operator_declare_name::value),
						hash_name(keyword_number_inf_nan_name::subtype<0>::value),
						hash_name(keyword_number_inf_nan_name::subtype<1>::value)};

				return names.contains(hash_name(name));
			}

			[[nodiscard]] static bool is_valid_object_name(const string_view_type name) noexcept { return not name.contains(keyword_class_accessor_name::value) && not is_reserved_name(name); }

			/**
			 * @throw exception::reserved_word_error 
			 * @throw exception::illegal_name_error
			 */
			static void validate_object_name(const foundation::string_view_type name)
			{
				if (is_reserved_name(name)) { throw exception::reserved_word_error{name}; }

				if (name.contains(keyword_class_accessor_name::value)) { throw exception::illegal_name_error{name}; }
			}
		};

		// see also addons/ast_parser.hpp => operator_matcher
		enum class operation_precedence
		{
			// or
			logical_or = 0,
			// and
			logical_and,
			// |
			bitwise_or,
			// ^
			bitwise_xor,
			// &
			bitwise_and,
			// == or !=
			equality,
			// < or <= or > or >=
			comparison,
			// << or >>
			bitwise_shift,
			// + or -
			plus_minus,
			// * or / or %
			multiply_divide,
			// ! or ~ or + or -
			unary,

			operation_size,
		};
	}// namespace lang

	namespace ast
	{
		/**
		 * @brief Convenience type for file positions.
		 */
		using file_point = utils::basic_point<int>;
		using file_location = utils::basic_location<int>;

		struct parse_location
		{
			using filename_type = foundation::string_view_type;

			file_location location;
			filename_type filename;

			explicit parse_location(
					filename_type filename = "",
					file_location location = {})
				: location{location},
				  filename{filename} {}
		};

		struct ast_node;
		struct ast_node_tracer;
	}

	namespace exception
	{
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
		 * @brie Errors generated when loading a file
		 */
		class file_not_found_error final : public std::runtime_error
		{
		public:
			std::string filename;

			explicit file_not_found_error(std::string filename)
				: std::runtime_error{std_format::format("File '{}' not found", filename)},
				  filename{std::move(filename)} {}

			explicit file_not_found_error(std::string_view filename)
				: std::runtime_error{std_format::format("File '{}' not found", filename)},
				  filename{filename} {}
		};

		/**
		 * @brief Errors generated during parsing or evaluation.
		 */
		class eval_error final : public std::runtime_error
		{
		public:
			using string_type = foundation::string_type;
			using string_view_type = foundation::string_view_type;

			string_type reason;
			string_type filename;
			ast::file_point begin_position;
			string_type detail;
			std::vector<ast::ast_node_tracer> stack_traces;

		private:
			static void format_reason(std::string& target, const string_view_type r) { std_format::format_to(std::back_inserter(target), "Error: '{}' ", r); }

			static std::string get_formatted_reason(const string_view_type r)
			{
				string_type ret;
				format_reason(ret, r);
				return ret;
			}

			static void format_parameters(
					string_type& target,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
			{
				std_format::format_to(std::back_inserter(target),
				                      "With {} parameters: (",
				                      params.size());

				if (not params.empty())
				{
					for (auto it = params.begin(); it != params.end(); ++it)
					{
						std_format::format_to(std::back_inserter(target),
						                      "'{}'({})",
						                      dispatcher.nameof(*it),
						                      it->is_const() ? "immutable" : "mutable");

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

			static void format_filename(string_type& target, const ast::parse_location::filename_type f)
			{
				if (f != foundation::keyword_inline_eval_filename_name::value) { std_format::format_to(std::back_inserter(target), "in '{}' ", f); }
				else { std_format::format_to(std::back_inserter(target), "during evaluation "); }
			}

			static void format_position(string_type& target, const ast::file_point p) { std_format::format_to(std::back_inserter(target), "at ({}, {}) ", p.line, p.column); }

			static string_type format(
					const string_view_type r,
					const ast::parse_location::filename_type f,
					const ast::file_point p,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
			{
				string_type ret;

				format_reason(ret, r);
				format_parameters(ret, params, has_dot_notation, dispatcher);
				format_filename(ret, f);
				format_position(ret, p);

				return ret;
			}

			static string_type format(
					const string_view_type r,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
			{
				string_type ret;

				format_reason(ret, r);
				format_parameters(ret, params, has_dot_notation, dispatcher);

				return ret;
			}

			static string_type format(
					const string_view_type r,
					const string_view_type f,
					const ast::file_point p)
			{
				string_type ret;

				format_reason(ret, r);
				format_filename(ret, f);
				format_position(ret, p);

				return ret;
			}

			static void format_types(
					string_type& target,
					const foundation::const_function_proxy_type& function,
					bool has_dot_notation,
					const foundation::dispatcher& dispatcher);

			static string_type format_detail(
					const foundation::const_function_proxies_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
			{
				string_type ret;

				if (functions.size() == 1)
				{
					gal_assert(functions.front().operator bool());
					ret.append("\tExpected: ");
					format_types(ret, functions.front(), has_dot_notation, dispatcher);
					ret.push_back('\n');
				}
				else
				{
					std_format::format_to(std::back_inserter(ret), "\t{} overload(s) available: \n", functions.size());

					std::ranges::for_each(
							functions,
							[&ret, has_dot_notation, &dispatcher](const auto& function)
							{
								ret.push_back('\t');
								eval_error::format_types(ret, function, has_dot_notation, dispatcher);
								ret.push_back('\n');
							});
				}

				return ret;
			}

		public:
			eval_error(
					const string_view_type reason,
					const string_view_type filename,
					const ast::file_point begin_position,
					const foundation::parameters_view_type params,
					const foundation::const_function_proxies_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
				: std::runtime_error{format(reason, filename, begin_position, params, has_dot_notation, dispatcher)},
				  reason{reason},
				  filename{filename},
				  begin_position{begin_position},
				  detail{format_detail(functions, has_dot_notation, dispatcher)} {}

			eval_error(
					const string_view_type reason,
					const foundation::parameters_view_type params,
					const foundation::const_function_proxies_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher& dispatcher)
				: std::runtime_error{format(reason, params, has_dot_notation, dispatcher)},
				  reason{reason},
				  detail{format_detail(functions, has_dot_notation, dispatcher)} {}

			eval_error(
					const string_view_type reason,
					const string_view_type filename,
					const ast::file_point begin_position)
				: std::runtime_error{format(reason, filename, begin_position)},
				  reason{reason},
				  filename{filename},
				  begin_position{begin_position} {}

			explicit eval_error(
					const string_view_type reason)
				: std::runtime_error{get_formatted_reason(reason)},
				  reason{reason} {}

			void pretty_print_to(string_type& dest) const;

			[[nodiscard]] string_type pretty_print() const;
		};
	}// namespace exception

	namespace ast
	{
		using ast_node_ptr = std::unique_ptr<ast_node>;

		template<typename NodeType, typename... Args>
			requires std::derived_from<NodeType, ast_node>
		[[nodiscard]] ast_node_ptr make_node(Args&&... args) { return std::make_unique<NodeType>(std::forward<Args>(args)...); }

		class ast_visitor_base
		{
		public:
			constexpr ast_visitor_base() noexcept = default;
			constexpr virtual ~ast_visitor_base() noexcept = default;
			constexpr ast_visitor_base(const ast_visitor_base&) = default;
			constexpr ast_visitor_base& operator=(const ast_visitor_base&) = default;
			constexpr ast_visitor_base(ast_visitor_base&&) = default;
			constexpr ast_visitor_base& operator=(ast_visitor_base&&) = default;

			virtual void visit(const ast_node& node) = 0;
		};

		class ast_optimizer_base
		{
		public:
			constexpr ast_optimizer_base() = default;
			constexpr virtual ~ast_optimizer_base() noexcept = default;
			constexpr ast_optimizer_base(const ast_optimizer_base&) = default;
			constexpr ast_optimizer_base& operator=(const ast_optimizer_base&) = default;
			constexpr ast_optimizer_base(ast_optimizer_base&&) = default;
			constexpr ast_optimizer_base& operator=(ast_optimizer_base&&) = default;

			[[nodiscard]] virtual ast_node_ptr optimize(ast_node_ptr node) = 0;
		};

		class ast_parser_base
		{
		public:
			ast_parser_base() = default;
			virtual ~ast_parser_base() noexcept = default;

			ast_parser_base(ast_parser_base&&) = default;

			ast_parser_base& operator=(const ast_parser_base&) = delete;
			ast_parser_base& operator=(ast_parser_base&&) = delete;

		protected:
			ast_parser_base(const ast_parser_base&) = default;

		public:
			[[nodiscard]] virtual ast_node_ptr parse(foundation::string_view_type input, parse_location::filename_type filename) = 0;

			[[nodiscard]] virtual ast_visitor_base& get_visitor() = 0;

			[[nodiscard]] virtual ast_optimizer_base& get_optimizer() = 0;

			[[nodiscard]] virtual std::string debug_print(const ast_node& node, foundation::string_view_type prepend) const = 0;

			virtual void debug_print_to(foundation::string_type& dest, const ast_node& node, foundation::string_view_type prepend) const = 0;
		};

		namespace ast_detail
		{
			using ast_rtti_index_type = int;

			struct ast_rtti_index_counter
			{
				inline static ast_rtti_index_type index = 0;
			};

			template<typename T>
			struct ast_rtti
			{
				const static ast_rtti_index_type value;
			};

			template<typename T>
			const ast_rtti_index_type ast_rtti<T>::value = ++ast_rtti_index_counter::index;

			struct ast_rtti_manager
			{
			private:
				inline static std::map<ast_rtti_index_type, foundation::string_type> ast_rtti_info_{};

			public:
				static foundation::string_view_type register_ast_rtti_name(const ast_rtti_index_type index, const foundation::string_view_type name) { return ast_rtti_info_.emplace(index, name).first->second; }

				[[nodiscard]] static foundation::string_view_type nameof(const ast_rtti_index_type index) { return ast_rtti_info_[index]; }
			};

			#define GAL_AST_SET_RTTI(class_name)                       \
		constexpr static auto get_rtti_index() noexcept        \
		{                                                      \
			return ast_detail::ast_rtti<class_name>::value; \
		}                                                      \
		inline static auto ast_register_name = ast_detail::ast_rtti_manager::register_ast_rtti_name(ast_detail::ast_rtti<class_name>::value, #class_name);

			template<typename T>
			concept has_rtti_index = requires(T t)
			{
				{
					T::get_rtti_index()
				} -> std::same_as<ast_rtti_index_type>;
			};

			template<typename T>
			struct ast_node_common_base
			{
				template<typename>
				friend struct ast_node_common_base;

				// for ast_node::remake_node
				friend struct ast::ast_node;

				using text_type = foundation::string_type;
				using identifier_type = foundation::string_view_type;

			private:
				ast_rtti_index_type class_index_{};
				parse_location location_;
				identifier_type identifier_;

				// for cast ctor below
				template<typename U>
				explicit operator const ast_node_common_base<U>&() const { return reinterpret_cast<const ast_node_common_base<U>&>(*this); }

			public:
				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr bool is() const noexcept { return class_index_ == TargetNode::get_rtti_index(); }

				template<has_rtti_index... TargetNode>
				[[nodiscard]] constexpr bool is_any() const noexcept { return ((class_index_ == TargetNode::get_rtti_index()) || ...); }

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr TargetNode* as() noexcept { return class_index_ == TargetNode::get_rtti_index() ? static_cast<TargetNode*>(this) : nullptr; }

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr const TargetNode* as() const noexcept { return class_index_ == TargetNode::get_rtti_index() ? static_cast<const TargetNode*>(this) : nullptr; }

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr TargetNode* as_no_check() noexcept
				{
					gal_assert(is<TargetNode>());
					return static_cast<TargetNode*>(this);
				}

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr const TargetNode* as_no_check() const noexcept
				{
					gal_assert(is<TargetNode>());
					return static_cast<const TargetNode*>(this);
				}

				[[nodiscard]] constexpr identifier_type identifier() const noexcept { return identifier_; }

				[[nodiscard]] parse_location::filename_type filename() const noexcept { return location_.filename; }

				[[nodiscard]] file_point location_begin() const noexcept { return location_.location.begin; }

				[[nodiscard]] file_point location_end() const noexcept { return location_.location.end; }

				explicit ast_node_common_base(
						const ast_rtti_index_type index,
						const identifier_type text,
						parse_location location)
					: class_index_{index},
					  location_{location},
					  identifier_{text} {}

				// for ast_node -> ast_node_tracer
				template<typename U>
					requires(not std::is_same_v<U, T>)
				explicit ast_node_common_base(const ast_node_common_base<U>& other)
					: ast_node_common_base{static_cast<const ast_node_common_base<T>&>(other)} {}

				explicit ast_node_common_base(
						const ast_rtti_index_type index,
						T&& node)
					: ast_node_common_base{index, node.identifier_, std::move(node.location_)} {}

				explicit ast_node_common_base(
						const ast_rtti_index_type index,
						const T& node)
					: class_index_{index},
					  location_{node.location_},
					  identifier_{node.identifier_} {}

				void pretty_format_position_to(text_type& target) const
				{
					std_format::format_to(std::back_inserter(target),
					                      "(line: {}, column: {} in file '{}')",
					                      location_begin().line,
					                      location_begin().column,
					                      filename());
				}

				[[nodiscard]] text_type pretty_position_print() const
				{
					text_type ret;
					pretty_format_position_to(ret);
					return ret;
				}

				void pretty_format_to(text_type& target) const
				{
					target.append(identifier_);
					std::ranges::for_each(
							static_cast<const T&>(*this).view(),
							[&target](const auto& child) { child.pretty_format_to(target); });
				}

				[[nodiscard]] text_type pretty_print() const
				{
					text_type result;
					pretty_format_to(result);
					return result;
				}

				void to_string_to(text_type& target, const foundation::string_view_type prepend = "") const
				{
					std_format::format_to(
							std::back_inserter(target),
							"{} {}(class index: {}) identifier: '{}' at:\n\t",
							prepend,
							ast_rtti_manager::nameof(class_index_),
							class_index_,
							identifier_);
					pretty_format_position_to(target);

					std_format::format_to(
							std::back_inserter(target),
							"\n\twith {}(s) child node: \n\n",
							static_cast<const T&>(*this).size());

					std::ranges::for_each(
							static_cast<const T&>(*this).view(),
							[&target, prepend](const auto& child) { child.to_string_to(target, prepend); });
				}

				/**
			 * @brief Prints the contents of an AST node, including its children, recursively
			 */
				[[nodiscard]] text_type to_string(const std::string_view prepend = "") const
				{
					text_type result;
					to_string_to(result, prepend);
					return result;
				}
			};
		}// namespace ast_detail

		struct ast_node : ast_detail::ast_node_common_base<ast_node>
		{
			using children_type = std::vector<ast_node_ptr>;

		protected:
			children_type children_;

			ast_node(
					const ast_detail::ast_rtti_index_type index,
					const identifier_type identifier,
					const parse_location location,
					children_type&& children = {})
				: ast_node_common_base{index, identifier, location},
				  children_{std::move(children)} {}

			explicit ast_node(
					ast_node_common_base&& base,
					children_type&& children = {})
				: ast_node_common_base{base},
				  children_{std::move(children)} {}

		public:
			ast_node(const ast_node&) = delete;
			ast_node& operator=(const ast_node&) = delete;
			ast_node(ast_node&&) = default;
			ast_node& operator=(ast_node&&) = default;
			virtual ~ast_node() noexcept = default;

		private:
			/**
			 * @throw exception::eval_error
			 */
			virtual foundation::boxed_value do_eval([[maybe_unused]] const foundation::dispatcher_state& state, [[maybe_unused]] ast_visitor_base& visitor) { throw std::runtime_error{"un-dispatched ast_node (internal error)"}; }

		public:
			foundation::boxed_value eval(const foundation::dispatcher_state& state, ast_visitor_base& visitor);

			template<typename NodeType, typename... Args>
				requires std::is_base_of_v<ast_node, NodeType>
			[[nodiscard]] ast_node_ptr remake_node(Args&&... extra_args) &&
			{
				if constexpr (std::is_constructible_v<NodeType, identifier_type, parse_location, Args...>)
				{
					// there are some ast_nodes that have no children
					return ast::make_node<NodeType>(identifier_, location_, std::forward<Args>(extra_args)...);
				}
				else if (std::is_constructible_v<NodeType, identifier_type, parse_location, children_type&&, Args...>)
				{
					// construct node with children
					return ast::make_node<NodeType>(identifier_, location_, std::move(children_), std::forward<Args>(extra_args)...);
				}
				else
				{
					gal_assert(false, "unknown ast_node constructor");
					UNREACHABLE();
				}
			}

			/**
			 * @throw eval_error
			 */
			static bool get_bool_condition(const foundation::boxed_value& object, const foundation::dispatcher_state& state)
			{
				try { return state->boxed_cast<bool>(object); }
				catch (const exception::bad_boxed_cast&) { throw exception::eval_error{"Condition not boolean"}; }
			}

			/**
			 * @throw eval_error
			 */
			static bool get_scoped_bool_condition(ast_node& node, const foundation::dispatcher_state& state, ast_visitor_base& visitor)
			{
				foundation::scoped_scope scoped_scope{state};
				return get_bool_condition(node.eval(state, visitor), state);
			}

		private:
			[[nodiscard]] constexpr auto get_unwrapped_children() { return children_ | std::views::transform([](auto& ptr) -> ast_node& { return *ptr; }); }

			[[nodiscard]] constexpr auto get_unwrapped_children() const { return children_ | std::views::transform([](const auto& ptr) -> const ast_node& { return *ptr; }); }

		public:
			[[nodiscard]] children_type exchange_children(
					children_type&& new_children = {} GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
							,
							const foundation::string_view_type reason = "no reason",
							const std_source_location& location = std_source_location::current()))
			{
				GAL_LANG_RECODE_CALL_LOCATION_DEBUG_DO(
						utils::logger::info("'{}' from (file: '{}' function: '{}' position: ({}:{})), try to exchange children with a '{}' element(s) children because '{}'",
							__func__,
							location.file_name(),
							location.function_name(),
							location.line(),
							location.column(),
							new_children.size(),
							reason);)

				return std::exchange(children_, std::move(new_children));
			}

			[[nodiscard]] constexpr children_type::size_type size() const noexcept { return children_.size(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return children_.empty(); }

			[[nodiscard]] constexpr ast_node_ptr& get_child_ptr(const children_type::size_type index) noexcept { return children_[index]; }

			[[nodiscard]] constexpr const ast_node_ptr& get_child_ptr(const children_type::size_type index) const noexcept { return const_cast<ast_node&>(*this).get_child_ptr(index); }

			[[nodiscard]] constexpr ast_node_ptr& front_ptr() noexcept { return children_.front(); }

			[[nodiscard]] constexpr const ast_node_ptr& front_ptr() const noexcept { return children_.front(); }

			[[nodiscard]] constexpr ast_node_ptr& back_ptr() noexcept { return children_.back(); }

			[[nodiscard]] constexpr const ast_node_ptr& back_ptr() const noexcept { return children_.back(); }

			[[nodiscard]] constexpr auto view_ptr() noexcept { return children_ | std::views::all; }

			[[nodiscard]] constexpr auto view_ptr() const noexcept { return children_ | std::views::all; }

			[[nodiscard]] constexpr auto sub_view_ptr(const children_type::difference_type begin, const children_type::difference_type count) noexcept { return view_ptr() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view_ptr(const children_type::difference_type begin, const children_type::difference_type count) const noexcept { return view_ptr() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view_ptr(const children_type::difference_type begin) noexcept { return view_ptr() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto sub_view_ptr(const children_type::difference_type begin) const noexcept { return view_ptr() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto front_view_ptr(const children_type::difference_type count) noexcept { return sub_view_ptr(0, count); }

			[[nodiscard]] constexpr auto front_view_ptr(const children_type::difference_type count) const noexcept { return sub_view_ptr(0, count); }

			[[nodiscard]] constexpr auto back_view_ptr(const children_type::difference_type count) noexcept { return view_ptr() | std::views::reverse | std::views::take(count) | std::views::reverse; }

			[[nodiscard]] constexpr auto back_view_ptr(const children_type::difference_type count) const noexcept { return view_ptr() | std::views::reverse | std::views::take(count) | std::views::reverse; }

			[[nodiscard]] constexpr ast_node& get_child(const children_type::difference_type index) noexcept { return get_unwrapped_children().operator[](index); }

			[[nodiscard]] constexpr const ast_node& get_child(const children_type::difference_type index) const noexcept { return const_cast<ast_node&>(*this).get_child(index); }

			[[nodiscard]] constexpr ast_node& front() noexcept { return get_unwrapped_children().front(); }

			[[nodiscard]] constexpr const ast_node& front() const noexcept { return const_cast<ast_node&>(*this).front(); }

			[[nodiscard]] constexpr ast_node& back() noexcept { return get_unwrapped_children().back(); }

			[[nodiscard]] constexpr const ast_node& back() const noexcept { return const_cast<ast_node&>(*this).back(); }

			// todo: begin/end returns a `new` view's begin/end each time, so comparing them will generate an error
			// [[nodiscard]] [[deprecated("use view instead")]] constexpr auto begin() noexcept { return get_unwrapped_children().begin(); }

			// [[nodiscard]] [[deprecated("use view instead")]] constexpr auto begin() const noexcept { return get_unwrapped_children().begin(); }

			// [[nodiscard]] [[deprecated("use view instead")]] constexpr auto end() noexcept { return get_unwrapped_children().end(); }

			// [[nodiscard]] [[deprecated("use view instead")]] constexpr auto end() const noexcept { return get_unwrapped_children().end(); }

			[[nodiscard]] constexpr auto view() noexcept { return get_unwrapped_children(); }

			[[nodiscard]] constexpr auto view() const noexcept { return get_unwrapped_children(); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) noexcept { return view() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) const noexcept { return view() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) noexcept { return view() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) const noexcept { return view() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) const noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) noexcept { return view() | std::views::reverse | std::views::take(count) | std::views::reverse; }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) const noexcept { return view() | std::views::reverse | std::views::take(count) | std::views::reverse; }
		};

		struct ast_node_tracer : ast_detail::ast_node_common_base<ast_node_tracer>
		{
			GAL_AST_SET_RTTI(ast_node_tracer)

			using children_type = std::vector<ast_node_tracer>;

			children_type children;

			explicit ast_node_tracer(const ast_node& node)
				: ast_node_common_base{node}
			{
				children.reserve(node.size());
				std::ranges::copy(node.view() | std::views::transform([](const auto& n) { return static_cast<ast_node_tracer>(n); }),
				                  std::back_inserter(children));
			}

			[[nodiscard]] constexpr children_type::size_type size() const noexcept { return children.size(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return children.empty(); }

			[[nodiscard]] constexpr ast_node_tracer& get_child(const children_type::difference_type index) noexcept { return children[index]; }

			[[nodiscard]] constexpr const ast_node_tracer& get_child(const children_type::difference_type index) const noexcept { return const_cast<ast_node_tracer&>(*this).get_child(index); }

			[[nodiscard]] constexpr ast_node_tracer& front() noexcept { return children.front(); }

			[[nodiscard]] constexpr const ast_node_tracer& front() const noexcept { return const_cast<ast_node_tracer&>(*this).front(); }

			[[nodiscard]] constexpr ast_node_tracer& back() noexcept { return children.back(); }

			[[nodiscard]] constexpr const ast_node_tracer& back() const noexcept { return const_cast<ast_node_tracer&>(*this).back(); }

			[[nodiscard]] constexpr auto view() noexcept { return children | std::views::all; }

			[[nodiscard]] constexpr auto view() const noexcept { return children | std::views::all; }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) noexcept { return view() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) const noexcept { return view() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) noexcept { return view() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) const noexcept { return view() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) const noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) noexcept { return view() | std::views::reverse | std::views::take(count) | std::views::reverse; }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) const noexcept { return view() | std::views::reverse | std::views::take(count) | std::views::reverse; }
		};
	}

	namespace exception
	{
		inline void eval_error::format_types(string_type& target, const foundation::const_function_proxy_type& function, const bool has_dot_notation, const foundation::dispatcher& dispatcher)
		{
			gal_assert(function.operator bool());

			const auto arity = function->arity_size();
			const auto types = function->type_view();

			if (arity == foundation::function_proxy_base::no_parameters_arity)
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
					                      "{}({})",
					                      dispatcher.nameof(*it),
					                      it->is_const() ? "immutable" : "mutable");

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

			if (const auto fun = std::dynamic_pointer_cast<const foundation::dynamic_function_proxy_base>(function);
				fun && fun->has_function_body())
			{
				if (const auto guard = fun->get_guard())
				{
					if (const auto guard_fun = std::dynamic_pointer_cast<const foundation::dynamic_function_proxy_base>(guard);
						guard_fun && guard_fun->has_function_body())
					{
						target.append(foundation::keyword_function_guard_name::value);
						guard_fun->get_function_body().pretty_format_to(target);
					}
				}

				target.append("\n\tDefined at: ");
				fun->get_function_body().pretty_format_position_to(target);
			}
		}

		inline eval_error::string_type eval_error::pretty_print() const
		{
			string_type ret{};
			pretty_print_to(ret);
			return ret;
		}
	}// namespace exception
}

#endif
