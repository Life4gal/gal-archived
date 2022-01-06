#pragma once

#ifndef GAL_LANG_AST_LEXER_HPP
#define GAL_LANG_AST_LEXER_HPP

#include <variant>
#include <concepts>

#include <utils/point.hpp>
#include <utils/assert.hpp>
#include <utils/enum_utils.hpp>
#include <utils/confusable.hpp>
#include <utils/hash_container.hpp>
#include <utils/string_pool.hpp>
#include <utils/string_utils.hpp>
#include <utils/hash.hpp>

#include <ast/ast.hpp>
#include <ast/allocator.hpp>

namespace gal::ast
{
	class lexeme_point
	{
	public:
		/**
		 * @note
		 *		lexeme_point::to_string
		 *		lexer::read_next
		 */
		enum class token_type
		{
			eof = 0,
			char_sentinel_begin = 0,

			// 1 ~ 255 => character

			char_sentinel_end = 256,

			// **
			pow,
			// ==
			equal,
			// !=
			not_equal,
			// <=
			less_equal,
			// >=
			greater_equal,

			// <<
			bitwise_left_shift,
			// >>
			bitwise_right_shift,

			// +=
			plus_assign,
			// -=
			minus_assign,
			// *=
			multiply_assign,
			// /=
			divide_assign,
			// %=
			modulus_assign,
			// **=
			pow_assign,
			// ::
			double_colon,
			// ->
			right_arrow,
			// ...
			ellipsis,

			raw_string,
			// ''' string ''' or """ string """ 
			quoted_string,
			number,
			name,

			// # some comment
			comment,
			// <[optional level number]< string_l1
			// string_l2
			// string_l3 >[optional level number]>
			// ===> string_l1string_l2sting_l3
			block_comment,

			broken_string,
			broken_comment,
			broken_unicode,
			error,

			keyword_sentinel_begin,

			keyword_and,
			keyword_break,
			keyword_do,
			keyword_else,
			// ReSharper disable once IdentifierTypo
			keyword_elif,
			keyword_end,
			keyword_false,
			keyword_for,
			keyword_function,
			keyword_if,
			keyword_in,
			keyword_local,
			keyword_null,
			keyword_not,
			keyword_or,
			keyword_repeat,
			keyword_return,
			keyword_then,
			keyword_true,
			keyword_until,
			keyword_while,

			// this is also the sentinel of all tokens
			keyword_sentinel_end,
		};

		using keyword_literal_type = ast_name;

		using token_underlying_type = std::underlying_type_t<token_type>;

		using data_or_name_type = ast_name;
		using codepoint_type = std::uint32_t;

		using data_type = std::variant<data_or_name_type, codepoint_type>;

		constexpr static keyword_literal_type keywords[] =
		{
				{"and"},
				{"break"},
				{"do"},
				{"else"},
				// ReSharper disable once StringLiteralTypo
				{"elif"},
				{"end"},
				{"false"},
				{"for"},
				{"function"},
				{"if"},
				{"in"},
				{"local"},
				{"null"},
				{"not"},
				{"or"},
				{"repeat"},
				{"return"},
				{"then"},
				{"true"},
				{"until"},
				{"while"}
		};

		static_assert(std::size(keywords) == static_cast<token_underlying_type>(token_type::keyword_sentinel_end) - static_cast<token_underlying_type>(token_type::keyword_sentinel_begin) + 1 - 2);

		[[nodiscard]] constexpr static token_underlying_type token_to_scalar(token_type type) noexcept { return static_cast<token_underlying_type>(type); }

		[[nodiscard]] constexpr static bool is_keyword(const ast_name keyword) noexcept { return std::ranges::find(keywords, keyword) != std::ranges::end(keywords); }

		[[nodiscard]] constexpr static keyword_literal_type get_keyword(const token_type token) noexcept { return keywords[token_to_scalar(token) - token_to_scalar(token_type::keyword_sentinel_begin) - 1]; }

