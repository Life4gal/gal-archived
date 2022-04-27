#pragma once

#ifndef GAL_LANG_LANGUAGE_PARSER_HPP
#define GAL_LANG_LANGUAGE_PARSER_HPP

#include <gal/language/eval.hpp>
#include <gal/language/common.hpp>
#include <utils/string_utils.hpp>
#include <utils/enum_utils.hpp>

namespace gal::lang
{
	namespace parser_detail
	{
		enum class alphabet
		{
			symbol = 0,
			keyword,
			identifier,
			whitespace,

			integer,
			floating_point,

			integer_suffix,
			floating_point_suffix,

			bin_prefix,
			bin,
			hex_prefix,
			hex,

			alphabet_size
		};

		constexpr std::size_t max_alphabet_length = 1 << 8;

		struct alphabet_matcher
		{
		private:
			using matrix_type = std::array<std::array<bool, max_alphabet_length>, static_cast<std::size_t>(alphabet::alphabet_size)>;

			[[nodiscard]] constexpr static matrix_type build() noexcept
			{
				matrix_type abs{};

				constexpr auto setter = [](matrix_type& a, alphabet which_kind, const int what) constexpr noexcept { a[static_cast<std::size_t>(which_kind)][static_cast<std::size_t>(what)] = true; };

				setter(abs, alphabet::symbol, lang::operator_assign_name::value[0]);

				setter(abs, alphabet::symbol, lang::operator_less_than_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_greater_than_name::value[0]);

				setter(abs, alphabet::symbol, lang::operator_plus_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_minus_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_multiply_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_divide_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_remainder_name::value[0]);

				setter(abs, alphabet::symbol, lang::operator_bitwise_and_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_bitwise_or_name::value[0]);
				setter(abs, alphabet::symbol, lang::operator_bitwise_xor_name::value[0]);

				setter(abs, alphabet::symbol, '.');
				setter(abs, alphabet::floating_point, '.');
				setter(abs, alphabet::keyword, '_');
				setter(abs, alphabet::identifier, '_');

				setter(abs, alphabet::whitespace, ' ');
				setter(abs, alphabet::whitespace, '\t');

				for (auto i = 'a'; i < 'z'; ++i)
				{
					setter(abs, alphabet::keyword, i);
					setter(abs, alphabet::keyword, i - ('a' - 'A'));

					setter(abs, alphabet::identifier, i);
					setter(abs, alphabet::identifier, i - ('a' - 'A'));
				}

				for (auto i = '0'; i < '9'; ++i)
				{
					setter(abs, alphabet::keyword, i);

					setter(abs, alphabet::integer, i);
					setter(abs, alphabet::floating_point, i);
					setter(abs, alphabet::hex, i);
				}

				// see also suffix_matcher::check
				setter(abs, alphabet::integer_suffix, 'l');
				setter(abs, alphabet::integer_suffix, 'L');
				setter(abs, alphabet::integer_suffix, 'u');
				setter(abs, alphabet::integer_suffix, 'U');
				setter(abs, alphabet::floating_point_suffix, 'l');
				setter(abs, alphabet::floating_point_suffix, 'L');
				setter(abs, alphabet::floating_point_suffix, 'f');
				setter(abs, alphabet::floating_point_suffix, 'F');

				for (auto i = '0'; i < '1'; ++i) { setter(abs, alphabet::bin, i); }
				setter(abs, alphabet::bin_prefix, 'b');
				setter(abs, alphabet::bin_prefix, 'B');

				for (auto i = 'a'; i < 'f'; ++i)
				{
					setter(abs, alphabet::hex, i);
					setter(abs, alphabet::hex, i - ('a' - 'A'));
				}
				setter(abs, alphabet::hex_prefix, 'x');
				setter(abs, alphabet::hex_prefix, 'X');

				return abs;
			}

			constexpr static auto alphabets = []() constexpr noexcept
					{
						matrix_type abs{};

						constexpr auto setter = [](matrix_type& a, alphabet which_kind, const int what) constexpr noexcept { a[static_cast<std::size_t>(which_kind)][static_cast<std::size_t>(what)] = true; };

						setter(abs, alphabet::symbol, lang::operator_assign_name::value[0]);

						setter(abs, alphabet::symbol, lang::operator_less_than_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_greater_than_name::value[0]);

						setter(abs, alphabet::symbol, lang::operator_plus_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_minus_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_multiply_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_divide_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_remainder_name::value[0]);

						setter(abs, alphabet::symbol, lang::operator_bitwise_and_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_bitwise_or_name::value[0]);
						setter(abs, alphabet::symbol, lang::operator_bitwise_xor_name::value[0]);

						setter(abs, alphabet::symbol, '.');
						setter(abs, alphabet::floating_point, '.');
						setter(abs, alphabet::keyword, '_');
						setter(abs, alphabet::identifier, '_');

						setter(abs, alphabet::whitespace, ' ');
						setter(abs, alphabet::whitespace, '\t');

						for (auto i = 'a'; i < 'z'; ++i)
						{
							setter(abs, alphabet::keyword, i);
							setter(abs, alphabet::keyword, i - ('a' - 'A'));

							setter(abs, alphabet::identifier, i);
							setter(abs, alphabet::identifier, i - ('a' - 'A'));
						}

						for (auto i = '0'; i < '9'; ++i)
						{
							setter(abs, alphabet::keyword, i);

							setter(abs, alphabet::integer, i);
							setter(abs, alphabet::floating_point, i);
							setter(abs, alphabet::hex, i);
						}

						// see also suffix_matcher::check
						setter(abs, alphabet::integer_suffix, 'l');
						setter(abs, alphabet::integer_suffix, 'L');
						setter(abs, alphabet::integer_suffix, 'u');
						setter(abs, alphabet::integer_suffix, 'U');
						setter(abs, alphabet::floating_point_suffix, 'l');
						setter(abs, alphabet::floating_point_suffix, 'L');
						setter(abs, alphabet::floating_point_suffix, 'f');
						setter(abs, alphabet::floating_point_suffix, 'F');

						for (auto i = '0'; i < '1'; ++i) { setter(abs, alphabet::bin, i); }
						setter(abs, alphabet::bin_prefix, 'b');
						setter(abs, alphabet::bin_prefix, 'B');

						for (auto i = 'a'; i < 'f'; ++i)
						{
							setter(abs, alphabet::hex, i);
							setter(abs, alphabet::hex, i - ('a' - 'A'));
						}
						setter(abs, alphabet::hex_prefix, 'x');
						setter(abs, alphabet::hex_prefix, 'X');

						return abs;
					}
					();
			// todo: calling build fails to generate constant expressions?
			//build();

		public:
			[[nodiscard]] constexpr static bool belong(const char c, const alphabet a) noexcept { return alphabets[static_cast<std::size_t>(a)][c]; }
		};

		struct suffix_matcher
		{
			enum class suffix_type : std::uint8_t
			{
				default_type = 1 << 0,
				// unsigned integer
				unsigned_type = 1 << 1,
				// float, not double(default)
				float_type = 1 << 2,
				// long integer/floating point
				long_type = 1 << 3,
				// long long integer
				long_long_type = 1 << 4,
			};

			// see also alphabet_matcher::build
			[[nodiscard]] constexpr static suffix_type check(const foundation::string_view_type string)
			{
				auto ret{suffix_type::default_type};

				for (const auto c: string | std::views::reverse)
				{
					if (c == 'u' || c == 'U') { utils::set_enum_flag_set(ret, suffix_type::unsigned_type); }
					else if (c == 'f' || c == 'F') { utils::set_enum_flag_set(ret, suffix_type::float_type); }
					else if (c == 'l' || c == 'L')
					{
						if (utils::check_any_enum_flag(ret, suffix_type::long_type)) { utils::set_enum_flag_set(ret, suffix_type::long_long_type); }
						else { utils::set_enum_flag_set(ret, suffix_type::long_type); }
					}
					else { break; }
				}

				if (ret != suffix_type::default_type) { utils::unset_enum_flag_set(ret, suffix_type::default_type); }
				return ret;
			}
		};

		struct parse_point
		{
			using size_type = int;
			using difference_type = std::size_t;// std::pointer_traits<const char*>::difference_type;

			constexpr static size_type invalid_pos = -1;

			constexpr static char invalid_char{""[0]};
			constexpr static std::string_view invalid_string{"invalid_string"};

		private:
			const char* current_;
			const char* end_;
			size_type last_column_;

		public:
			lang::file_point point;

			constexpr parse_point(const char* current, const char* end) noexcept
				: current_{current},
				  end_{end},
				  last_column_{invalid_pos},
				  point{1, 1} {}

			constexpr parse_point() noexcept
				: current_{nullptr},
				  end_{nullptr},
				  last_column_{invalid_pos},
				  point{invalid_pos, invalid_pos} {}

			[[nodiscard]] constexpr std::string_view str(const parse_point& end) const noexcept
			{
				if (current_ and end_)
				{
					return std::string_view{
							current_,
							static_cast<std::string_view::size_type>(std::ranges::distance(current_, end.current_))};
				}
				return invalid_string;
			}

			constexpr parse_point& operator++() noexcept
			{
				if (current_ != end_)
				{
					if (utils::is_new_line(*current_))
					{
						++point.line;
						last_column_ = point.column;
						point.column = 1;
					}
					else { ++point.column; }

					++current_;
				}
				return *this;
			}

			constexpr parse_point& operator--() noexcept
			{
				--current_;
				if (utils::is_new_line(*current_))
				{
					--point.line;
					point.column = last_column_;
				}
				else { --point.column; }
				return *this;
			}

			constexpr parse_point operator+(const difference_type offset) const noexcept
			{
				// gal_assert(offset > 0);
				auto tmp = *this;
				for (difference_type i = 0; i < offset; ++i) { ++tmp; }
				return tmp;
			}

			constexpr parse_point& operator+=(const difference_type offset) noexcept
			{
				*this = *this + offset;
				return *this;
			}

