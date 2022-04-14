#pragma once

#ifndef GAL_LANG_LANGUAGE_COMMON_HPP
#define GAL_LANG_LANGUAGE_COMMON_HPP

#include <unordered_set>
#include <utils/hash.hpp>
#include <utils/point.hpp>
#include <gal/foundation/dispatcher.hpp>

namespace gal::lang
{
	namespace lang
	{
		struct ast_node_tracer;

		struct name_validator
		{
			using name_type = foundation::string_view_type;

			[[nodiscard]] constexpr static auto name_hasher(const name_type name) noexcept { return utils::hash_fnv1a<false>(name); }

			[[nodiscard]] static bool is_reserved_name(const name_type name) noexcept
			{
				const static std::unordered_set names{
						name_hasher(keyword_define_name::value),
						name_hasher(keyword_function_name::value),
						name_hasher(keyword_variable_name::value),
						name_hasher(keyword_true_name::value),
						name_hasher(keyword_false_name::value),
						name_hasher(keyword_class_name::value),
						name_hasher(keyword_member_decl_name::value),
						name_hasher(keyword_global_name::value),
						name_hasher(keyword_placeholder_name::value),
						name_hasher(keyword_comma_name::value),
						name_hasher(keyword_while_name::value),
						name_hasher(keyword_for_name::value),
						name_hasher(keyword_break_name::value),
						name_hasher(keyword_if_name::value),
						name_hasher(keyword_else_name::value),
						name_hasher(keyword_logical_and_name::value),
						name_hasher(keyword_logical_or_name::value),
						name_hasher(keyword_return_name::value),
				};

				return names.contains(name_hasher(name));
			}

			[[nodiscard]] static bool is_valid_object_name(const name_type name) noexcept { return not name.contains(keyword_class_accessor_name::value) && not is_reserved_name(name); }

			static void validate_object_name(const name_type name)
			{
				if (is_reserved_name(name)) { throw exception::reserved_word_error{name}; }

				if (name.contains(keyword_class_accessor_name::value)) { throw exception::illegal_name_error{name}; }
			}
		};

		/**
		 * @brief Signature of module entry point that all binary loadable modules must implement.
		 */
		using core_maker_signature = foundation::shared_engine_core (*)();

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
			member_decl_t,
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