		constexpr static keyword_literal_type non_token_keywords[] =
		{
				{"continue"},
				{"export"},// export
				{"using"}, // type alias
				{"declare"},
				{"self"},
				{"class"},
				{"extends"},
				{"typeof"},
		};

		[[nodiscard]] constexpr static keyword_literal_type get_continue_keyword() noexcept { return non_token_keywords[0]; }
		[[nodiscard]] constexpr static keyword_literal_type get_export_keyword() noexcept { return non_token_keywords[1]; }
		[[nodiscard]] constexpr static keyword_literal_type get_type_alias_keyword() noexcept { return non_token_keywords[2]; }
		[[nodiscard]] constexpr static keyword_literal_type get_declare_keyword() noexcept { return non_token_keywords[3]; }
		[[nodiscard]] constexpr static keyword_literal_type get_self_keyword() noexcept { return non_token_keywords[4]; }
		[[nodiscard]] constexpr static keyword_literal_type get_class_keyword() noexcept { return non_token_keywords[5]; }
		[[nodiscard]] constexpr static keyword_literal_type get_extend_keyword() noexcept { return non_token_keywords[6]; }
		[[nodiscard]] constexpr static keyword_literal_type get_typeof_keyword() noexcept { return non_token_keywords[7]; }

		/**
		 * @brief 1 ~ 255
		 */
		constexpr static char non_token_symbol[] =
		{
				'=',
				'+',
				'-',
				'*',
				'/',
				'%',
				'&',
				'|',
				'^',
				'~',
				',',
				'!',
				':',
				'(',
				')',
				'[',
				']',
				'{',
				'}',
				';',
				'.',
				'<',
				'>',
				'#',
				'\'',
				'"',
				'_',
				'@',
				'?',
		};

		[[nodiscard]] constexpr static char get_assignment_symbol() noexcept { return non_token_symbol[0]; }
		[[nodiscard]] constexpr static char get_plus_symbol() noexcept { return non_token_symbol[1]; }
		[[nodiscard]] constexpr static char get_minus_symbol() noexcept { return non_token_symbol[2]; }
		[[nodiscard]] constexpr static char get_multiply_symbol() noexcept { return non_token_symbol[3]; }
		[[nodiscard]] constexpr static char get_divide_symbol() noexcept { return non_token_symbol[4]; }
		[[nodiscard]] constexpr static char get_modulus_symbol() noexcept { return non_token_symbol[5]; }
		[[nodiscard]] constexpr static char get_bitwise_and_symbol() noexcept { return non_token_symbol[6]; }
		[[nodiscard]] constexpr static char get_bitwise_or_symbol() noexcept { return non_token_symbol[7]; }
		[[nodiscard]] constexpr static char get_bitwise_xor_symbol() noexcept { return non_token_symbol[8]; }
		[[nodiscard]] constexpr static char get_bitwise_not_symbol() noexcept { return non_token_symbol[9]; }
		[[nodiscard]] constexpr static char get_comma_symbol() noexcept { return non_token_symbol[10]; }
		[[nodiscard]] constexpr static char get_not_symbol() noexcept { return non_token_symbol[11]; }
		[[nodiscard]] constexpr static char get_colon_symbol() noexcept { return non_token_symbol[12]; }
		[[nodiscard]] constexpr static char get_parentheses_bracket_open_symbol() noexcept { return non_token_symbol[13]; }
		[[nodiscard]] constexpr static char get_parentheses_bracket_close_symbol() noexcept { return non_token_symbol[14]; }
		[[nodiscard]] constexpr static char get_square_bracket_open_symbol() noexcept { return non_token_symbol[15]; }
		[[nodiscard]] constexpr static char get_square_bracket_close_symbol() noexcept { return non_token_symbol[16]; }
		[[nodiscard]] constexpr static char get_curly_bracket_open_symbol() noexcept { return non_token_symbol[17]; }
		[[nodiscard]] constexpr static char get_curly_bracket_close_symbol() noexcept { return non_token_symbol[18]; }
		[[nodiscard]] constexpr static char get_semicolon_symbol() noexcept { return non_token_symbol[19]; }
		[[nodiscard]] constexpr static char get_dot_symbol() noexcept { return non_token_symbol[20]; }
		[[nodiscard]] constexpr static char get_less_than_symbol() noexcept { return non_token_symbol[21]; }
		[[nodiscard]] constexpr static char get_greater_than_symbol() noexcept { return non_token_symbol[22]; }
		[[nodiscard]] constexpr static char get_sharp_symbol() noexcept { return non_token_symbol[23]; }
		[[nodiscard]] constexpr static char get_single_quotation_symbol() noexcept { return non_token_symbol[24]; }
		[[nodiscard]] constexpr static char get_double_quotation_symbol() noexcept { return non_token_symbol[25]; }
		[[nodiscard]] constexpr static char get_underscore_symbol() noexcept { return non_token_symbol[26]; }
		[[nodiscard]] constexpr static char get_at_symbol() noexcept { return non_token_symbol[27]; }
		[[nodiscard]] constexpr static char get_question_mark_symbol() noexcept { return non_token_symbol[28]; }