			constexpr parse_point operator-(const difference_type offset) const noexcept
			{
				// gal_assert(offset > 0);
				auto tmp = *this;
				for (difference_type i = 0; i < offset; ++i) { --tmp; }
				return tmp;
			}

			constexpr parse_point& operator-=(const difference_type offset) noexcept
			{
				*this = *this - offset;
				return *this;
			}

			[[nodiscard]] constexpr bool operator==(const parse_point& other) const noexcept { return current_ == other.current_; }

			// ReSharper disable once CppNonExplicitConversionOperator
			[[nodiscard]] operator lang::file_point() const noexcept { return point; }

			[[nodiscard]] constexpr difference_type remaining() const noexcept { return static_cast<difference_type>(end_ - current_); }

			[[nodiscard]] constexpr bool finish() const noexcept { return remaining() == 0; }

			[[nodiscard]] constexpr const char* begin() const noexcept { return current_; }

			[[nodiscard]] constexpr const char* end() const noexcept { return end_; }

			[[nodiscard]] constexpr char operator[](const difference_type offset) const noexcept
			{
				if (remaining() < offset) { return invalid_char; }
				return current_[offset];
			}

			[[nodiscard]] constexpr char peek() const noexcept { return this->operator[](0); }

			[[nodiscard]] constexpr bool read_char(const char c) noexcept
			{
				if (const auto n = peek(); n != invalid_char && n == c)
				{
					this->operator++();
					return true;
				}
				return false;
			}
		};

		struct operator_matcher
		{
			using operator_name_type = foundation::string_view_type;
			using group_id_type = std::size_t;

			constexpr static std::array group_ids{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
			constexpr static auto group_size = std::tuple_size_v<decltype(group_ids)>;

			constexpr static std::array m0{
					operator_name_type{lang::operator_logical_or_name::value}};
			constexpr static std::array m1{
					operator_name_type{lang::operator_logical_and_name::value}};
			constexpr static std::array m2{
					operator_name_type{lang::operator_bitwise_or_name::value}};
			constexpr static std::array m3{
					operator_name_type{lang::operator_bitwise_xor_name::value}};
			constexpr static std::array m4{
					operator_name_type{lang::operator_bitwise_and_name::value}};
			constexpr static std::array m5{
					operator_name_type{lang::operator_equal_name::value},
					operator_name_type{lang::operator_not_equal_name::value}};
			constexpr static std::array m6{
					operator_name_type{lang::operator_less_than_name::value},
					operator_name_type{lang::operator_less_equal_name::value},
					operator_name_type{lang::operator_greater_than_name::value},
					operator_name_type{lang::operator_greater_equal_name::value}};
			constexpr static std::array m7{
					operator_name_type{lang::operator_bitwise_shift_left_name::value},
					operator_name_type{lang::operator_bitwise_shift_right_name::value}};
			// We share precedence here but then separate them later
			constexpr static std::array m8{
					operator_name_type{lang::operator_plus_name::value},
					operator_name_type{lang::operator_minus_name::value}};
			constexpr static std::array m9{
					operator_name_type{lang::operator_multiply_name::value},
					operator_name_type{lang::operator_divide_name::value},
					operator_name_type{lang::operator_remainder_name::value}};
			constexpr static std::array m10{
					operator_name_type{lang::operator_unary_not_name::value},
					operator_name_type{lang::operator_unary_plus_name::value},
					operator_name_type{lang::operator_unary_minus_name::value},
					operator_name_type{lang::operator_unary_bitwise_complement_name::value}};

			constexpr static std::array operators{
					lang::operation_precedence::logical_or,
					lang::operation_precedence::logical_and,
					lang::operation_precedence::bitwise_or,
					lang::operation_precedence::bitwise_xor,
					lang::operation_precedence::bitwise_and,
					lang::operation_precedence::equality,
					lang::operation_precedence::comparison,
					lang::operation_precedence::bitwise_shift,
					lang::operation_precedence::plus_minus,
					lang::operation_precedence::multiply_divide,
					lang::operation_precedence::unary
			};

			static_assert(std::tuple_size_v<decltype(operators)> == static_cast<std::size_t>(lang::operation_precedence::operation_size));
			static_assert(std::tuple_size_v<decltype(operators)> == group_size);

			template<typename Predicate>
			[[nodiscard]] constexpr static bool any_of(const group_id_type group_id, Predicate&& predicate) noexcept
			{
				auto matcher = [p = std::forward<Predicate>(predicate)](const auto& array) { return std::ranges::any_of(array, p); };

				// todo: better way
				switch (group_id)
				{
					case 0: { return matcher(m0); }
					case 1: { return matcher(m1); }
					case 2: { return matcher(m2); }
					case 3: { return matcher(m3); }
					case 4: { return matcher(m4); }
					case 5: { return matcher(m5); }
					case 6: { return matcher(m6); }
					case 7: { return matcher(m7); }
					case 8: { return matcher(m8); }
					case 9: { return matcher(m9); }
					case 10: { return matcher(m10); }
					default:
					{
						gal_assert(false, "unknown group id");
						return false;
					}
				}
			}

			[[nodiscard]] constexpr static bool match(const group_id_type group_id, const operator_name_type name) noexcept { return operator_matcher::any_of(group_id, [name](const auto& n) { return name == n; }); }

			[[nodiscard]] constexpr static bool match(const operator_name_type name) noexcept { return std::ranges::any_of(group_ids, [name](const auto id) { return operator_matcher::match(id, name); }); }
		};

		[[nodiscard]] inline foundation::boxed_value floating_point_packer(const std::string_view string)
		{
			const auto suffix = suffix_matcher::check(string);

			auto packer = [string]<typename T>
			{
				T result;
				std::from_chars(string.data(), string.data() + string.size(), result);
				return const_var(result);
			};

			if (utils::check_any_enum_flag(suffix, suffix_matcher::suffix_type::float_type))
			{
				// float, not double
				return packer.decltype(packer)::operator()<float>();
			}
			if (utils::check_any_enum_flag(suffix, suffix_matcher::suffix_type::long_type))
			{
				// long double
				return packer.decltype(packer)::operator()<long double>();
			}

			gal_assert(suffix == suffix_matcher::suffix_type::default_type);
			// default, double
			return packer.decltype(packer)::operator()<double>();
		}

		[[nodiscard]] inline foundation::boxed_value integral_packer(const std::string_view string, const int base = 10)
		{
			const auto suffix = suffix_matcher::check(string);

			auto packer = [string, base]<typename T>
			{
				T result;
				std::from_chars(string.data(), string.data() + string.size(), result, base);
				return const_var(result);
			};

			const bool is_unsigned = utils::check_any_enum_flag(suffix, suffix_matcher::suffix_type::unsigned_type);
			const bool is_long = utils::check_any_enum_flag(suffix, suffix_matcher::suffix_type::long_type);
			// ReSharper disable once CppTooWideScope
			const bool is_long_long = utils::check_any_enum_flag(suffix, suffix_matcher::suffix_type::long_long_type);

			if (is_long_long)
			{
				if (is_unsigned) { return packer.decltype(packer)::operator()<unsigned long long>(); }
				return packer.decltype(packer)::operator()<long long>();
			}
			if (is_long)
			{
				if (is_unsigned) { return packer.decltype(packer)::operator()<unsigned long>(); }
				return packer.decltype(packer)::operator()<long>();
			}
			if (is_unsigned) { return packer.decltype(packer)::operator()<unsigned int>(); }

			gal_assert(suffix == suffix_matcher::suffix_type::default_type);
			return packer.decltype(packer)::operator()<int>();
		}
	}// namespace parser_detail

	namespace lang
	{
		class parser final : public parser_detail::parser_base
		{
		public:
			using parse_depth_type = std::size_t;

		private:
			struct scoped_parser : utils::scoped_base<scoped_parser, std::reference_wrapper<parser>>
			{
				friend struct scoped_base<scoped_parser, std::reference_wrapper<parser>>;

				constexpr explicit scoped_parser(parser& p)
					: scoped_base{p} {}

			private:
				constexpr void do_construct() const
				{
					if (auto& depth = data().get().current_parse_depth_;
						depth >= data().get().max_parse_depth_)
					{
						throw exception::eval_error{
								std_format::format("Maximum parse depth '{}' exceeded", data().get().max_parse_depth_),
								data().get().filename_,
								data().get().point_};
					}
					else { ++depth; }
				}

				constexpr void do_destruct() const noexcept { --data().get().current_parse_depth_; }
			};

			parser_detail::parse_point point_;

			std::reference_wrapper<ast_visitor> visitor_;
			std::reference_wrapper<ast_optimizer> optimizer_;

			const parse_depth_type max_parse_depth_;

			parse_depth_type current_parse_depth_;

			foundation::string_view_type filename_;
			ast_node::children_type match_stack_;

			/**
			 * @throw exception::eval_error if is not a valid object name
			 */
			void check_object_name(const foundation::string_view_type name) const
			{
				if (not name_validator::is_valid_object_name(name))
				{
					throw exception::eval_error{
							std_format::format("Object name '{}' is an invalid name", name),
							filename_,
							point_};
				}
			}

		public:
			[[nodiscard]] ast_visitor& get_visitor() noexcept override { return visitor_; }

			[[nodiscard]] ast_optimizer& get_optimizer() const noexcept { return optimizer_; }

			/**
			 * @brief Prints the parsed ast_nodes as a tree
			 */
			void debug_print_to(foundation::string_type& dest, const ast_node& node, const foundation::string_view_type prepend) const override
			{
				std_format::format_to(
						std::back_inserter(dest),
						"{}: {} at ({}, {})\n",
						prepend,
						node.identifier(),
						node.location_begin().line,
						node.location_begin().column);

				std::ranges::for_each(
						node.view(),
						[this, &dest, prepend](const auto& child_node) { this->debug_print_to(dest, child_node, prepend); });
			}

			/**
			 * @brief Prints the parsed ast_nodes as a tree
			 */
			[[nodiscard]] std::string debug_print(const ast_node& node, const foundation::string_view_type prepend) const override
			{
				std::string ret{};
				debug_print_to(ret, node, prepend);
				return ret;
			}