		constexpr foundation::string_view_type ast_node_name(const ast_node_type type) noexcept
		{
			// todo: name
			constexpr foundation::string_view_type node_type_names[]{
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
				  filename{std::move(filename)} { }

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
			std::string reason;
			std::string filename;
			lang::file_point begin_position;
			std::string detail;
			std::vector<lang::ast_node_tracer> stack_traces;

		private:
			static void format_reason(std::string& target, const std::string_view r) { std_format::format_to(std::back_inserter(target), "Error: '{}' ", r); }

			static std::string get_formatted_reason(const std::string_view r)
			{
				std::string ret;
				format_reason(ret, r);
				return ret;
			}

			static void format_parameters(
					std::string& target,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
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
						                      dispatcher.get_type_name(*it),
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
				if (f != lang::inline_eval_filename_name::value) { std_format::format_to(std::back_inserter(target), "in '{}' ", f); }
				else { std_format::format_to(std::back_inserter(target), "during evaluation "); }
			}

			static void format_position(std::string& target, const lang::file_point p) { std_format::format_to(std::back_inserter(target), "at ({}, {}) ", p.line, p.column); }

			static std::string format(
					const std::string_view r,
					const std::string_view f,
					const lang::file_point p,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
			{
				std::string ret;

				format_reason(ret, r);
				format_parameters(ret, params, has_dot_notation, dispatcher);
				format_filename(ret, f);
				format_position(ret, p);

				return ret;
			}

			static std::string format(
					const std::string_view r,
					const foundation::parameters_view_type params,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
			{
				std::string ret;

				format_reason(ret, r);
				format_parameters(ret, params, has_dot_notation, dispatcher);

				return ret;
			}

			static std::string format(
					const std::string_view r,
					const std::string_view f,
					const lang::file_point p)
			{
				std::string ret;

				format_reason(ret, r);
				format_filename(ret, f);
				format_position(ret, p);

				return ret;
			}

			static void format_types(
					std::string& target,
					const foundation::immutable_proxy_function& function,
					bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher);

			static std::string format_detail(
					const foundation::immutable_proxy_functions_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
			{
				std::string ret;

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
								format_types(ret, function, has_dot_notation, dispatcher);
								ret.push_back('\n');
							});
				}

				return ret;
			}

		public:
			eval_error(
					const std::string_view reason,
					const std::string_view filename,
					const lang::file_point begin_position,
					const foundation::parameters_view_type params,
					const foundation::immutable_proxy_functions_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: std::runtime_error{format(reason, filename, begin_position, params, has_dot_notation, dispatcher)},
				  reason{reason},
				  filename{filename},
				  begin_position{begin_position},
				  detail{format_detail(functions, has_dot_notation, dispatcher)} {}

			eval_error(
					const std::string_view reason,
					const foundation::parameters_view_type params,
					const foundation::immutable_proxy_functions_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: std::runtime_error{format(reason, params, has_dot_notation, dispatcher)},
				  reason{reason},
				  detail{format_detail(functions, has_dot_notation, dispatcher)} {}

			eval_error(
					const std::string_view reason,
					const foundation::parameters_type& params,
					const foundation::immutable_proxy_functions_type& functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: eval_error{reason, foundation::parameters_view_type{params}, foundation::immutable_proxy_functions_view_type{functions}, has_dot_notation, dispatcher} {}

			eval_error(
					const std::string_view reason,
					const foundation::parameters_view_type params,
					const foundation::immutable_proxy_functions_type& functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: eval_error{reason, params, foundation::immutable_proxy_functions_view_type{functions}, has_dot_notation, dispatcher} {}

			eval_error(
					const std::string_view reason,
					const foundation::parameters_type& params,
					const foundation::immutable_proxy_functions_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: eval_error{reason, foundation::parameters_view_type{params}, functions, has_dot_notation, dispatcher} {}

			eval_error(
					const std::string_view reason,
					const std::string_view filename,
					const lang::file_point begin_position)
				: std::runtime_error{format(reason, filename, begin_position)},
				  reason{reason},
				  filename{filename},
				  begin_position{begin_position} {}

			explicit eval_error(
					const std::string_view reason)
				: std::runtime_error{get_formatted_reason(reason)},
				  reason{reason} {}

			void pretty_print_to(std::string& dest) const;

			[[nodiscard]] std::string pretty_print() const;
		};
	}// namespace exception

	namespace lang
	{
		struct ast_node;
		using ast_node_ptr = std::unique_ptr<ast_node>;

		class ast_visitor
		{
		public:
			constexpr ast_visitor() noexcept = default;
			constexpr virtual ~ast_visitor() noexcept = default;
			constexpr ast_visitor(const ast_visitor&) = default;
			constexpr ast_visitor& operator=(const ast_visitor&) = default;
			constexpr ast_visitor(ast_visitor&&) = default;
			constexpr ast_visitor& operator=(ast_visitor&&) = default;

			virtual void visit(const ast_node& node)
			{
				(void)node;
				throw std::runtime_error{"visitor is not implemented"};
			}
		};

		class ast_optimizer
		{
		public:
			constexpr ast_optimizer() = default;
			constexpr virtual ~ast_optimizer() noexcept = default;
			constexpr ast_optimizer(const ast_optimizer&) = default;
			constexpr ast_optimizer& operator=(const ast_optimizer&) = default;
			constexpr ast_optimizer(ast_optimizer&&) = default;
			constexpr ast_optimizer& operator=(ast_optimizer&&) = default;

			// ReSharper disable once CppParameterMayBeConst
			[[nodiscard]] virtual ast_node_ptr optimize(ast_node_ptr node)
			{
				(void)node;
				throw std::runtime_error{"optimizer is not implemented"};
			}
		};

		template<typename NodeType, typename... Args>
			requires std::derived_from<NodeType, ast_node>
		[[nodiscard]] ast_node_ptr make_node(Args&&... args) { return std::make_unique<NodeType>(std::forward<Args>(args)...); }

		namespace common_detail
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

			#define GAL_AST_SET_RTTI(class_name)                    \
			constexpr static auto get_rtti_index() noexcept \
			{                                               \
				return common_detail::ast_rtti<class_name>::value;         \
			}

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
				friend struct lang::ast_node;

				using text_type = foundation::string_type;
				using identifier_type = foundation::string_view_type;

			private:
				ast_rtti_index_type class_index_{};
				parse_location location_;
				identifier_type identifier_;

				template<typename Function>
				void format_children_to(text_type& target, Function&& function) const
				{
					static_cast<const T&>(*this).apply(
							[&target, function](const auto& child)
							{
								function(child, target);
								target.push_back(' ');
							});
				}

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

				[[nodiscard]] parse_location::filename_type filename() const noexcept { return location_.filename; }

				[[nodiscard]] file_point location_begin() const noexcept { return location_.location.begin; }

				[[nodiscard]] file_point location_end() const noexcept { return location_.location.end; }

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
					format_children_to(target, [](const auto& child, text_type& t) { child.pretty_format_to(t); });
				}

				[[nodiscard]] text_type pretty_print() const
				{
					text_type result;
					pretty_format_to(result);
					return result;
				}

				void to_string_to(text_type& target, const std::string_view prepend = "") const
				{
					std_format::format_to(
							std::back_inserter(target),
							"{}(class index: {}) identifier: {} at:\n ",
							prepend,
							class_index_,
							identifier_);
					pretty_format_position_to(target);
					target.push_back('\n');

					format_children_to(target, [prepend](const auto& child, text_type& t) { child.to_string_to(t, prepend); });
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
		}// namespace common_detail

		struct ast_node : common_detail::ast_node_common_base<ast_node>
		{
			// for access children
			friend struct ast_node_tracer;
			friend struct compiled_ast_node;

			using children_type = std::vector<ast_node_ptr>;

		protected:
			ast_node(
					const common_detail::ast_rtti_index_type index,
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

			[[nodiscard]] virtual foundation::boxed_value do_eval([[maybe_unused]] const foundation::dispatcher_detail::dispatcher_state& state, [[maybe_unused]] ast_visitor& visitor) const { throw std::runtime_error{"un-dispatched ast_node (internal error)"}; }

		private:
			children_type children_;

			[[nodiscard]] constexpr auto get_unwrapped_children() { return children_ | std::views::transform([](auto& ptr) -> ast_node& { return *ptr; }); }

			[[nodiscard]] constexpr auto get_unwrapped_children() const { return children_ | std::views::transform([](const auto& ptr) -> const ast_node& { return *ptr; }); }

		public:
			[[nodiscard]] foundation::parameters_type get_boxed_children() const
			{
				const auto children = get_unwrapped_children() | std::views::transform([](const auto& child) { return std::ref(child); });

				foundation::parameters_type ret{};
				ret.reserve(children.size());

				std::ranges::transform(
						children,
						std::back_inserter(ret),
						var<const std::reference_wrapper<const ast_node>&>);

				return ret;
			}

			virtual ~ast_node() noexcept = default;
			// due to unique_ptr
			ast_node(const ast_node&) = delete;
			ast_node& operator=(const ast_node&) = delete;
			ast_node(ast_node&&) = default;
			ast_node& operator=(ast_node&&) = default;

			template<typename NodeType, typename... Args>
				requires std::is_base_of_v<ast_node, NodeType>
			[[nodiscard]] ast_node_ptr remake_node(Args&&... extra_args) && { return lang::make_node<NodeType>(identifier_, location_, std::move(children_), std::forward<Args>(extra_args)...); }

			template<typename Function>
				requires (std::is_invocable_v<Function, ast_node&> or std::is_invocable_v<Function, const ast_node&>)
			void apply(Function&& function) const
			{
				std::ranges::for_each(
						get_unwrapped_children(),
						std::forward<Function>(function));
			}

			/**
			 * @throw eval_error
			 */
			static bool get_bool_condition(const foundation::boxed_value& object, const foundation::dispatcher_detail::dispatcher_state& state)
			{
				try { return state->boxed_cast<bool>(object); }
				catch (const exception::bad_boxed_cast&) { throw exception::eval_error{"Condition not boolean"}; }
			}

			/**
			 * @throw eval_error
			 */
			static bool get_scoped_bool_condition(const ast_node& node, const foundation::dispatcher_detail::dispatcher_state& state, ast_visitor& visitor)
			{
				foundation::dispatcher_detail::scoped_scope scoped_scope{state};
				return get_bool_condition(node.eval(state, visitor), state);
			}

			foundation::boxed_value eval(const foundation::dispatcher_detail::dispatcher_state& state, ast_visitor& visitor) const;

			void swap(children_type& children) noexcept
			{
				using std::swap;
				swap(children_, children);
			}

			[[nodiscard]] constexpr children_type::size_type size() const noexcept { return children_.size(); }

			[[nodiscard]] constexpr bool empty() const noexcept { return children_.empty(); }

			[[nodiscard]] constexpr ast_node_ptr& get_child_ptr(const children_type::size_type index) noexcept { return children_[index]; }

			[[nodiscard]] constexpr const ast_node_ptr& get_child_ptr(const children_type::size_type index) const noexcept { return const_cast<ast_node&>(*this).get_child_ptr(index); }

			[[nodiscard]] constexpr ast_node_ptr& front_ptr() noexcept { return children_.front(); }

			[[nodiscard]] constexpr const ast_node_ptr& front_ptr() const noexcept { return children_.front(); }

			[[nodiscard]] constexpr ast_node_ptr& back_ptr() noexcept { return children_.back(); }

			[[nodiscard]] constexpr const ast_node_ptr& back_ptr() const noexcept { return children_.back(); }

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

			[[nodiscard]] constexpr auto view() const noexcept { return get_unwrapped_children(); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) noexcept { return get_unwrapped_children() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin, const children_type::difference_type count) const noexcept { return get_unwrapped_children() | std::views::drop(begin) | std::views::take(count); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) noexcept { return get_unwrapped_children() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto sub_view(const children_type::difference_type begin) const noexcept { return get_unwrapped_children() | std::views::drop(begin); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto front_view(const children_type::difference_type count) const noexcept { return sub_view(0, count); }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) noexcept { return get_unwrapped_children() | std::views::reverse | std::views::take(count) | std::views::reverse; }

			[[nodiscard]] constexpr auto back_view(const children_type::difference_type count) const noexcept { return get_unwrapped_children() | std::views::reverse | std::views::take(count) | std::views::reverse; }
		};

		struct ast_node_tracer : common_detail::ast_node_common_base<ast_node_tracer>
		{
			GAL_AST_SET_RTTI(ast_node_tracer)

			using children_type = std::vector<ast_node_tracer>;

			children_type children;

			explicit ast_node_tracer(const ast_node& node)
				: ast_node_common_base{node}//,
			// children{node.get_unwrapped_children().begin(), node.get_unwrapped_children().end()}
			{
				children.reserve(node.size());
				std::ranges::copy(node.view() | std::views::transform([](const auto& n) { return static_cast<ast_node_tracer>(n); }),
				                  std::back_inserter(children));
			}

			template<typename Function>
			void apply(Function&& function) const
			{
				std::ranges::for_each(
						children,
						std::forward<Function>(function));
			}

			[[nodiscard]] constexpr const children_type& get_children() const noexcept { return children; }

			[[nodiscard]] constexpr auto begin() noexcept { return children.begin(); }

			[[nodiscard]] constexpr auto begin() const noexcept { return children.begin(); }

			[[nodiscard]] constexpr auto end() noexcept { return children.end(); }

			[[nodiscard]] constexpr auto end() const noexcept { return children.end(); }
		};

		inline foundation::boxed_value ast_node::eval(const foundation::dispatcher_detail::dispatcher_state& state, ast_visitor& visitor) const
		{
			visitor.visit(*this);
			try { return do_eval(state, visitor); }
			catch (exception::eval_error& e)
			{
				e.stack_traces.emplace_back(*this);
				throw;
			}
		}
	}// namespace lang

	namespace exception
	{
		inline void eval_error::format_types(
				std::string& target,
				const foundation::immutable_proxy_function& function,
				const bool has_dot_notation,
				const foundation::dispatcher_detail::dispatcher& dispatcher
				)
		{
			gal_assert(function.operator bool());

			const auto arity = function->get_arity();
			const auto& types = function->types();

			if (arity == foundation::proxy_function_base::no_parameters_arity)
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
					                      dispatcher.get_type_name(*it),
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

			if (const auto fun = std::dynamic_pointer_cast<const foundation::dynamic_proxy_function_base>(function);
				fun && fun->has_parse_tree())
			{
				if (const auto guard = fun->get_guard())
				{
					if (const auto guard_fun = std::dynamic_pointer_cast<const foundation::dynamic_proxy_function_base>(guard);
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

		inline std::string eval_error::pretty_print() const
		{
			std::string ret{};
			pretty_print_to(ret);
			return ret;
		}
	}

	namespace parser_detail
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
			[[nodiscard]] virtual lang::ast_visitor& get_visitor() = 0;
			[[nodiscard]] virtual lang::ast_node_ptr parse(foundation::string_view_type input, foundation::string_view_type filename) = 0;
			[[nodiscard]] virtual std::string debug_print(const lang::ast_node& node, foundation::string_view_type prepend) const = 0;
			virtual void debug_print_to(foundation::string_type& dest, const lang::ast_node& node, foundation::string_view_type prepend) const = 0;
		};
	}

	// todo: better way
	namespace interrupt_type
	{
		/**
		 * @brief Special type for returned values
		 */
		struct return_value
		{
			foundation::boxed_value value;
		};

		/**
		 * @brief Special type indicating a call to 'break'
		 */
		struct break_loop { };

		/**
		 * @brief Special type indicating a call to 'continue'
		 */
		struct continue_loop { };
	}
}// namespace gal::lang

#endif//GAL_LANG_LANGUAGE_COMMON_HPP