		/**
		 * @brief attempt to read a token, return the read token and the read length,
		 *
		 * @note Use the getter to get the character, the getter should accept an offset, and return the character at this offset.
		 *
		 * @note The getter should not actually change the state of the lexer, because we will return how many characters we read.
		 *
		 * @note changing the reading rules here requires corresponding changes to the output rules of to_string.
		 */
		template<typename SymbolGetter>
			requires std::is_invocable_r_v<char, SymbolGetter, std::size_t>
		[[nodiscard]] constexpr static std::pair<token_type, std::size_t> get_compound_symbol(SymbolGetter getter) noexcept(std::is_nothrow_invocable_r_v<char, SymbolGetter, std::size_t>)
		{
			std::size_t length = 0;

			auto do_consume = [&] { ++length; };

			auto do_return = [&](token_type type) { return std::make_pair(type, length); };
			auto do_return_cast = [&](std::convertible_to<token_underlying_type> auto type) { return std::make_pair(static_cast<token_type>(type), length); };

			switch (const auto c = getter(0))
			{
				case get_multiply_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_multiply_symbol())
					{
						do_consume();
						if (const auto next_next_c = getter(2); next_next_c == get_assignment_symbol())
						{
							do_consume();
							return do_return(token_type::pow_assign);
						}
						return do_return(token_type::pow);
					}
					else if (next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::pow_assign);
					}
					return do_return_cast(c);
				}
				case get_assignment_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::equal);
					}
					return do_return_cast(c);
				}
				case get_not_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::not_equal);
					}
					return do_return_cast(c);
				}
				case get_less_than_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::less_equal);
					}
					return do_return_cast(c);
				}
				case get_greater_than_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::greater_equal);
					}
					return do_return_cast(c);
				}
				case get_plus_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::plus_assign);
					}
					return do_return_cast(c);
				}
				case get_minus_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::minus_assign);
					}
					else if (next_c == get_greater_than_symbol())
					{
						do_consume();
						return do_return(token_type::right_arrow);
					}
					return do_return_cast(c);
				}
				case get_divide_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::divide_assign);
					}
					return do_return_cast(c);
				}
				case get_modulus_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_assignment_symbol())
					{
						do_consume();
						return do_return(token_type::modulus_assign);
					}
					return do_return_cast(c);
				}
				case get_colon_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_colon_symbol())
					{
						do_consume();
						return do_return(token_type::double_colon);
					}
					return do_return_cast(c);
				}
				case get_dot_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == get_dot_symbol())
					{
						do_consume();
						if (const auto next_next_c = getter(2); next_next_c == get_dot_symbol())
						{
							do_consume();
							return do_return(token_type::ellipsis);
						}
					}
					return do_return_cast(c);
				}
				case get_sharp_symbol():
				{
					do_consume();
					return do_return(token_type::comment);
				}
				case get_single_quotation_symbol():
				case get_double_quotation_symbol():
				{
					do_consume();
					if (const auto next_c = getter(1); next_c == c)
					{
						do_consume();
						if (const auto next_next_c = getter(2); next_next_c == c)
						{
							do_consume();
							return do_return(token_type::quoted_string);
						}
					}
					return do_return_cast(c);
				}
				case get_underscore_symbol():
				{
					do_consume();
					return do_return_cast(c);
				}
				default: { return do_return_cast(c); }
			}
		}

		constexpr static codepoint_type bad_codepoint = static_cast<codepoint_type>(-1);

	private:
		token_type type_;
		utils::location loc_;

		/*
		 * data -> string/number/comment -> own a piece of memory
		 * name -> name -> a view pointing to the parsed file or existing memory
		 * codepoint -> broken unicode -> just a number
		 */
		data_type data_;

	public:
		lexeme_point(const token_type type, const utils::location loc)
			: type_{type},
			  loc_{loc} { }

		lexeme_point(const token_underlying_type type, const utils::location loc)
			: lexeme_point(static_cast<token_type>(type), loc) {}

		lexeme_point(const token_type type, const utils::location loc, data_or_name_type data_or_name)
			: type_{type},
			  loc_{loc},
			  data_{data_or_name}
		{
			gal_assert(
					is_any_type_of(token_type::raw_string, token_type::quoted_string, token_type::number, token_type::comment, token_type::block_comment, token_type::name) ||
					is_any_keyword(),
					"Mismatch type! Type should be string/number/comment/name/keyword."
					);
		}

		lexeme_point(const utils::location loc, codepoint_type codepoint)
			: type_{token_type::broken_unicode},
			  loc_{loc},
			  data_{codepoint} { }

		[[nodiscard]] static lexeme_point bad_lexeme_point(const utils::location loc) { return {token_type::eof, loc}; }

		template<typename... Args>
			requires((std::is_convertible_v<Args, token_type> || std::is_convertible_v<Args, token_underlying_type>) && ...)
		[[nodiscard]] constexpr bool is_any_type_of(Args ... types) const noexcept { return utils::is_any_enum_of(type_, static_cast<token_underlying_type>(types)...); }

		template<bool Opened = true, bool Closed = true, typename T>
			requires(std::is_convertible_v<T, token_type> || std::is_convertible_v<T, token_underlying_type>)
		[[nodiscard]] constexpr bool is_between_type_of(T begin, std::type_identity_t<T> end) const noexcept { return utils::is_enum_between_of<Opened, Closed>(type_, begin, end); }

		[[nodiscard]] constexpr bool is_any_keyword() const noexcept { return is_between_type_of<false, false>(token_type::keyword_sentinel_begin, token_type::keyword_sentinel_end); }

		[[nodiscard]] constexpr bool is_any_character() const noexcept { return is_between_type_of<false, false>(token_type::char_sentinel_begin, token_type::char_sentinel_end); }

		[[nodiscard]] constexpr bool is_comment() const noexcept { return is_any_type_of(token_type::comment, token_type::block_comment); }

		[[nodiscard]] constexpr bool has_follower() const noexcept { return is_any_type_of(token_type::eof, token_type::keyword_else, token_type::keyword_elif, token_type::keyword_end, token_type::keyword_until); }

		[[nodiscard]] constexpr bool is_end_point() const noexcept { return type_ == token_type::eof; }

		[[nodiscard]] constexpr token_type get_type() const noexcept { return type_; }

		[[nodiscard]] constexpr utils::location get_location() const noexcept { return loc_; }

		constexpr void reset_location(const utils::location new_loc) noexcept { loc_ = new_loc; }

		[[nodiscard]] GAL_ASSERT_CONSTEXPR data_or_name_type get_data_or_name() const noexcept
		{
			gal_assert(std::holds_alternative<data_or_name_type>(data_), "We should be holding a string/number/comment/name/keyword, but in fact we don't.");
			return std::get<data_or_name_type>(data_);
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR codepoint_type get_codepoint() const noexcept
		{
			gal_assert(std::holds_alternative<codepoint_type>(data_), "We should be holding a codepoint, but in fact we don't.");
			return std::get<codepoint_type>(data_);
		}

		[[nodiscard]] constexpr std::optional<ast_expression_unary::operand_type> to_unary_operand() const noexcept
		{
			using enum ast_expression_unary::operand_type;
			if (type_ == token_type::keyword_not) { return unary_not; }
			const auto scalar = token_to_scalar(type_);
			if (scalar == get_not_symbol()) { return unary_not; }
			if (scalar == get_plus_symbol()) { return unary_plus; }
			if (scalar == get_minus_symbol()) { return unary_minus; }
			return std::nullopt;
		}

		template<typename Reporter>
			requires std::is_invocable_v<Reporter, utils::location, std::string>
		[[nodiscard]] constexpr std::optional<ast_expression_unary::operand_type> check_unary_operand(Reporter reporter) const noexcept
		{
			using enum ast_expression_unary::operand_type;
			// there should be nothing confusing to unary operands...
			(void)type_;
			(void)reporter;
			return std::nullopt;
		}

		[[nodiscard]] constexpr std::optional<ast_expression_binary::operand_type> to_binary_operand() const noexcept
		{
			using enum ast_expression_binary::operand_type;

			switch (type_)// NOLINT(clang-diagnostic-switch-enum)
			{
				case token_type::pow: { return binary_pow; }
				case token_type::keyword_and: { return binary_logical_and; }
				case token_type::keyword_or: { return binary_logical_or; }
				case token_type::equal: { return binary_equal; }
				case token_type::not_equal: { return binary_not_equal; }
				case token_type::less_equal: { return binary_less_equal; }
				case token_type::greater_equal: { return binary_greater_equal; }
				default: { break; }
			}

			const auto scalar = token_to_scalar(type_);
			if (scalar == get_plus_symbol()) { return binary_plus; }
			if (scalar == get_minus_symbol()) { return binary_minus; }
			if (scalar == get_multiply_symbol()) { return binary_multiply; }
			if (scalar == get_divide_symbol()) { return binary_divide; }
			if (scalar == get_modulus_symbol()) { return binary_modulus; }
			if (scalar == get_less_than_symbol()) { return binary_less_than; }
			if (scalar == get_greater_than_symbol()) { return binary_greater_than; }

			return std::nullopt;
		}

		template<typename Reporter>
			requires std::is_invocable_v<Reporter, utils::location, std::string>
		[[nodiscard]] constexpr std::optional<ast_expression_binary::operand_type> check_binary_operand(Reporter reporter) const noexcept
		{
			using enum ast_expression_binary::operand_type;
			// unfortunately we support almost all binary operands...
			// maybe the unary operand conflicts with the binary operand priority?
			(void)type_;
			(void)reporter;
			return std::nullopt;
		}

		[[nodiscard]] constexpr std::optional<ast_expression_binary::operand_type> to_compound_operand() const noexcept
		{
			using enum ast_expression_binary::operand_type;
			switch (type_)// NOLINT(clang-diagnostic-switch-enum)
			{
				case token_type::plus_assign: { return binary_plus; }
				case token_type::minus_assign: { return binary_minus; }
				case token_type::multiply_assign: { return binary_multiply; }
				case token_type::divide_assign: { return binary_divide; }
				case token_type::modulus_assign: { return binary_modulus; }
				case token_type::pow_assign: { return binary_pow; }
				default: { return std::nullopt; }
			}
		}

		[[nodiscard]] std::string to_string() const noexcept
		{
			switch (type_)// NOLINT(clang-diagnostic-switch-enum)
			{
				case token_type::eof: { return "<eof>"; }
				case token_type::pow: { return "'**'"; }
				case token_type::equal: { return "'=='"; }
				case token_type::not_equal: { return "'!='"; }
				case token_type::less_equal: { return "'<='"; }
				case token_type::greater_equal: { return "'>='"; }
				case token_type::bitwise_left_shift: { return "'<<'"; }
				case token_type::bitwise_right_shift: { return "'>>'"; }
				case token_type::plus_assign: { return "'+='"; }
				case token_type::minus_assign: { return "'-='"; }
				case token_type::multiply_assign: { return "'*='"; }
				case token_type::divide_assign: { return "'/='"; }
				case token_type::modulus_assign: { return "'%='"; }
				case token_type::pow_assign: { return "'**='"; }
				case token_type::raw_string:
				case token_type::quoted_string:
				case token_type::number:
				{
					[[likely]] if (const auto& data = get_data_or_name(); not data.empty()) { return std_format::format("{}", data); }

					[[unlikely]]
					if (type_ == token_type::raw_string || type_ == token_type::quoted_string) { return "<string>"; }
					if (type_ == token_type::number) { return "<number>"; }
					return "<identifier>";
				}
				case token_type::name:
				{
					const auto name = get_data_or_name();
					return std_format::format("{}", not name.empty() ? name : "identifier");
				}
				case token_type::comment:
				case token_type::block_comment: { return "<comment>"; }
				case token_type::double_colon: { return "'::'"; }
				case token_type::right_arrow: { return "'->'"; }
				case token_type::ellipsis: { return "'...'"; }
				case token_type::broken_string: { return "<malformed string>"; }
				case token_type::broken_comment: { return "<unfinished comment>"; }
				case token_type::broken_unicode:
				{
					gal_assert(std::holds_alternative<codepoint_type>(data_), "We should be holding a codepoint, but in fact we don't.");
					const auto codepoint = std::get<codepoint_type>(data_);
					if (const auto* confusable = utils::find_confusable(codepoint); confusable) { return std_format::format("Unicode character U+{:#0x} (did you mean '{}'?)", codepoint, confusable); }
					return std_format::format("Unicode character U+{:#0x}", codepoint);
				}
				default:
				{
					if (is_any_keyword()) { return std_format::format("'{}'", get_keyword(type_)); }
					if (is_any_character()) { return std_format::format("{}", static_cast<char>(type_)); }
					return "<unknown>";
				}
			}
		}
	};

	struct comment
	{
		lexeme_point::token_type type;
		utils::location loc;

		[[nodiscard]] constexpr bool valid_comment() const noexcept
		{
			using enum lexeme_point::token_type;
			return utils::is_any_enum_of(type, comment, block_comment, broken_comment);
		}
	};

	class ast_name_table
	{
	public:
		using name_type = ast_name;
		using name_pool_type = utils::string_pool<name_type::value_type, false, name_type::traits_type>;

	private:
		struct table_entry_view
		{
			name_type name;
		};

		struct table_entry
		{
			name_type name;
			lexeme_point::token_type type;

			[[nodiscard]] constexpr bool operator==(const table_entry& rhs) const noexcept { return name == rhs.name; }
		};

		struct entry_hasher
		{
		private:
			[[nodiscard]] constexpr std::size_t operator()(const name_type name) const noexcept { return utils::fnv1a_hash(name); }

		public:
			using is_transparent = int;

			[[nodiscard]] constexpr std::size_t operator()(const table_entry& entry) const noexcept { return this->operator()(entry.name); }

			[[nodiscard]] constexpr std::size_t operator()(const table_entry_view& entry) const noexcept { return this->operator()(entry.name); }
		};

		struct entry_equal
		{
			using is_transparent = int;

			[[nodiscard]] constexpr bool operator()(const table_entry_view& entry_view, const table_entry& entry) const noexcept { return entry_view.name == entry.name; }

			[[nodiscard]] constexpr bool operator()(const table_entry& entry, const table_entry_view& entry_view) const noexcept { return entry.name == entry_view.name; }

			[[nodiscard]] constexpr bool operator()(const table_entry& entry1, const table_entry& entry2) const noexcept { return entry1 == entry2; }
		};

		utils::hash_set<table_entry, entry_hasher, entry_equal> data_;
		name_pool_type& pool_;
	public:
		explicit ast_name_table(name_pool_type& pool)
			: pool_(pool)
		{
			using enum lexeme_point::token_type;
			constexpr auto begin = static_cast<lexeme_point::token_underlying_type>(keyword_sentinel_begin) + 1;
			constexpr auto end = static_cast<lexeme_point::token_underlying_type>(keyword_sentinel_end);
			for (auto i = begin; i < end; ++i) { insert(lexeme_point::keywords[i - begin].data(), static_cast<lexeme_point::token_type>(i)); }
		}

		name_type insert(const name_type name, const lexeme_point::token_type type = lexeme_point::token_type::name)
		{
			const auto [it, inserted] = data_.emplace(pool_.append(name), type);
			gal_assert(inserted, "Cannot insert an existed entry!");

			return it->name;
		}

		std::pair<name_type, lexeme_point::token_type> insert_if_not_exist(const name_type name)
		{
			if (const auto it = data_.find(table_entry_view{name}); it != data_.end()) { return std::make_pair(it->name, it->type); }

			const auto [it, inserted] = data_.emplace(pool_.append(name), lexeme_point::token_type::name);
			gal_assert(inserted, "This should not happened!");

			return std::make_pair(it->name, it->type);
		}

		std::pair<name_type, lexeme_point::token_type> get(const name_type name)
		{
			if (const auto it = data_.find(table_entry_view{name}); it != data_.end()) { return std::make_pair(it->name, it->type); }
			return std::make_pair(name_type{}, lexeme_point::token_type::name);
		}
	};

	class lexer
	{
	public:
		using buffer_type = ast_name;
		using offset_type = buffer_type::size_type;

		// must be signed
		using multi_line_string_level_number_type = std::int32_t;
		// level number <=> number string length
		using multi_line_string_level_type = std::pair<multi_line_string_level_number_type, offset_type>;

		constexpr static auto multi_line_string_error_format = buffer_type::npos;
		constexpr static auto multi_line_string_its_not = buffer_type::npos - 1;

	private:
		buffer_type buffer_;

		ast_name_table& name_table_;

		offset_type offset_;

		offset_type line_;
		offset_type line_offset_;

		lexeme_point point_;

		utils::location previous_loc_;

		bool skip_comment_;
		bool read_name_;

	public:
		lexer(buffer_type buffer, ast_name_table& name_table)
			: buffer_{buffer},
			  name_table_{name_table},
			  offset_{0},
			  line_{0},
			  line_offset_{0},
			  point_{lexeme_point::token_type::eof, {{0, 0}, {0, 0}}},
			  previous_loc_{{0, 0}, {0, 0}},
			  skip_comment_{false},
			  read_name_{true} {}

		void set_skip_comment(const bool skip) noexcept { skip_comment_ = skip; }

		void set_read_name(const bool read) noexcept { read_name_ = read; }

		[[nodiscard]] utils::location previous_location() const noexcept { return previous_loc_; }

		const lexeme_point& next() { return next(skip_comment_); }

		const lexeme_point& next(const bool skip_comment)
		{
			do
			{
				consume_until(utils::is_whitespace);

				previous_loc_ = point_.get_location();

				point_ = read_next();
			} while (skip_comment && point_.is_comment());

			return point_;
		}

		void next_line()
		{
			consume_until([](const auto c) { return c && not utils::is_new_line(c); });

			next();
		}

		lexeme_point peek_next()
		{
			const auto current_offset = offset_;
			const auto current_line = line_;
			const auto current_line_offset = line_offset_;
			const auto current_point = point_;
			const auto current_previous_loc = previous_loc_;

			const auto ret = next();

			offset_ = current_offset;
			line_ = current_line;
			line_offset_ = current_line_offset;
			point_ = current_point;
			previous_loc_ = current_previous_loc;

			return ret;
		}

		[[nodiscard]] const lexeme_point& current() const noexcept { return point_; }

		static bool write_quoted_string(ast_name_owned data);

		static void write_multi_line_string(ast_name_owned data);

	private:
		[[nodiscard]] constexpr const char* current_data() const noexcept { return buffer_.data() + offset_; }

		[[nodiscard]] constexpr const char* data_end() const noexcept { return buffer_.data() + buffer_.size(); }

		[[nodiscard]] constexpr char peek_char_directly() const noexcept { return peek_char_directly(0); }

		[[nodiscard]] constexpr char peek_char_directly(const offset_type offset) const noexcept { return buffer_[offset_ + offset]; }

		[[nodiscard]] constexpr char peek_char() const noexcept { return peek_char(0); }

		[[nodiscard]] constexpr char peek_char(const offset_type offset) const noexcept { return offset_ + offset < buffer_.size() ? buffer_[offset_ + offset] : static_cast<char>(0); }

		[[nodiscard]] constexpr utils::position current_position() const noexcept { return {line_, offset_ - line_offset_}; }

		constexpr void consume() noexcept
		{
			if (utils::is_new_line(peek_char_directly()))
			{
				++line_;
				line_offset_ = offset_ + 1;
			}

			++offset_;
		}

		constexpr void consume_n(const offset_type n) noexcept { for (auto i = n; i != 0; --i) { consume(); } }

		constexpr void consume_until(std::invocable<char> auto func) noexcept { while (func(peek_char())) { consume(); } }

		constexpr void consume_if(std::invocable<char> auto func) noexcept { if (func(peek_char())) { consume(); } }

		[[nodiscard]] constexpr static char multi_line_string_begin() noexcept { return lexeme_point::get_less_than_symbol(); }

		[[nodiscard]] constexpr static char multi_line_string_end() noexcept { return lexeme_point::get_greater_than_symbol(); }

		[[nodiscard]] constexpr bool is_multi_line_string_begin() const noexcept { return peek_char() == multi_line_string_begin(); }

		[[nodiscard]] constexpr bool is_multi_line_string_end() const noexcept { return peek_char() == multi_line_string_end(); }

		/**
		 * @brief Read multi line string
		 * @return return a pair, the first is the (level number), and the second is the length of the number string,
		 * if it is not a multi line string, the second is `multi_line_string_its_not`,
		 * or it is a malformed multi line string, the second is `multi_line_string_error_length`
		 *
		 * (it will just eat a '<' or '>' if something wrong)
		 *
		 * @note the second '<'/'>' will be left
		 *
		 * <level_number< string_l1
		 * string_l2
		 * string_l3 >level_number>
		 * => string_l1string_l2string_l3
		 */
		[[nodiscard]] multi_line_string_level_type read_multi_line_string_level();

		[[nodiscard]] lexeme_point read_multi_line_string(utils::position begin, multi_line_string_level_type level, lexeme_point::token_type ok, lexeme_point::token_type broken);

		[[nodiscard]] lexeme_point read_quoted_string(char quotation, std::size_t length);

		[[nodiscard]] lexeme_point read_comment();

		[[nodiscard]] std::pair<ast_name_table::name_type, lexeme_point::token_type> read_name();

		[[nodiscard]] lexeme_point read_number(utils::position begin, offset_type start_offset);

		[[nodiscard]] lexeme_point read_utf8_error();

		[[nodiscard]] lexeme_point read_next();
	};
}

#endif // GAL_LANG_AST_LEXER_HPP