		private:
			template<typename NodeType, typename... Args>
			[[nodiscard]] auto make_node(const foundation::string_view_type identifier, const file_point prev_point, Args&&... args) const { return lang::make_node<NodeType>(identifier, parse_location{filename_, {prev_point, point_}}, std::forward<Args>(args)...); }

			/**
			 * @brief Helper function that collects ast_nodes from a starting position to the top of the stack into a new AST node
			 */
			template<typename NodeType>
			void build_match(ast_node::children_type::size_type match_begin, const foundation::string_view_type text = "")
			{
				bool is_deep = false;

				auto location = [&is_deep, match_begin, this]() -> parse_location
				{
					// so we want to take everything to the right of this and make them children
					if (match_begin != match_stack_.size())
					{
						is_deep = true;
						return parse_location{filename_, {match_stack_[match_begin]->location_begin(), point_}};
					}

					// todo: fix the fact that a successful match that captured no ast_nodes doesn't have any real start position
					return parse_location{filename_, {point_, point_}};
				}();

				ast_node::children_type children{};

				if (is_deep)
				{
					const auto begin_pos = match_stack_.begin() + static_cast<ast_node::children_type::difference_type>(match_begin);

					children.assign(
							std::make_move_iterator(begin_pos),
							std::make_move_iterator(match_stack_.end()));
					match_stack_.erase(begin_pos, match_stack_.end());
				}

				match_stack_.push_back(optimizer_.get().optimize(lang::make_node<NodeType>(text, std::move(location), std::move(children))));
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/////////////////                                    PARSER                                                  ///////////////////
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/**
			 * @brief Reads a char from input if it matches the parameter, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_char(const char c) noexcept { return point_.read_char(c); }

			/**
			 * @brief Reads a symbol group from input if it matches the parameter, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_symbol(const foundation::string_view_type symbol) noexcept
			{
				if (point_.remaining() >= symbol.length())
				{
					if (std::ranges::mismatch(symbol, point_).in1 != symbol.end()) { return false; }

					point_ += static_cast<parser_detail::parse_point::difference_type>(symbol.length());
					return true;
				}
				return false;
			}

			template<typename T>
			[[nodiscard]] bool read_any() noexcept
			{
				if constexpr (T::size_no_0 == 1) { return this->read_char(T::value[0]); }
				else { return this->read_symbol(T::value); }
			}

			/**
			 * @brief Reads an end-of-line group from input, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_eol(const bool end_of_statement = false) noexcept
			{
				if (point_.finish()) { return false; }

				if (read_symbol("\r\n") || read_char('\n'))
				{
					point_.point.column = 1;
					return true;
				}

				// optional ';'
				if (not end_of_statement && read_char(';')) { return true; }

				return false;
			}

			/**
			 * @brief Skips any multi-line or single-line comment
			 */
			[[nodiscard]] bool skip_comment()
			{
				if (read_any<keyword_comment_multi_name::left_type>())
				{
					while (not point_.finish())
					{
						if (read_any<keyword_comment_multi_name::right_type>()) { break; }
						if (not read_eol()) { ++point_; }
					}
					return true;
				}

				if (read_any<keyword_comment_single_name>())
				{
					while (not point_.finish())
					{
						if (read_symbol("\r\n"))
						{
							point_ -= 2;
							break;
						}
						if (read_char('\n'))
						{
							point_ -= 1;
							break;
						}
						point_ += 1;
					}
					return true;
				}

				if (read_any<keyword_comment_annotation_name::left_type>())
				{
					while (not point_.finish())
					{
						if (read_any<keyword_comment_annotation_name::right_type>()) { break; }
						if (read_symbol("\r\n"))
						{
							point_ -= 2;
							break;
						}
						if (read_char('\n'))
						{
							point_ -= 1;
							break;
						}
						point_ += 1;
					}
					return true;
				}

				return false;
			}

			/**
			 * @brief Skips whitespace, which means space and tab, but not cr/lf
			 *
			 * @throw exception::eval_error Illegal character read
			 */
			bool skip_whitespace(const bool skip_cr_lf = false)
			{
				bool result = false;

				while (not point_.finish())
				{
					if (const auto c = point_[0];
						c > 0x7e) { throw exception::eval_error{std_format::format("Illegal character '{}'", c), filename_, point_}; }
					else
					{
						if (const auto is_eol = (c != parser_detail::parse_point::invalid_char) && (c == '\n' || (c == '\r' && point_[1] == '\n'));
							parser_detail::alphabet_matcher::belong(c, parser_detail::alphabet::whitespace) || (skip_cr_lf && is_eol))
						{
							if (is_eol && c == '\r')
							{
								// discards lf
								++point_;
							}
							++point_;

							result = true;
						}
						else if (skip_comment()) { result = true; }
						else { break; }
					}
				}

				return result;
			}

			/**
			 * @brief Reads the optional exponent (scientific notation) and suffix for a Float
			 */
			[[nodiscard]] bool read_exponent_and_suffix() noexcept
			{
				if (point_.finish()) { return true; }

				// Support a form of scientific notation: 42e-42, 3.14E+9, 0.01e42
				if (const auto c = point_.peek();
					std::tolower(c) == 'e')
				{
					++point_;

					if (not point_.finish()) { if (const auto nc = point_.peek(); nc == '-' || nc == '+') { ++point_; } }

					const auto exponent_point = point_;
					while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer)) { ++point_; }
					if (point_ == exponent_point)
					{
						// Require at least one digit after the exponent
						return false;
					}
				}

				// Parse optional float suffix
				while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::floating_point_suffix)) { ++point_; }

				return true;
			}

