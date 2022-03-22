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
		struct ast_node_trace;

		struct name_validator
		{
			using name_type = foundation::string_view_type;

			[[nodiscard]] static bool is_reserved_name(const name_type name) noexcept
			{
				constexpr auto hash = [](const name_type s) constexpr noexcept { return utils::hash_fnv1a<false>(s); };

				const static std::unordered_set names{
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

				return names.contains(hash(name));
			}

			[[nodiscard]] static bool is_valid_object_name(const name_type name) noexcept { return not name.contains(keyword_class_scope_name::value) && not is_reserved_name(name); }

			static void validate_object_name(const name_type name)
			{
				if (is_reserved_name(name)) { throw exception::reserved_word_error{name}; }

				if (name.contains(keyword_class_scope_name::value)) { throw exception::illegal_name_error{name}; }
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
		 * @brief Errors generated during parsing or evaluation.
		 */
		class eval_error final : public std::runtime_error
		{
		public:
			std::string reason;
			std::string filename;
			lang::file_point begin_position;
			std::string detail;
			std::vector<lang::ast_node_trace> stack_traces;

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
				// todo: default eval filename
				if (f != "__EVAL__") { std_format::format_to(std::back_inserter(target), "in '{}' ", f); }
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
					const foundation::proxy_function& function,
					bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher);

			static std::string format_detail(
					const foundation::proxy_functions_view_type functions,
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
					const foundation::proxy_functions_view_type functions,
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
					const foundation::proxy_functions_view_type functions,
					const bool has_dot_notation,
					const foundation::dispatcher_detail::dispatcher& dispatcher)
				: std::runtime_error{format(reason, params, has_dot_notation, dispatcher)},
				  reason{reason},
				  detail{format_detail(functions, has_dot_notation, dispatcher)} {}

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
			return ast_rtti<class_name>::value;         \
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

			protected:
				ast_rtti_index_type class_index_;

			public:
				parse_location location;

				using text_type = std::string;
				// do not modify text.
				// todo: do we really need to keep the text?
				text_type text;

			private:
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

				template<typename U>
				explicit operator const ast_node_common_base<U>&() const { return reinterpret_cast<const ast_node_common_base<U>&>(*this); }

			public:
				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr bool is() const noexcept { return class_index_ == TargetNode::get_rtti_index(); }

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr TargetNode* as() noexcept { return class_index_ == TargetNode::get_rtti_index() ? static_cast<TargetNode*>(this) : nullptr; }

				template<has_rtti_index TargetNode>
				[[nodiscard]] constexpr const TargetNode* as() const noexcept { return class_index_ == TargetNode::get_rtti_index() ? static_cast<const TargetNode*>(this) : nullptr; }

				explicit ast_node_common_base(
						const ast_rtti_index_type index,
						text_type&& text,
						parse_location&& location)
					: class_index_{index},
					  location{std::move(location)},
					  text{std::move(text)} {}

				template<typename U>
					requires(not std::is_same_v<U, T>)
				explicit ast_node_common_base(const ast_node_common_base<U>& other)
					: ast_node_common_base{static_cast<const ast_node_common_base<T>&>(other)} {}

				[[nodiscard]] const parse_location::filename_type& filename() const noexcept { return *location.filename; }

				[[nodiscard]] file_point location_begin() const noexcept { return location.location.begin; }

				[[nodiscard]] file_point location_end() const noexcept { return location.location.end; }

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
					target.append(text);
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
							"{}(class index: {}) {} : ",
							prepend,
							class_index_,
							text);
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

		template<typename>
		struct ast_node;
		template<typename T>
		using ast_node_ptr = std::unique_ptr<ast_node<T>>;

		template<typename NodeType, typename... Args>
			requires std::derived_from<NodeType, ast_node<typename NodeType::tracer_type>>
		[[nodiscard]] ast_node_ptr<typename NodeType::tracer_type> make_node(Args&&... args) { return std::make_unique<NodeType>(std::forward<Args>(args)...); }

		struct ast_node_base : common_detail::ast_node_common_base<ast_node_base>
		{
			friend struct ast_node_common_base<ast_node_base>;

		protected:
			using ast_node_common_base<ast_node_base>::ast_node_common_base;

		private:
			// todo: return type
			[[nodiscard]] virtual std::vector<std::reference_wrapper<ast_node_base>> get_children() const noexcept = 0;
			// compromise :(
			static const ast_node_base& unwrap_child(const std::reference_wrapper<ast_node_base>& child) noexcept { return child.get(); }

		public:
			virtual ~ast_node_base() noexcept = default;
			ast_node_base(const ast_node_base&) = default;
			ast_node_base& operator=(const ast_node_base&) = default;
			ast_node_base(ast_node_base&&) = default;
			ast_node_base& operator=(ast_node_base&&) = default;

			template<typename Function>
			void apply(Function&& function) const
			{
				const auto& children = get_children();
				std::ranges::for_each(
						children | std::views::transform(unwrap_child),
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

			[[nodiscard]] virtual foundation::boxed_value eval(const foundation::dispatcher_detail::dispatcher_state& state) const = 0;
		};

		struct ast_node_trace : common_detail::ast_node_common_base<ast_node_trace>
		{
			using children_type = std::vector<ast_node_trace>;

			children_type children;

			explicit ast_node_trace(const ast_node_base& node);

			template<typename Function>
			void apply(Function&& function)
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

		template<typename Tracer>
		struct ast_node : ast_node_base
		{
			using tracer_type = Tracer;
			// although we would like to use concept in the template parameter list,
			// unfortunately we can't use it in concept without knowing the type of ast_node_impl,
			// so we can only use this unfriendly way.
			static_assert(
				requires(const foundation::dispatcher_detail::dispatcher_state& s, const ast_node<tracer_type>* n)
				{
					tracer_type::trace(s, n);
				},
				"invalid tracer");

			using node_ptr_type = ast_node_ptr<tracer_type>;
			using children_type = std::vector<node_ptr_type>;

		private:
			template<typename>
			friend struct compiled_ast_node;
			children_type children_;

			[[nodiscard]] std::vector<std::reference_wrapper<ast_node_base>> get_children() const noexcept override
			{
				// return children_ | std::views::transform([](const auto& child_ptr) -> ast_node_base& { return *child_ptr; });
				std::vector<std::reference_wrapper<ast_node_base>> ret{};
				std::ranges::for_each(
						children_,
						[&ret](const auto& ptr) { ret.emplace_back(*ptr); });
				return ret;
			}

		public:
			ast_node(
					const common_detail::ast_rtti_index_type index,
					std::string&& text,
					parse_location&& location,
					children_type&& children = {})
				: ast_node_base{index, std::move(text), std::move(location)},
				  children_{std::move(children)} {}

		protected:
			[[nodiscard]] virtual foundation::boxed_value do_eval(const foundation::dispatcher_detail::dispatcher_state& state) const { throw std::runtime_error{"un-dispatched ast_node (internal error)"}; }

		public:
			/**
			 * @throw eval_error
			 */
			static bool get_scoped_bool_condition(const node_ptr_type& node, const foundation::dispatcher_detail::dispatcher_state& state)
			{
				foundation::dispatcher_detail::scoped_stack_scope scoped_stack{state};
				return get_bool_condition(node->eval(state), state);
			}

			[[nodiscard]] foundation::boxed_value eval(const foundation::dispatcher_detail::dispatcher_state& state) const final
			{
				try
				{
					tracer_type::trace(state, this);
					return do_eval(state);
				}
				catch (exception::eval_error& e)
				{
					e.stack_traces.push_back(*this);
					throw;
				}
			}

			void swap(children_type& children) noexcept
			{
				using std::swap;
				swap(children_, children);
			}

			[[nodiscard]] typename children_type::size_type size() const noexcept { return children_.size(); }

			[[nodiscard]] bool empty() const noexcept { return children_.empty(); }

			[[nodiscard]] ast_node<tracer_type>& get_child(typename children_type::size_type index) noexcept { return *children_[index]; }

			[[nodiscard]] const ast_node<tracer_type>& get_child(typename children_type::size_type index) const noexcept { return const_cast<ast_node<tracer_type>&>(*this).get_child(index); }

			[[nodiscard]] ast_node<tracer_type>& front() noexcept { return *children_.front(); }

			[[nodiscard]] const ast_node<tracer_type>& front() const noexcept { return const_cast<ast_node<tracer_type>&>(*this).front(); }

			[[nodiscard]] ast_node<tracer_type>& back() noexcept { return *children_.back(); }

			[[nodiscard]] const ast_node<tracer_type>& back() const noexcept { return const_cast<ast_node<tracer_type>&>(*this).back(); }

			struct child_iterator
			{
				using iterator_concept = typename children_type::iterator::iterator_concept;
				using iterator_category = typename children_type::iterator::iterator_category;
				using value_type = typename children_type::iterator::value_type;
				using difference_type = typename children_type::iterator::difference_type;
				using pointer = typename children_type::iterator::pointer;
				using reference = typename children_type::iterator::reference;

				typename children_type::iterator iterator{};

				[[nodiscard]] typename children_type::iterator& base() noexcept { return iterator; }

				[[nodiscard]] const typename children_type::iterator& base() const noexcept { return iterator; }

				[[nodiscard]] decltype(auto) operator*() const noexcept { return unwrap_child(*iterator); }

				[[nodiscard]] decltype(auto) operator->() const noexcept { return &unwrap_child(*iterator); }

				decltype(auto) operator++() noexcept
				{
					iterator.operator++();
					return *this;
				}

				decltype(auto) operator++(int) noexcept
				{
					auto tmp = *this;
					iterator.operator++(int{});
					return tmp;
				}

				decltype(auto) operator+(difference_type offset) noexcept
				{
					auto tmp = *this;
					tmp.iterator += offset;
					return tmp;
				}

				decltype(auto) operator--() const noexcept
				{
					iterator.operator--();
					return *this;
				}

				decltype(auto) operator--(int) const noexcept
				{
					auto tmp = *this;
					iterator.operator--(int{});
					return tmp;
				}

				decltype(auto) operator-(difference_type offset) noexcept
				{
					auto tmp = *this;
					tmp.iterator -= offset;
					return tmp;
				}

				decltype(auto) operator[](difference_type offset) const noexcept { return unwrap_child(iterator[offset]); }
			};

			[[nodiscard]] auto begin() noexcept { child_iterator{children_.begin()}; }

			[[nodiscard]] auto begin() const noexcept { return child_iterator{children_.begin()}; }

			[[nodiscard]] auto end() noexcept { child_iterator{children_.end()}; }

			[[nodiscard]] auto end() const noexcept { return child_iterator{children_.end()}; }
		};
	}// namespace lang

	namespace exception
	{
		inline void eval_error::format_types(
				std::string& target,
				const foundation::proxy_function& function,
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
	}
}// namespace gal::lang

#endif//GAL_LANG_LANGUAGE_COMMON_HPP
