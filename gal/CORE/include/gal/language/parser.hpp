#pragma once

#ifndef GAL_LANG_LANGUAGE_PARSER_HPP
#define GAL_LANG_LANGUAGE_PARSER_HPP

#include <string_view>
#include <gal/language/common.hpp>
#include <utils/string_utils.hpp>

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
			}();
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
			[[nodiscard]] constexpr static suffix_type check(const std::string_view string)
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
				if (const auto n = peek(); n != invalid_char && n != c)
				{
					this->operator++();
					return true;
				}
				return false;
			}
		};

		struct operator_matcher
		{
			using operator_name_type = std::string_view;
			using group_id_type = std::size_t;

			constexpr static group_id_type group_ids[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

			constexpr static operator_name_type m1[]{
					{lang::operator_logical_and_name::value}};
			constexpr static operator_name_type m2[]{
					{lang::operator_logical_or_name::value}};
			constexpr static operator_name_type m3[]{
					{lang::operator_bitwise_or_name::value}};
			constexpr static operator_name_type m4[]{
					{lang::operator_bitwise_xor_name::value}};
			constexpr static operator_name_type m5[]{
					{lang::operator_bitwise_and_name::value}};
			constexpr static operator_name_type m6[]{
					{lang::operator_equal_name::value},
					{lang::operator_not_equal_name::value}};
			constexpr static operator_name_type m7[]{
					{lang::operator_less_than_name::value},
					{lang::operator_less_equal_name::value},
					{lang::operator_greater_than_name::value},
					{lang::operator_greater_equal_name::value}};
			constexpr static operator_name_type m8[]{
					{lang::operator_bitwise_shift_left_name::value},
					{lang::operator_bitwise_shift_right_name::value}};
			// We share precedence here but then separate them later
			constexpr static operator_name_type m9[]{
					{lang::operator_plus_name::value},
					{lang::operator_minus_name::value}};
			constexpr static operator_name_type m10[]{
					{lang::operator_multiply_name::value},
					{lang::operator_divide_name::value},
					{lang::operator_remainder_name::value}};
			constexpr static operator_name_type m11[]{
					{lang::operator_unary_not_name::value},
					{lang::operator_unary_plus_name::value},
					{lang::operator_unary_minus_name::value},
					{lang::operator_unary_bitwise_complement_name::value}};

			template<typename Predicate>
			[[nodiscard]] constexpr static bool any_of(const group_id_type group_id, Predicate&& predicate) noexcept
			{
				auto matcher = [p = std::forward<Predicate>(predicate)](const auto& array) { return std::ranges::any_of(array, p); };

				// todo: better way
				switch (group_id)
				{
					case 1: { return matcher(m1); }
					case 2: { return matcher(m2); }
					case 3: { return matcher(m3); }
					case 4: { return matcher(m4); }
					case 5: { return matcher(m5); }
					case 7: { return matcher(m7); }
					case 8: { return matcher(m8); }
					case 9: { return matcher(m9); }
					case 10: { return matcher(m10); }
					case 11: { return matcher(m11); }
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
	}

	namespace lang
	{
		class parser final : public parser_detail::parser_base
		{
		private:
			struct scoped_parser : utils::scoped_object<scoped_parser>
			{
				friend struct utils::scoped_object<scoped_parser>;

			private:
				std::reference_wrapper<parser> p_;

			public:
				constexpr explicit scoped_parser(parser& p)
					: p_{p} {}

			private:
				constexpr void do_construct() const
				{
					if (auto& depth = p_.get().current_parse_depth_;
						depth >= p_.get().max_parse_depth_)
					{
						throw exception::eval_error{
								std_format::format("Maximum parse depth '{}' exceeded", p_.get().max_parse_depth_),
								*p_.get().filename_,
								p_.get().point_
						};
					}
					else { ++depth; }
				}

				constexpr void do_destruct() const noexcept { --p_.get().current_parse_depth_; }
			};

			parser_detail::parse_point point_;

			std::unique_ptr<ast_visitor> visitor_;
			std::unique_ptr<ast_optimizer> optimizer_;

			const std::size_t max_parse_depth_;

			std::size_t current_parse_depth_;

			parse_location::shared_filename_type filename_;
			ast_node::children_type match_stack_;

			/**
			 * @throw eval_error if is not a valid object name
			 */
			void check_object_name(const name_validator::name_type name) const
			{
				if (not name_validator::is_valid_object_name(name))
				{
					throw exception::eval_error{
							std_format::format("Object name '{}' is an invalid name!", name),
							*filename_,
							point_};
				}
			}

			[[nodiscard]] void* get_visitor_ptr() override { return &visitor_; }

		public:
			[[nodiscard]] ast_visitor& get_visitor() const noexcept { return *visitor_; }

			[[nodiscard]] ast_optimizer& get_optimizer() const noexcept { return *optimizer_; }

			[[nodiscard]] ast_node_ptr parse(std::string_view input, std::string_view filename) override;

			/**
			 * @brief Prints the parsed ast_nodes as a tree
			 */
			void debug_print_to(std::string& dest, const ast_node& node, const std::string_view prepend) const override
			{
				std_format::format_to(
						std::back_inserter(dest),
						"{}: {} at ({}, {})\n",
						prepend,
						node.identifier(),
						node.location_begin().line,
						node.location_begin().column);
				node.apply([this, &dest, prepend](const auto& child_node) { this->debug_print_to(dest, child_node, prepend); });
			}

			/**
			 * @brief Prints the parsed ast_nodes as a tree
			 */
			[[nodiscard]] std::string debug_print(const ast_node& node, const std::string_view prepend) const override
			{
				std::string ret{};
				debug_print_to(ret, node, prepend);
				return ret;
			}

			template<typename NodeType, typename... Args>
			[[nodiscard]] auto make_node(const foundation::string_view_type text, const file_point prev_point, Args&&... args) const { return lang::make_node<NodeType>(text, parse_location{filename_, {prev_point, point_}}, std::forward<Args>(args)...); }

			/**
			 * @brief Helper function that collects ast_nodes from a starting position to the top of the stack into a new AST node
			 */
			template<typename NodeType>
			void build_match(ast_node::children_type::difference_type match_begin, const foundation::string_view_type text)
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
					const auto begin_pos = match_stack_.begin() + match_begin;

					children.assign(
							std::make_move_iterator(begin_pos),
							std::make_move_iterator(match_stack_.end()));
					match_stack_.erase(begin_pos, match_stack_.end());
				}

				match_stack_.push_back(optimizer_->optimize(lang::make_node<NodeType>(text, std::move(location), std::move(children))));
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////
			/////////////////                                    PARSER                                                  ///////////////////
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/**
			 * @brief Reads a char from input if it matches the parameter, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_char(const char c) noexcept { return point_.read_char(c); }

			/**
			 * @brief Reads an end-of-line group from input, without skipping initial whitespace
			 */
			[[nodiscard]] bool read_eol(const bool eos = false) noexcept
			{
				if (point_.finish()) { return false; }

				if (read_symbol("\r\n") || read_char('\n'))
				{
					point_.point.column = 1;
					return true;
				}

				if (not eos && read_symbol(end_of_line_name::value)) { return true; }

				return false;
			}

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

			/**
			 * @brief Skips any multi-line or single-line comment
			 */
			[[nodiscard]] bool skip_comment()
			{
				if (read_symbol(comment_multi_line_name::left_type::value))
				{
					while (not point_.finish())
					{
						if (read_symbol(comment_multi_line_name::right_type::value)) { break; }
						if (not read_eol()) { ++point_; }
					}
					return true;
				}

				if (read_symbol(comment_single_line_name::value))
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

				if (read_symbol(comment_annotation_name::value))
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

				return false;
			}

			/**
			 * @brief Skips whitespace, which means space and tab, but not cr/lf
			 *
			 * @throw eval_error Illegal character read
			 */
			[[nodiscard]] bool skip_whitespace(const bool skip_cr_lf = false)
			{
				while (not point_.finish())
				{
					if (const auto c = point_[0];
						c > 0x7e) { throw exception::eval_error{std_format::format("Illegal character '{}'", c), *filename_, point_}; }
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

							return true;
						}

						if (skip_comment()) { return true; }
						break;
					}
				}

				return false;
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
			 * @brief Reads a number from the input, detecting if it's an integer or floating point
			 */
			[[nodiscard]] bool read_number() noexcept
			{
				(void)skip_whitespace();

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
			 * @brief Reads an identifier from input which conforms to identifier naming conventions, without skipping initial whitespace
			 *
			 * @throw eval_error Carriage return in identifier literal
			 * @throw eval_error Missing contents of identifier literal
			 * @throw eval_error Incomplete identifier literal
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

				if (point_.peek() == '`')
				{
					++point_;
					const auto begin = point_;

					while (not point_.finish() && point_.peek() != '`')
					{
						if (read_eol())
						{
							throw exception::eval_error{
									"Carriage return in identifier literal",
									*filename_,
									point_};
						}
						++point_;
					}

					if (begin == point_)
					{
						throw exception::eval_error{
								"Missing contents of identifier literal",
								*filename_,
								point_};
					}
					if (point_.finish())
					{
						throw exception::eval_error{
								"Incomplete identifier literal",
								*filename_,
								point_};
					}
					++point_;
					return true;
				}
				return false;
			}
		};
	}
}

#endif // GAL_LANG_LANGUAGE_PARSER_HPP