			/**
			 * @brief Reads a floating point value from input, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_floating_point() noexcept
			{
				if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::floating_point))
				{
					while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer)) { ++point_; }

					if (not point_.finish())
					{
						if (const auto c = point_.peek();
							std::tolower(c) == 'e')
						{
							// The exponent is valid even without any decimal in the Float (1e10, 2e-15)
							return read_exponent_and_suffix();
						}
						else if (c == '.')
						{
							++point_;
							if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer))
							{
								++point_;
								while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer)) { ++point_; }

								// After any decimal digits, support an optional exponent (3.14e42)
								return read_exponent_and_suffix();
							}
							--point_;
						}
					}
				}

				return false;
			}

			/**
			 * @brief Reads a hex value from input, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_hex() noexcept
			{
				if (not point_.finish() && point_.peek() == '0')
				{
					++point_;
					if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::hex_prefix))
					{
						++point_;
						if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::hex))
						{
							++point_;
							while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::hex)) { ++point_; }
							while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer_suffix)) { ++point_; }
							return true;
						}
						--point_;
					}
					--point_;
				}

				return false;
			}

			/**
			 * @brief Reads a binary value from input, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_binary() noexcept
			{
				if (not point_.finish() && point_.peek() == '0')
				{
					++point_;
					if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::bin_prefix))
					{
						++point_;
						if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::bin))
						{
							++point_;
							while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::bin)) { ++point_; }
							return true;
						}
						--point_;
					}
					--point_;
				}

				return false;
			}

			/**
			 * @brief Reads an integer suffix
			 */
			void read_integer_suffix() noexcept { while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::integer_suffix)) { ++point_; } }

			/**
			 * @brief Reads an identifier from input which conforms to identifier naming conventions, without skipping initial whitespace
			 *
			 * @throw exception::eval_error Carriage return in identifier literal
			 * @throw exception::eval_error Missing contents of identifier literal
			 * @throw exception::eval_error Incomplete identifier literal
			 */
			[[nodiscard]] bool read_identifier()
			{
				if (point_.finish()) { return false; }

				if (parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::identifier))
				{
					++point_;
					while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::keyword)) { ++point_; }
					return true;
				}

				return false;
			}

			/**
			 * @brief Reads an operator from input which conforms to identifier naming conventions, without skipping initial whitespace
			 *
			 * @note Called only after read_identifier has read the operator keyword.
			 *  
			 * @throw exception::eval_error Carriage return in identifier literal
			 * @throw exception::eval_error Missing contents of identifier literal
			 * @throw exception::eval_error Incomplete identifier literal
			 */
			[[nodiscard]] bool read_operator()
			{
				if (point_.finish()) { return false; }

				const auto begin = point_;
				// todo: check is valid operator?
				while (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::symbol))
				{
					if (read_eol())
					{
						throw exception::eval_error{
								"Carriage return in identifier literal",
								filename_,
								point_};
					}
					++point_;
				}

				if (begin == point_)
				{
					throw exception::eval_error{
							"Missing contents of identifier literal",
							filename_,
							point_};
				}
				if (point_.finish())
				{
					throw exception::eval_error{
							"Incomplete identifier literal",
							filename_,
							point_};
				}
				++point_;
				return true;
			}

			// todo: better char parser
			template<typename StringType>
			struct char_parser
			{
				using string_type = StringType;
				using value_type = typename string_type::value_type;
				using size_type = typename string_type::size_type;

				string_type& match;
				const bool interpolation_allowed;

				bool is_escaped = false;
				bool is_interpolated = false;
				bool saw_interpolation_marker = false;
				bool is_octal = false;
				bool is_hex = false;
				size_type unicode_size = 0;
				string_type octal_matches;
				string_type hex_matches;

				void process_hex() noexcept
				{
					if (not hex_matches.empty())
					{
						value_type value;
						std::from_chars(hex_matches.data(), hex_matches.data() + hex_matches.size(), value, 16);
						match.push_back(value);
					}

					hex_matches.clear();
					is_escaped = false;
					is_hex = false;
				}

				void process_octal()
				{
					if (not octal_matches.empty())
					{
						value_type value;
						std::from_chars(hex_matches.data(), hex_matches.data() + hex_matches.size(), value, 8);
						match.push_back(value);
					}

					octal_matches.clear();
					is_escaped = false;
					is_octal = false;
				}

				void process_unicode()
				{
					std::uint32_t codepoint;
					std::from_chars(hex_matches.data(), hex_matches.data() + hex_matches.size(), codepoint, 16);
					const auto match_size = hex_matches.size();
					hex_matches.clear();
					is_escaped = false;

					const auto u_size = std::exchange(unicode_size, 0);
					if (u_size != match_size) { throw exception::eval_error{"Incomplete unicode escape sequence"}; }
					if (u_size == 4 && codepoint > 0xD800 && codepoint <= 0xDFFF) { throw exception::eval_error{"Invalid 16 bit universal character"}; }

					char buff[4];
					if (const auto size = utils::to_utf8(buff, codepoint);
						size == 0)
					{
						// this must be an invalid escape sequence?
						throw exception::eval_error{"Invalid 32 bit universal character"};
					}
					else { match.append(buff, size); }
				}

				char_parser(string_type& m, const bool i)
					: match{m},
					  interpolation_allowed{i} {}

				char_parser(const char_parser&) = delete;
				char_parser& operator=(const char_parser&) = delete;
				char_parser(char_parser&&) = delete;
				char_parser& operator=(char_parser&&) = delete;

				~char_parser() noexcept
				{
					try
					{
						if (is_octal) { process_octal(); }
						if (is_hex) { process_hex(); }
						if (unicode_size > 0) { process_unicode(); }
					}
					catch (const exception::eval_error&)
					{
						// Something happened with parsing, we'll catch it later?
					}
				}


				void parse(const value_type c, const file_point point, const parse_location::filename_type& filename)
				{
					const auto is_octal_char = utils::is_oct_digit(c);
					const auto is_hex_char = utils::is_hex_digit(c);

					if (is_octal)
					{
						if (is_octal_char)
						{
							octal_matches.push_back(c);
							if (octal_matches.size() == 3) { process_octal(); }
							return;
						}
						process_octal();
					}
					else if (is_hex)
					{
						if (is_hex_char)
						{
							hex_matches.push_back(c);
							if (hex_matches.size() == 2 * sizeof(value_type)) { process_hex(); }
							return;
						}
						process_hex();
					}
					else if (unicode_size > 0)
					{
						if (is_hex_char)
						{
							hex_matches.push_back(c);
							if (hex_matches.size() == unicode_size)
							{
								// Format is specified to be /uABC on collecting from A to C do parsing
								process_unicode();
							}
							return;
						}
						// Not a unicode anymore, try parsing any way
						// May be someone used \uAA only
						process_unicode();
					}

					if (c == '\\')
					{
						if (is_escaped)
						{
							match.push_back(c);
							is_escaped = false;
						}
						else { is_escaped = true; }
					}
					else
					{
						if (is_escaped)
						{
							if (is_octal_char)
							{
								is_octal = true;
								octal_matches.push_back(c);
							}
							else if (c == 'x') { is_hex = true; }
							else if (c == 'u') { unicode_size = 4; }
							else if (c == 'U') { unicode_size = 8; }
							else
							{
								switch (c)
								{
									case '\'':
									case '\"':
									case '?':
									case '$':
									{
										match.push_back(c);
										break;
									}
									case 'a':
									case 'b':
									case 'f':
									case 'n':
									case 'r':
									case 't':
									case 'v':
									{
										match.push_back(utils::take_escape(c));
										break;
									}
									default:
									{
										throw exception::eval_error{
												"Unknown escaped sequence in string",
												filename,
												point};
									}
								}

								is_escaped = false;
							}
						}
						else if (interpolation_allowed && c == '$') { saw_interpolation_marker = true; }
						else { match.push_back(c); }
					}
				}
			};

			/**
			 * @brief Reads a quoted string from input, without skipping initial whitespace
			 *
			 * @throw exception::eval_error Unclosed quoted string
			 */
			[[nodiscard]] bool read_quoted_string()
			{
				// todo: string format?
				if (point_.finish() || point_.peek() != '\"') { return false; }

				auto prev_char = '\"';
				++point_;

				int in_interpolation = 0;
				bool in_quote = false;
				while (not point_.finish() && (point_.peek() != '\"' || in_interpolation > 0 || prev_char == '\\'))
				{
					if (not read_eol())
					{
						const auto current_char = point_.peek();
						if (prev_char == '$' && current_char == '{') { ++in_interpolation; }
						else if (prev_char != '\\' && current_char == '\"') { in_quote = !in_quote; }
						else if (current_char == '}' && not in_quote) { --in_interpolation; }

						if (prev_char == '\\') { prev_char = '\0'; }
						else { prev_char = current_char; }
						++point_;
					}
				}

				if (point_.finish())
				{
					throw exception::eval_error{
							"Unclosed quoted string",
							filename_,
							point_};
				}
				++point_;
				return true;
			}

			/**
			 * @brief Reads a character group from input, without skipping initial whitespace
			 *
			 * @throw exception::eval_error Unclosed single-quoted string
			 */
			[[nodiscard]] bool read_single_quoted_string()
			{
				if (point_.finish() || point_.peek() != '\'') { return false; }

				auto prev_char = '\'';
				++point_;

				while (not point_.finish() && (point_.peek() != '\'' || prev_char == '\\'))
				{
					if (not read_eol())
					{
						if (prev_char == '\\') { prev_char = '\0'; }
						else { prev_char = point_.peek(); }
						++point_;
					}
				}

				if (point_.finish()) { throw exception::eval_error{"Unclosed single-quoted string", filename_, point_}; }
				++point_;
				return true;
			}

			[[nodiscard]] bool read_operator(const parser_detail::operator_matcher::group_id_type group_id, parser_detail::operator_matcher::operator_name_type& name) noexcept
			{
				return parser_detail::operator_matcher::any_of(
						group_id,
						[&name, this](const parser_detail::operator_matcher::operator_name_type& element)
						{
							if (build_symbol(element))
							{
								name = element;
								return true;
							}
							return false;
						});
			}

			/**
			 * @brief Reads a char or a symbol from input depend on Name
			 *
			 * @throw exception::eval_error throw from read_char(' ') || read_symbol(" ")
			 */
			template<typename Name>
			[[nodiscard]] bool build_any()
			{
				if constexpr (Name::size_no_0 == 1) { return this->build_char(Name::value[0]); }
				else { return this->build_symbol(Name::value); }
			}

		public:
			/**
			 * @brief Reads (and potentially captures) a char from input if it matches the parameter
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_char(const char c) noexcept
			{
				scoped_parser p{*this};
				skip_whitespace();
				return read_char(c);
			}

			/**
			 * @brief Reads until the end of the current statement
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_eos()
			{
				scoped_parser p{*this};
				skip_whitespace();

				return read_eol(true);
			}

			/**
			 * @brief Reads (and potentially captures) an end-of-line group from input
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_eol()
			{
				scoped_parser p{*this};
				skip_whitespace();

				return read_eol(false);
			}

			/**
			 * @brief Reads a number from the input, detecting if it's an integer or floating point
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_number()
			{
				skip_whitespace();

				const auto begin = point_;
				if (not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::floating_point))
				{
					if (read_hex())
					{
						const auto match = begin.str(point_);
						match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, parser_detail::integral_packer(match, 16)));
						return true;
					}

					if (read_binary())
					{
						const auto match = begin.str(point_);
						match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, parser_detail::integral_packer(match, 2)));
						return true;
					}

					if (read_floating_point())
					{
						const auto match = begin.str(point_);
						match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, parser_detail::floating_point_packer(match)));
						return true;
					}

					read_integer_suffix();

					if (const auto match = begin.str(point_);
						match.empty()) { return false; }
					else if (match[0] == '0')
					{
						// Octal
						match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, parser_detail::integral_packer(match, 8)));
					}
					else
					{
						// Decimal
						match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, parser_detail::integral_packer(match, 10)));
					}

					return true;
				}

				return false;
			}

			/**
			 * @brief Reads (and potentially captures) an identifier from input
			 *
			 * @throw exception::eval_error throw from read_identifier()
			 * @throw exception::eval_error throw from check_object_name(name)
			 */
			[[nodiscard]] bool build_identifier(const bool need_validate_name)
			{
				skip_whitespace();

				const auto begin = point_;

				if (not read_identifier()) { return false; }

				auto text = begin.str(point_);

				if (need_validate_name) { check_object_name(text); }

				switch (
					const auto text_hash = name_validator::hash_name(text);
					text_hash)
				{
					case name_validator::hash_name(keyword_true_name::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(true)));
						break;
					}
					case name_validator::hash_name(keyword_false_name::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(false)));
						break;
					}
					case name_validator::hash_name(keyword_operator_declare_name::value):
					{
						// since our operators are not keyword/identifiers, read_identifier will stop when they are read, so we can safely read them again.
						const auto operator_begin = point_;
						[[maybe_unused]] const auto result = read_operator();
						gal_assert(result);
						text = operator_begin.str(point_);
						match_stack_.emplace_back(this->make_node<id_ast_node>(text, begin));
						break;
					}
					case name_validator::hash_name(keyword_number_inf_nan_name::subtype<0>::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(std::numeric_limits<double>::infinity())));
						break;
					}
					case name_validator::hash_name(keyword_function_argument_placeholder_name::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(std::make_shared<foundation::function_argument_placeholder>())));
						break;
					}
					case name_validator::hash_name(keyword_number_inf_nan_name::subtype<1>::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(std::numeric_limits<double>::quiet_NaN())));
						break;
					}
					case name_validator::hash_name(keyword_magic_line_name::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(begin.point.line)));
						break;
					}
					case name_validator::hash_name(keyword_magic_file_name::value):
					{
						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(filename_)));
						break;
					}
					case name_validator::hash_name(keyword_magic_function_name::value):
					{
						for (bool find_arg = false; const auto& node: match_stack_ | std::views::reverse)
						{
							if (node->is<arg_list_ast_node>() && not find_arg) { find_arg = true; }
							else if (node->is<id_ast_node>() && find_arg)
							{
								match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(node->identifier())));
								return true;
							}
							else { find_arg = false; }
						}

						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(decltype(std::declval<ast_node::children_type::value_type>()->identifier()){keyword_function_not_found_name::value})));
						break;
					}
					case name_validator::hash_name(keyword_magic_class_name::value):
					{
						for (bool find_arg = false, find_id = false; const auto& node: match_stack_ | std::views::reverse)
						{
							if (node->is<arg_list_ast_node>() && not find_arg) { find_arg = true; }
							else if (node->is<id_ast_node>() && find_arg)
							{
								if (find_id)
								{
									match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(node->identifier())));
									return true;
								}
								find_id = true;
							}
							else
							{
								find_arg = false;
								find_id = false;
							}
						}

						match_stack_.emplace_back(this->make_node<constant_ast_node>(text, begin, const_var(decltype(std::declval<ast_node::children_type::value_type>()->identifier()){keyword_class_not_found_name::value})));
						break;
					}
					// todo: other internal magic name?
					default:
					{
						match_stack_.emplace_back(this->make_node<id_ast_node>(text, begin));
						break;
					}
				}

				return true;
			}

			/**
			 * @brief Reads an argument from input
			 *
			 * @throw exception::eval_error throw from read_identifier(true)
			 */
			[[nodiscard]] bool build_argument(const bool allow_set_type = true)
			{
				const auto prev_size = match_stack_.size();
				skip_whitespace();

				if (not build_identifier(true)) { return false; }

				skip_whitespace();

				// todo: split?
				if (allow_set_type) { (void)build_identifier(true); }

				build_match<arg_ast_node>(prev_size);

				return true;
			}

		private:
			template<typename Function>
			[[nodiscard]] bool do_build_argument_list(Function&& function)
			{
				scoped_parser p{*this};
				skip_whitespace(true);

				const auto prev_size = match_stack_.size();
				const auto result = [this, f = std::forward<Function>(function)]
				{
					if (f())
					{
						while (build_eol()) {}

						while (build_char(','))
						{
							while (build_eol()) {}
							if (not f())
							{
								throw exception::eval_error{
										"Unexpected value in parameter list",
										filename_,
										point_};
							}
						}
						return true;
					}
					return false;
				}();

				build_match<arg_list_ast_node>(prev_size);
				skip_whitespace(true);
				return result;
			}

		public:
			/**
			 * @brief Reads a comma-separated list of values from input. Id's only, no types allowed
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 * @throw exception::eval_error Unexpected value in parameter list
			 */
			[[nodiscard]] bool build_identifier_argument_list()
			{
				return do_build_argument_list(
						[this] { return this->build_argument(false); });
			}

			/**
			 * @brief Reads a comma-separated list of values from input, for function declarations
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 * @throw exception::eval_error Unexpected value in parameter list
			 */
			[[nodiscard]] bool build_decl_argument_list()
			{
				return do_build_argument_list(
						[this] { return this->build_argument(true); });
			}

			/**
			 * @brief Reads a comma-separated list of values from input
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 * @throw exception::eval_error Unexpected value in parameter list
			 */
			[[nodiscard]] bool build_argument_list()
			{
				return do_build_argument_list(
						[this] { return this->build_equation(); });
			}

			/**
			 * @brief Reads possible special container values, including ranges and map_pairs
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ") || skip_whitespace(true) || build_eol()
			 * @throw exception::eval_error throw from build_value_range() || build_map_pair() || build_operator()
			 * @throw exception::eval_error Unexpected comma(,) or value in container
			 */
			[[nodiscard]] bool build_container_argument_list()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				skip_whitespace(true);

				if (build_map_pair())
				{
					while (build_eol()) {}

					while (build_char(','))
					{
						while (build_eol()) {}

						if (not build_map_pair())
						{
							throw exception::eval_error{
									"Unexpected comma(,) or value in container",
									filename_,
									point_};
						}
					}

					build_match<arg_list_ast_node>(prev_size);
					skip_whitespace(true);
					return true;
				}

				if (build_operator())
				{
					while (build_eol()) {}

					while (build_char(','))
					{
						while (build_eol()) {}

						if (not build_operator())
						{
							throw exception::eval_error{
									"Unexpected comma(,) or value in container",
									filename_,
									point_};
						}
					}

					build_match<arg_list_ast_node>(prev_size);
					skip_whitespace(true);
					return true;
				}

				skip_whitespace(true);
				return false;
			}


			/**
			 * @brief Reads a lambda (anonymous function) from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_identifier_argument_list()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete anonymous function bind
			 * @throw exception::eval_error Incomplete anonymous function
			 */
			[[nodiscard]] bool build_lambda()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				// '['
				if (not build_any<keyword_lambda_name::left_type>()) { return false; }

				// capture argument list is optional
				(void)build_identifier_argument_list();

				// ']'
				if (not build_any<keyword_lambda_name::right_type>())
				{
					throw exception::eval_error{
							"Incomplete anonymous function, missing ']'",
							filename_,
							point_};
				}

				// argument list is optional
				if (build_char('('))
				{
					(void)build_decl_argument_list();

					if (not build_char(')'))
					{
						throw exception::eval_error{
								"Incomplete anonymous function, missing ')'",
								filename_,
								point_};
					}
				}

				while (build_eol()) {}

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete anonymous function, missing block",
							filename_,
							point_};
				}

				build_match<lambda_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads a function definition from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eos() || build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_identifier(true)
			 * @throw exception::eval_error throw from build_decl_argument_list()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Missing function name in definition
			 * @throw exception::eval_error Missing method name in definition
			 * @throw exception::eval_error Incomplete function definition
			 * @throw exception::eval_error Missing guard expression for function
			 */
			[[nodiscard]] bool build_def(const bool class_context = false, const foundation::string_view_type class_name = "")
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_define_name::value)) { return false; }

				if (class_context) { match_stack_.emplace_back(this->make_node<id_ast_node>(class_name, point_)); }

				if (not build_identifier(true))
				{
					throw exception::eval_error{
							"Missing function name in definition",
							filename_,
							point_};
				}

				// todo: Do we really need to support something like 'def class_name::function_name(argument_list): block' ?
				const auto is_member_method = [this]
				{
					if (build_any<keyword_class_accessor_name>())
					{
						// We're now a method
						if (not build_identifier(true))
						{
							throw exception::eval_error{
									"Missing method name in definition",
									filename_,
									point_};
						}
						return true;
					}
					return false;
				}();

				if (build_char('('))
				{
					(void)build_decl_argument_list();

					if (not build_char(')'))
					{
						throw exception::eval_error{
								"Incomplete function definition, missing ')'",
								filename_,
								point_};
					}
				}

				while (build_eos()) {}

				if (build_any<keyword_function_guard_name>())
				{
					if (not build_operator())
					{
						throw exception::eval_error{
								"Missing guard expression for function",
								filename_,
								point_};
					}
				}

				while (build_eol()) {}

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete function definition, missing block",
							filename_,
							point_};
				}

				if (is_member_method || class_context) { build_match<method_ast_node>(prev_size); }
				else { build_match<def_ast_node>(prev_size); }

				return true;
			}

			/**
			 * @brief Reads an if/else if/else block from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_equation()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete 'if' expression
			 * @throw exception::eval_error Incomplete 'if' expression, missing ':'
			 * @throw exception::eval_error Incomplete 'if' expression, missing block
			 * Incomplete 'else' expression, missing block
			 */
			[[nodiscard]] bool build_if()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_if_name::value)) { return false; }

				if (not build_equation())
				{
					throw exception::eval_error{
							"Incomplete 'if' expression",
							filename_,
							point_};
				}

				const auto is_init_if = build_eol() && build_equation();

				if (not build_any<keyword_block_begin_name>())
				{
					throw exception::eval_error{
							"Incomplete 'if' expression, missing ':'",
							filename_,
							point_};
				}

				while (build_eol()) {}

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete 'if' expression, missing block",
							filename_,
							point_};
				}

				while (true)
				{
					while (build_eol()) {}

					// no more else
					if (not build_keyword(keyword_else_name::value)) { break; }

					// else if
					if (build_if()) { continue; }

					while (build_eol()) {}

					// just else
					if (not build_block())
					{
						throw exception::eval_error{
								"Incomplete 'else' expression, missing block",
								filename_,
								point_};
					}
				}

				if (const auto children_size = match_stack_.size() - prev_size;
					(is_init_if && children_size == 3) || (not is_init_if && children_size == 2)) { match_stack_.emplace_back(lang::make_node<noop_ast_node>()); }

				if (not is_init_if) { build_match<if_ast_node>(prev_size); }
				else
				{
					build_match<if_ast_node>(prev_size + 1);
					build_match<block_ast_node>(prev_size);
				}

				return true;
			}

			/**
			 * @brief Reads a while block from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_equation()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete 'while' expression
			 * @throw exception::eval_error Incomplete 'if' expression, missing ':'
			 * @throw exception::eval_error Incomplete 'if' expression, missing block
			 */
			[[nodiscard]] bool build_while()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_while_name::value)) { return false; }

				if (not build_equation())
				{
					throw exception::eval_error{
							"Incomplete 'while' expression",
							filename_,
							point_};
				}

				if (not build_any<keyword_block_begin_name>())
				{
					throw exception::eval_error{
							"Incomplete 'while' expression, missing ':'",
							filename_,
							point_};
				}

				while (build_eol()) {}

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete 'while' expression, missing block",
							filename_,
							point_};
				}

				build_match<while_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads a for block from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_equation()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete 'for' expression
			 * @throw exception::eval_error Incomplete 'ranged-for' expression
			 * @throw exception::eval_error Incomplete 'for' expression, missing block
			 */
			[[nodiscard]] bool build_for()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_for_in_name::subtype<0>::value)) { return false; }

				if (not build_equation())
				{
					throw exception::eval_error{
							"Incomplete 'ranged-for' expression, missing iteration value",
							filename_,
							point_};
				}

				if (not build_keyword(keyword_for_in_name::subtype<1>::value))
				{
					throw exception::eval_error{
							"Incomplete 'ranged-for' expression, missing 'in'",
							filename_,
							point_};
				}

				if (not build_equation())
				{
					throw exception::eval_error{
							"Incomplete 'ranged-for' expression, missing iteration range",
							filename_,
							point_};
				}

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete 'for' expression, missing block",
							filename_,
							point_};
				}

				if (match_stack_.size() - prev_size != 3)
				{
					throw exception::eval_error{
							"Incomplete 'ranged-for' expression",
							filename_,
							point_};
				}
				build_match<ranged_for_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads a match statement from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_equation()
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete match' expression
			 * @throw exception::eval_error Incomplete match' expression, missing ':'
			 * @throw exception::eval_error Incomplete match-case' expression
			 * @throw exception::eval_error Incomplete match-case' expression, missing ':'
			 * @throw exception::eval_error Incomplete match-case' expression, missing block
			 * @throw exception::eval_error Incomplete match-default' expression, missing ':'
			 * @throw exception::eval_error Incomplete match-default' expression, missing block
			 */
			[[nodiscard]] bool build_match()
			{
				/**
				 * @brief Reads a case block from input
				 */
				auto build_case = [this]
				{
					scoped_parser c_p{*this};
					const auto c_prev_size = match_stack_.size();

					if (build_any<keyword_match_case_default_name::subtype<1>>())
					{
						if (not build_operator())
						{
							throw exception::eval_error{
									"Incomplete 'match-case' expression",
									filename_,
									point_};
						}

						if (not build_block())
						{
							throw exception::eval_error{
									"Incomplete 'match-case' expression, missing block",
									filename_,
									point_};
						}

						build_match<match_case_ast_node>(c_prev_size);
						return true;
					}

					if (build_any<keyword_match_case_default_name::subtype<2>>())
					{
						if (not build_block())
						{
							throw exception::eval_error{
									"Incomplete 'match-default' expression, missing block",
									filename_,
									point_};
						}

						build_match<match_default_ast_node>(c_prev_size);
						return true;
					}

					return false;
				};

				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_any<keyword_match_case_default_name::subtype<0>>()) { return false; }

				if (not build_operator())
				{
					throw exception::eval_error{
							"Incomplete 'match' expression",
							filename_,
							point_};
				}

				// inline build_block, but this block is composed of case+default.
				if (not build_any<keyword_block_begin_name>())
				{
					throw exception::eval_error{
							"Incomplete 'match' expression, missing ':'",
							filename_,
							point_};
				}

				while (build_eol()) {}

				while (build_case())
				{
					// just eat it
					while (build_eol()) {}
				}

				build_match<match_ast_node>(prev_size);
				return true;
			}

		private:
			template<typename NodeType, typename Keyword>
			[[nodiscard]] bool do_build_keyword_statement()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (this->build_keyword(Keyword::value))
				{
					(void)build_operator();
					this->build_match<NodeType>(prev_size);
					return true;
				}
				return false;
			}

		public:
			/**
			 * @brief Reads a continue statement from input
			 *
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_operator()
			 */
			[[nodiscard]] bool build_continue() { return do_build_keyword_statement<continue_ast_node, keyword_continue_break_return_name::subtype<0>>(); }

			/**
			 * @brief Reads a break statement from input
			 *
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_operator()
			 */
			[[nodiscard]] bool build_break() { return do_build_keyword_statement<break_ast_node, keyword_continue_break_return_name::subtype<1>>(); }

			/**
			 * @brief Reads a return statement from input
			 *
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_operator()
			 */
			[[nodiscard]] bool build_return() { return do_build_keyword_statement<return_ast_node, keyword_continue_break_return_name::subtype<2>>(); }

			/**
			 * @brief Reads a function definition from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_block()
			 * @throw exception::eval_error Incomplete 'try' block, missing ':'
			 * @throw exception::eval_error Incomplete 'try' block, missing block
			 * @throw exception::eval_error Incomplete 'try-catch' expression
			 * @throw exception::eval_error Incomplete 'try-catch' expression, missing ':'
			 * @throw exception::eval_error Incomplete 'try-catch' expression, missing block
			 * @throw exception::eval_error Incomplete 'try-finally' expression, missing ':'
			 * @throw exception::eval_error Incomplete 'try-finally' expression, missing block
			 */
			[[nodiscard]] bool build_try()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_try_catch_finally_name::subtype<0>::value)) { return false; }

				if (not build_block())
				{
					throw exception::eval_error{
							"Incomplete 'try' block, missing block",
							filename_,
							point_};
				}

				while (true)
				{
					while (build_eol()) {}

					if (not build_keyword(keyword_try_catch_finally_name::subtype<1>::value)) { break; }

					const auto catch_prev_size = match_stack_.size();

					if (not build_argument())
					{
						throw exception::eval_error{
								"Incomplete 'try-catch' expression",
								filename_,
								point_};
					}

					if (not build_block())
					{
						throw exception::eval_error{
								"Incomplete 'try-catch' expression, missing block",
								filename_,
								point_};
					}

					build_match<try_catch_ast_node>(catch_prev_size);
				}

				while (build_eol()) {}

				if (build_keyword(keyword_try_catch_finally_name::subtype<2>::value))
				{
					const auto finally_prev_size = match_stack_.size();

					if (not build_block())
					{
						throw exception::eval_error{
								"Incomplete 'try-finally' expression, missing block",
								filename_,
								point_};
					}

					build_match<try_finally_ast_node>(finally_prev_size);
				}

				build_match<try_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads a class block from input
			 *
			 * @throw exception::eval_error throw from build_keyword(" ")
			 * @throw exception::eval_error throw from build_identifier(true)
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error throw from build_class_block(" ")
			 * @throw exception::eval_error Class definitions only allowed at top scope
			 * @throw exception::eval_error Missing class name in definition
			 * @throw exception::eval_error Incomplete 'class' block
			 */
			[[nodiscard]] bool build_class(const bool class_allowed)
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_keyword(keyword_class_name::value)) { return false; }

				if (not class_allowed)
				{
					throw exception::eval_error{
							"Class definitions only allowed at top scope",
							filename_,
							point_};
				}

				if (not build_identifier(true))
				{
					throw exception::eval_error{
							"Missing class name in definition",
							filename_,
							point_};
				}

				const auto& class_name = match_stack_.back()->identifier();

				while (build_eol()) {}

				if (not build_class_block(class_name))
				{
					throw exception::eval_error{
							"Incomplete 'class' block",
							filename_,
							point_};
				}

				build_match<class_decl_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads (and potentially captures) a quoted string from input.
			 * Translates escaped sequences.
			 *
			 * @todo Need to be optimized
			 * @todo format?
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 * @throw exception::eval_error Unclosed in-string eval
			 */
			[[nodiscard]] bool build_quoted_string()
			{
				scoped_parser sp{*this};
				skip_whitespace();

				const auto begin = point_;
				if (read_quoted_string())
				{
					foundation::string_type match{};
					const auto prev_size = match_stack_.size();
					const auto is_interpolated = [&match, &begin, prev_size, this]
					{
						char_parser p{match, true};

						for (auto b = begin + 1, e = point_ - 1; b != e;)
						{
							if (p.saw_interpolation_marker)
							{
								if (b.peek() == '{')
								{
									// We've found an interpolation point
									match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, const_var(match)));
									if (p.is_interpolated)
									{
										// If we've seen previous interpolation, add on instead of making a new one
										build_match<binary_operator_ast_node>(prev_size, operator_plus_name::value);
									}

									// We've finished with the part of the string up to this point, so clear it
									match.clear();

									const auto eval_begin = b;
									++b;
									while (b != e && b.peek() != '}') { ++b; }
									const auto eval_string = eval_begin.str(b);

									if (b.peek() == '}')
									{
										p.is_interpolated = true;
										++b;

										const auto to_string_size = match_stack_.size();
										match_stack_.emplace_back(this->make_node<id_ast_node>(operator_to_string_name::value, begin));

										const auto eval_size = match_stack_.size();
										try { match_stack_.emplace_back(parse_instruct_eval(eval_string)); }
										catch (const exception::eval_error& ex)
										{
											throw exception::eval_error{
													ex.what(),
													filename_,
													begin};
										}

										build_match<arg_list_ast_node>(eval_size);
										build_match<fun_call_ast_node>(to_string_size);
										build_match<binary_operator_ast_node>(prev_size, operator_plus_name::value);
									}
									else
									{
										throw exception::eval_error{
												"Unclosed in-string eval",
												filename_,
												begin};
									}
								}
								else { match.push_back('$'); }
								p.saw_interpolation_marker = false;
							}
							else
							{
								p.parse(b.peek(), begin, filename_);
								++b;
							}
						}

						if (p.saw_interpolation_marker) { match.push_back('$'); }

						return p.is_interpolated;
					}();

					match_stack_.push_back(this->make_node<constant_ast_node>(match, begin, const_var(match)));
					if (is_interpolated) { build_match<binary_operator_ast_node>(prev_size, operator_plus_name::value); }

					return true;
				}
				return false;
			}

			/**
			 * @brief Reads (and potentially captures) a char group from input.
			 * Translates escaped sequences.
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 * @throw exception::eval_error Single-quoted strings must be 1 character long
			 */
			[[nodiscard]] bool build_single_quoted_string()
			{
				scoped_parser sp{*this};
				skip_whitespace();

				const auto begin = point_;
				if (read_single_quoted_string())
				{
					std::string match{};
					{
						// scope for char_parser destructor
						char_parser p{match, false};

						for (auto b = begin + 1, e = point_ - 1; b != e; ++b) { p.parse(b.peek(), begin, filename_); }
					}

					if (match.size() != 1)
					{
						throw exception::eval_error{
								"Single-quoted strings must be 1 character long",
								filename_,
								point_};
					}

					match_stack_.emplace_back(this->make_node<constant_ast_node>(match, begin, const_var(match[0])));
					return true;
				}
				return false;
			}

			/**
			 * @brief Reads (and potentially captures) a string from input if it matches the parameter
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_keyword(const foundation::string_view_type symbol)
			{
				scoped_parser p{*this};
				skip_whitespace();

				const auto begin = point_;
				const auto result = read_symbol(symbol);
				// ignore substring matches
				if (result && not point_.finish() && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::keyword))
				{
					point_ = begin;
					return false;
				}
				return result;
			}

			/**
			 * @brief Reads (and potentially captures) a symbol group from input if it matches the parameter
			 *
			 * @throw exception::eval_error throw from skip_whitespace()
			 */
			[[nodiscard]] bool build_symbol(const foundation::string_view_type symbol, const bool disallow_prevention = false)
			{
				scoped_parser p{*this};
				skip_whitespace();

				const auto begin = point_;
				const auto result = read_symbol(symbol);
				// ignore substring matches
				if (result && not point_.finish() && not disallow_prevention && parser_detail::alphabet_matcher::belong(point_.peek(), parser_detail::alphabet::symbol))
				{
					if (point_.peek() != operator_assign_name::value[0] && parser_detail::operator_matcher::match(begin.str(point_)) && not parser_detail::operator_matcher::match(begin.str(point_ + 1)))
					{
						// don't throw this away, it's a good match and the next is not
					}
					else
					{
						point_ = begin;
						return false;
					}
				}
				return result;
			}

			/**
			 * @brief Parses a variable specified with a & aka reference
			 *
			 * @todo reference format?
			 *
			 * @throw exception::eval_error throw from build_symbol("&")
			 * @throw exception::eval_error throw from build_identifier(true)
			 * @throw exception::eval_error Incomplete '&'(aka reference) expression
			 */
			[[nodiscard]] bool build_reference()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (build_symbol("&"))
				{
					if (not build_identifier(true)) { throw exception::eval_error{"Incomplete '&'(aka reference) expression", filename_, point_}; }

					build_match<reference_ast_node>(prev_size);
					return true;
				}
				return false;
			}

			/**
			 * @brief Reads an expression surrounded by parentheses from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_operator()
			 * @throw exception::eval_error Incomplete expression
			 * @throw exception::eval_error Missing closing parenthesis
			 */
			[[nodiscard]] bool build_paren_expression()
			{
				scoped_parser p{*this};

				if (build_char('('))
				{
					if (not build_operator()) { throw exception::eval_error{"Incomplete expression", filename_, point_}; }

					if (not build_char(')')) { throw exception::eval_error{"Missing closing parenthesis", filename_, point_}; }

					return true;
				}
				return false;
			}

			/**
			 * @brief Reads, and identifies, a short-form container initialization from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_container_argument_list()
			 * @throw exception::eval_error Incomplete inline container initializer, missing closing bracket
			 */
			[[nodiscard]] bool build_inline_container()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_any<keyword_inline_container_gen_name::left_type>()) { return false; }

				(void)build_container_argument_list();

				if (not build_any<keyword_inline_container_gen_name::right_type>())
				{
					throw exception::eval_error{
							"Incomplete inline container initializer, missing closing bracket ']'",
							filename_,
							point_};
				}

				if (prev_size != match_stack_.size() && not match_stack_.back()->empty() && match_stack_.back()->front().is<map_pair_ast_node>()) { build_match<inline_map_ast_node>(prev_size); }
				else { build_match<inline_array_ast_node>(prev_size); }

				return true;
			}

			/**
			 * @brief Reads a pair of values used to create a map initialization from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_operator()
			 * @throw exception::eval_error Incomplete map pair, missing the second
			 */
			[[nodiscard]] bool build_map_pair()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				const auto begin = point_;
				if (not build_operator()) { return false; }

				if (not build_any<keyword_pair_splitter_name>())
				{
					point_ = begin;
					while (prev_size != match_stack_.size()) { match_stack_.pop_back(); }
					return false;
				}

				if (not build_operator())
				{
					throw exception::eval_error{
							"Incomplete pair, missing the second",
							filename_,
							point_};
				}

				build_match<map_pair_ast_node>(prev_size);
				return true;
			}

			/**
			 * @brief Reads a unary prefixed expression from input
			 *
			 * @todo need to be optimized
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_operator(group_id)
			 * @throw exception::eval_error Incomplete unary prefix expression
			 */
			[[nodiscard]] bool build_unary_expression()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				auto operator_builder = [this, prev_size]<typename Element>
				{
					if (not build_operator(parser_detail::operator_matcher::operators.size() - 1))
					{
						throw exception::eval_error{
								std_format::format("Incomplete unary prefix '{}' expression", Element::value),
								filename_,
								point_};
					}

					build_match<unary_operator_ast_node>(prev_size, Element::value);
				};

				return [this, operator_builder]<std::size_t... Index>(std::index_sequence<Index...>) -> bool
				{
					auto f = [this, operator_builder]<std::size_t I>
					{
						using type = std::tuple_element_t<I, decltype(parser_detail::operator_matcher::m10)>;
						if constexpr (type::size_no_0 == 1)
						{
							if (this->build_char(type::value[0]))
							{
								operator_builder.decltype(operator_builder)::operator()<type>();
								return true;
							}
							return false;
						}
						else
						{
							if (this->build_symbol(type::value))
							{
								operator_builder.decltype(operator_builder)::operator()<type>();
								return true;
							}
							return false;
						}
					};

					return (f.decltype(f)::template operator()<Index>() || ...);
				}(std::make_index_sequence<std::tuple_size_v<decltype(parser_detail::operator_matcher::m10)>>{});
			}

			/**
			 * @brief Reads a dot expression(member access), then proceeds to check if it's a function or array call
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_identifier(true) || build_lambda() || build_number() || build_quoted_string() || build_single_quoted_string() || build_paren_expression() || build_inline_container()
			 * @throw exception::eval_error throw from build_argument_list()
			 * @throw exception::eval_error throw from build_operator()
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error Incomplete function call
			 * @throw exception::eval_error Incomplete array access
			 */
			[[nodiscard]] bool build_dot_fun_call()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not(build_identifier(true) || build_lambda() || build_number() || build_quoted_string() || build_single_quoted_string() || build_paren_expression() || build_inline_container())) { return false; }

				while (true)
				{
					// func(arguments)
					if (build_char('('))
					{
						(void)build_argument_list();

						if (not build_char(')'))
						{
							throw exception::eval_error{
									"Incomplete function call, missing ')'",
									filename_,
									point_};
						}

						build_match<fun_call_ast_node>(prev_size);
						// todo: Workaround for method calls until we have a better solution
						if (match_stack_.empty())
						{
							throw exception::eval_error{
									"Incomplete dot access fun call",
									filename_,
									point_};
						}

						if (auto& back = match_stack_.back();
							back->empty())
						{
							throw exception::eval_error{
									"Incomplete dot access fun call",
									filename_,
									point_};
						}
						else if (back->front().is<dot_access_ast_node>())
						{
							if (back->front().empty())
							{
								throw exception::eval_error{
										"Incomplete dot access fun call",
										filename_,
										point_};
							}

							auto dot_access = std::move(back->front_ptr());
							auto fun_call = std::move(back);
							match_stack_.pop_back();

							ast_node::children_type dot_access_children = dot_access->exchange_children();
							ast_node::children_type fun_call_children = fun_call->exchange_children();

							fun_call_children.front().swap(dot_access_children.back());
							dot_access_children.pop_back();

							[[maybe_unused]] const auto ef = fun_call->exchange_children(std::move(fun_call_children));
							gal_assert(ef.empty());
							dot_access_children.push_back(std::move(fun_call));
							[[maybe_unused]] const auto ed = dot_access->exchange_children(std::move(dot_access_children));
							gal_assert(ed.empty());

							if (dot_access->size() != 2)
							{
								throw exception::eval_error{
										"Incomplete dot access fun call",
										filename_,
										point_};
							}
							match_stack_.emplace_back(std::move(dot_access));
						}
					}
					// arr[index]
					else if (build_any<array_access_interface_name::left_type>())
					{
						if (not(build_operator() && build_any<array_access_interface_name::right_type>()))
						{
							throw exception::eval_error{
									"Incomplete array access, missing ']'",
									filename_,
									point_};
						}

						build_match<array_access_ast_node>(prev_size);
					}
					// object.method
					else if (build_symbol("."))
					{
						if (not build_identifier(true))
						{
							throw exception::eval_error{
									"Incomplete dot access fun call, missing function name",
									filename_,
									point_};
						}

						if (match_stack_.size() - prev_size != 2)
						{
							throw exception::eval_error{
									"Incomplete dot access fun call",
									filename_,
									point_};
						}

						build_match<dot_access_ast_node>(prev_size);
					}
					else if (build_eol())
					{
						const auto begin = --point_;

						while (build_eol()) {}

						if (build_symbol(".")) { --point_; }
						else
						{
							point_ = begin;
							break;
						}
					}
					else { break; }
				}

				return true;
			}

			/**
			 * @brief Reads a variable declaration from input
			 *
			 * @throw exception::eval_error throw from build_keyword("decl") || build_keyword("var") || build_keyword("global")
			 * @throw exception::eval_error throw from build_identifier(true)
			 * @throw exception::eval_error throw from build_reference()
			 * @throw exception::eval_error Incomplete member declaration
			 * @throw exception::eval_error Incomplete variable declaration
			 * @throw exception::eval_error Incomplete global declaration
			 */
			[[nodiscard]] bool build_var_decl(const bool class_context = false, const foundation::string_view_type class_name = "unknown_class")
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (build_keyword(keyword_variable_declare_name::value))
				{
					if (class_context)
					{
						match_stack_.emplace_back(this->make_node<id_ast_node>(class_name, point_));
						if (not build_identifier(true))
						{
							throw exception::eval_error{
									"Incomplete member declaration, missing variable name.",
									filename_,
									point_};
						}
						build_match<member_decl_ast_node>(prev_size);
						return true;
					}

					if (build_reference())
					{
						// we built a reference node - continue
					}
					else if (build_identifier(true)) { build_match<var_decl_ast_node>(prev_size); }
					else { throw exception::eval_error{"Incomplete variable declaration", filename_, point_}; }
					return true;
				}

				if (build_keyword(keyword_global_name::value))
				{
					if (not(build_reference() || build_identifier(true))) { throw exception::eval_error{"Incomplete global declaration", filename_, point_}; }

					build_match<global_decl_ast_node>(prev_size);
					return true;
				}

				return false;
			}

			/**
			 * @brief Parses any of a group of 'value' style ast_node groups from input
			 *
			 * @throw exception::eval_error throw from build_unary_expression() || build_dot_fun_call() || build_var_decl()
			 */
			[[nodiscard]] bool build_value()
			{
				scoped_parser p{*this};
				// todo: Although these branches do not have no side effects on the `state` of the parser, misjudgment can have a significant effect on the `efficiency` of the parser.
				// We need to be careful about the order of these branches.
				return build_var_decl() || build_dot_fun_call() || build_unary_expression();
			}

			/**
			 * @todo Need to be optimized
			 *
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error Incomplete expression
			 */
			[[nodiscard]] bool build_operator(const parser_detail::operator_matcher::group_id_type group_id = parser_detail::operator_matcher::group_ids[0])
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_value()) { return false; }

				// skip the unary one
				for (auto i = parser_detail::operator_matcher::group_size - 2; i >= group_id; --i)
				{
					parser_detail::operator_matcher::operator_name_type op;
					while (read_operator(i, op))
					{
						while (build_eol()) {}

						if (not build_operator(i + 1))
						{
							throw exception::eval_error{
									std_format::format("Incomplete '{}' expression", op),
									filename_,
									point_};
						}

						switch (parser_detail::operator_matcher::operators[i])
						{
								using enum operation_precedence;
							case logical_or:
							{
								build_match<logical_or_ast_node>(prev_size, op);
								break;
							}
							case logical_and:
							{
								build_match<logical_and_ast_node>(prev_size, op);
								break;
							}
							case bitwise_or:
							case bitwise_xor:
							case bitwise_and:
							case equality:
							case comparison:
							case bitwise_shift:
							case plus_minus:
							case multiply_divide:
							{
								build_match<binary_operator_ast_node>(prev_size, op);
								break;
							}
							case unary:
							case operation_size:
							{
								// cannot reach here because of if() statement at the top
								UNREACHABLE();
							}
							default: ;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Parses a string of binary equation operators
			 *
			 * @todo Need to be optimized
			 *
			 * @throw exception::eval_error throw from build_operator()
			 * @throw exception::eval_error throw from build_symbol(" ")
			 * @throw exception::eval_error throw from skip_whitespace(true)
			 * @throw exception::eval_error Incomplete equation
			 */
			[[nodiscard]] bool build_equation()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (build_operator())
				{
					using operator_name_type = parser_detail::operator_matcher::operator_name_type;

					for (constexpr std::array operators{
							     operator_name_type{operator_assign_name::value},
							     operator_name_type{operator_reference_assign_name::value},
							     operator_name_type{operator_plus_assign_name::value},
							     operator_name_type{operator_minus_assign_name::value},
							     operator_name_type{operator_multiply_assign_name::value},
							     operator_name_type{operator_divide_assign_name::value},
							     operator_name_type{operator_remainder_assign_name::value},
							     operator_name_type{operator_bitwise_shift_left_assign_name::value},
							     operator_name_type{operator_bitwise_shift_right_assign_name::value},
							     operator_name_type{operator_bitwise_and_assign_name::value},
							     operator_name_type{operator_bitwise_or_assign_name::value},
							     operator_name_type{operator_bitwise_xor_assign_name::value}};
					     const auto& op: operators)
					{
						if (build_symbol(op, true))
						{
							skip_whitespace(true);
							if (not build_equation()) { throw exception::eval_error{"Incomplete equation", filename_, point_}; }

							build_match<equation_ast_node>(prev_size, op);
							return true;
						}
					}
					return true;
				}

				return false;
			}

			/**
			 * @brief Top level parser, starts parsing of all known parses
			 *
			 * @throw exception::eval_error throw from build_def() || build_if() || build_while() || build_for() || build_switch() || build_class(class_allowed) || build_try()
			 * @throw exception::eval_error throw from build_equation() || build_return() || build_break() || build_continue()
			 * @throw exception::eval_error throw from build_block() || build_eol()
			 */
			[[nodiscard]] bool build_statements(const bool class_allowed = false)
			{
				scoped_parser p{*this};

				auto result = false;
				for (auto has_more = true, saw_eol = true; has_more;)
				{
					const auto begin = point_;
					// todo: Although these branches do not have no side effects on the `state` of the parser, misjudgment can have a significant effect on the `efficiency` of the parser.
					// We need to be careful about the order of these branches.
					if (build_block() || build_eol())
					{
						has_more = true;
						result = true;
						saw_eol = true;
					}
					else if (build_def() || build_if() || build_while() || build_for() || build_match() || build_class(class_allowed) || build_try())
					{
						if (not saw_eol)
						{
							throw exception::eval_error{
									"Two function definitions missing line separator",
									filename_,
									begin};
						}
						has_more = true;
						result = true;
						saw_eol = true;
					}
					else if (build_equation() || build_return() || build_break() || build_continue())
					{
						if (not saw_eol)
						{
							throw exception::eval_error{
									"Two function definitions missing line separator",
									filename_,
									begin};
						}
						has_more = true;
						result = true;
						saw_eol = false;
					}
					else { has_more = false; }
				}

				return result;
			}

			/**
			 * @brief Parses statements allowed inside of a class block
			 *
			 * @throw exception::eval_error throw from build_def(true, " ") || build_var_decl(true, " ")
			 * @throw exception::eval_error throw from build_eol()
			 * @throw exception::eval_error Two function definitions missing line separator
			 */
			[[nodiscard]] bool build_class_statements(const foundation::string_view_type class_name)
			{
				scoped_parser p{*this};

				auto result = false;
				for (auto has_more = true, saw_eol = true; has_more;)
				{
					const auto begin = point_;
					if (build_def(true, class_name) || build_var_decl(true, class_name))
					{
						if (not saw_eol)
						{
							throw exception::eval_error{
									"Two function definitions missing line separator",
									filename_,
									begin};
						}
						has_more = true;
						result = true;
						saw_eol = true;
					}
					else if (build_eol())
					{
						has_more = true;
						result = true;
						saw_eol = true;
					}
					else { has_more = false; }
				}

				return result;
			}

			/**
			 * @brief Reads a block from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_statements()
			 */
			[[nodiscard]] bool build_block()
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				while (build_eol()) {}

				if (build_any<keyword_block_begin_name>())
				{
					(void)build_statements();
					// todo: do we need keyword_block_end_name ?

					if (match_stack_.size() == prev_size) { match_stack_.emplace_back(lang::make_node<noop_ast_node>()); }

					build_match<block_ast_node>(prev_size);
					return true;
				}
				return false;
			}

			/**
			 * @brief Reads a curly-brace class block from input
			 *
			 * @throw exception::eval_error throw from build_char(' ') || build_symbol(" ")
			 * @throw exception::eval_error throw from build_class_statements(" ")
			 * @throw exception::eval_error Incomplete class block
			 */
			[[nodiscard]] bool build_class_block(const foundation::string_view_type class_name)
			{
				scoped_parser p{*this};
				const auto prev_size = match_stack_.size();

				if (not build_char('{')) { return false; }

				(void)build_class_statements(class_name);

				if (not build_char('}'))
				{
					throw exception::eval_error{
							"Incomplete class block, missing '}'",
							filename_,
							point_};
				}

				if (match_stack_.size() == prev_size) { match_stack_.emplace_back(lang::make_node<noop_ast_node>()); }

				build_match<block_ast_node>(prev_size);
				return true;
			}

		private:
			/**
			 * @brief Parses the given input string, tagging parsed ast_nodes with the given filename.
			 *
			 * @throw exception::eval_error throw from build_statements(true)
			 */
			[[nodiscard]] ast_node_ptr parse_internal(const foundation::string_view_type input, const foundation::string_view_type filename)
			{
				const auto begin = input.empty() ? nullptr : input.data();
				const auto end = begin ? begin + input.size() : nullptr;

				point_ = parser_detail::parse_point{begin, end};
				filename_ = filename;

				if (build_statements(true))
				{
					if (not point_.finish())
					{
						throw exception::eval_error{
								"Unparsed input remained",
								filename_,
								point_};
					}
					build_match<file_ast_node>(0);
				}
				else { match_stack_.emplace_back(lang::make_node<noop_ast_node>()); }

				gal_assert(match_stack_.size() == 1);
				return std::move(std::exchange(match_stack_, ast_node::children_type{}).front());
			}

			/**
			 * @throw exception::eval_error throw from parse_internal(" ")
			 */
			[[nodiscard]] ast_node_ptr parse_instruct_eval(const foundation::string_view_type input)
			{
				const auto last_point = point_;
				const auto last_filename = filename_;
				auto last_match_stack = std::exchange(match_stack_, ast_node::children_type{});

				auto result = parse_internal(input, "instruction_eval");

				point_ = last_point;
				filename_ = last_filename;
				match_stack_ = std::move(last_match_stack);

				return result;
			}

		public:
			explicit parser(std::reference_wrapper<ast_visitor> visitor, std::reference_wrapper<ast_optimizer> optimizer, const std::size_t max_parse_depth = 512)
				: visitor_{visitor},
				  optimizer_{optimizer},
				  max_parse_depth_{max_parse_depth},
				  current_parse_depth_{0} { match_stack_.reserve(2); }

			[[nodiscard]] ast_node_ptr parse(const foundation::string_view_type input, const foundation::string_view_type filename) override
			{
				parser p{visitor_, optimizer_};
				return p.parse_internal(input, filename);
			}
		};
	}
}

#endif // GAL_LANG_LANGUAGE_PARSER_HPP
