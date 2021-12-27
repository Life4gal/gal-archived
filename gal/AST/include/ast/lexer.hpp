#pragma once

#ifndef GAL_LANG_AST_LEXER_HPP
#define GAL_LANG_AST_LEXER_HPP

#include <variant>

#include <utils/point.hpp>
#include <utils/allocator.hpp>
#include <utils/assert.hpp>
#include <utils/enum_utils.hpp>
#include <utils/confusable.hpp>
#include <utils/hash_container.hpp>
#include <utils/string_pool.hpp>
#include <utils/string_utils.hpp>

#include <ast/ast.hpp>

namespace gal::ast
{
	class lexeme_point
	{
	public:
		enum class token_type
		{
			eof = 0,
			char_sentinel_begin = 0,

			// 1 ~ 255 => character

			char_sentinel_end = 256,

			// =
			assignment,
			// ==
			equal,
			// !=
			not_equal,
			// <
			less_than,
			// <=
			less_equal,
			// >
			greater_than,
			// >=
			greater_equal,

			// +
			plus,
			// -
			minus,
			// *
			multiply,
			// /
			divide,
			// %
			modulus,
			// **
			pow,

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

			// :
			colon,
			// ::
			double_colon,
			// ->
			right_arrow,

			// (
			parentheses_bracket_open,
			// )
			parentheses_bracket_close,
			// [
			square_bracket_open,
			// ]
			square_bracket_close,
			// {
			curly_bracket_open,
			// }
			curly_bracket_close,
			// ,
			comma,
			// ;
			semicolon,

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

			keyword_sentinel_end,
		};

		constexpr static ast_name_view keywords[] =
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

		using token_underlying_type = std::underlying_type_t<token_type>;

		using string_type = ast_name_view;
		using codepoint_type = std::uint32_t;

		using data_type = std::variant<string_type, codepoint_type>;

		static_assert(std::size(keywords) == static_cast<token_underlying_type>(token_type::keyword_sentinel_end) - static_cast<token_underlying_type>(token_type::keyword_sentinel_begin) + 1 - 2);

		constexpr static bool is_keyword(const ast_name_view keyword) noexcept { return std::ranges::find(keywords, keyword) != std::ranges::end(keywords); }
		constexpr static codepoint_type bad_codepoint = static_cast<codepoint_type>(-1);

	private:
		token_type type_;
		utils::location loc_;

		/*
		 * data -> string/number/comment
		 * name -> name
		 * codepoint -> broken unicode
		 */
		data_type data_;

	public:
		lexeme_point(const token_type type, const utils::location loc)
			: type_{type},
			  loc_{loc},
			  data_{""} { }

		lexeme_point(const token_underlying_type type, const utils::location loc)
			: lexeme_point(static_cast<token_type>(type), loc) {}

		lexeme_point(const token_type type, const utils::location loc, string_type data_or_name)
			: type_{type},
			  loc_{loc},
			  data_{data_or_name}
		{
			gal_assert(
					utils::is_any_enum_of(type, token_type::raw_string, token_type::quoted_string, token_type::number, token_type::comment, token_type::block_comment, token_type::name) ||
					utils::is_enum_between_of<false, false>(type_, token_type::keyword_sentinel_begin, token_type::keyword_sentinel_end),
					"Mismatch type! Type should be string/number/comment/name/keyword."
					);
		}

		lexeme_point(const utils::location loc, codepoint_type codepoint)
			: type_{token_type::broken_unicode},
			  loc_{loc},
			  data_{codepoint} { }

		[[nodiscard]] static lexeme_point bad_lexeme_point(const utils::location loc) { return {token_type::eof, loc}; }

		[[nodiscard]] constexpr bool is_comment() const noexcept { return utils::is_any_enum_of(type_, token_type::comment, token_type::block_comment); }

		[[nodiscard]] constexpr utils::location get_location() const noexcept { return loc_; }

		[[nodiscard]] std::string to_string() const noexcept
		{
			switch (type_)// NOLINT(clang-diagnostic-switch-enum)
			{
				case token_type::eof: { return "<eof>"; }
				case token_type::equal: { return "'=='"; }
				case token_type::not_equal: { return "'!='"; }
				case token_type::less_than: { return "'<'"; }
				case token_type::less_equal: { return "'<='"; }
				case token_type::greater_than: { return "'>'"; }
				case token_type::greater_equal: { return "'>='"; }
				case token_type::plus_assign: { return "'+='"; }
				case token_type::minus_assign: { return "'-='"; }
				case token_type::multiply_assign: { return "'*='"; }
				case token_type::divide_assign: { return "'/='"; }
				case token_type::modulus_assign: { return "'%='"; }
				case token_type::pow_assign: { return "'**='"; }
				case token_type::raw_string:
				case token_type::quoted_string:
				case token_type::number:
				case token_type::name:
				{
					gal_assert(std::holds_alternative<string_type>(data_), "We should be holding a string, but in fact we don't.");
					if (const auto& data = std::get<string_type>(data_); not data.empty())
					{
						[[likely]]
								return std_format::format("{}", data);
					}

					[[unlikely]]
					if (type_ == token_type::raw_string || type_ == token_type::quoted_string) { return "<string>"; }
					if (type_ == token_type::number) { return "<number>"; }
					return "<identifier>";
				}
				case token_type::comment:
				case token_type::block_comment: { return "<comment>"; }
				case token_type::double_colon: { return "'::'"; }
				case token_type::right_arrow: { return "'->'"; }
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
					if (utils::is_enum_between_of<false, false>(type_, token_type::char_sentinel_begin, token_type::char_sentinel_end)) { return std_format::format("{}", static_cast<char>(type_)); }
					if (utils::is_enum_between_of<false, false>(type_, token_type::keyword_sentinel_begin, token_type::keyword_sentinel_end)) { return std_format::format("{}", keywords[static_cast<token_underlying_type>(type_) - static_cast<token_underlying_type>(token_type::keyword_sentinel_begin) - 1]); }
					return "<unknown>";
				}
			}
		}
	};

	class ast_name_table
	{
	public:
		using name_type = ast_name_view;
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
			[[nodiscard]] constexpr std::size_t operator()(const ast_name_view name) const noexcept
			{
				// FNV-1a hash. See: http://www.isthe.com/chongo/tech/comp/fnv/
				constexpr std::uint64_t hash_init{14695981039346656037ull};
				constexpr std::uint64_t hash_prime{1099511628211ull};

				auto hash = hash_init;
				for (const auto c: name)
				{
					hash ^= c;
					hash *= hash_prime;
				}
				return hash;
			}

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

		name_type insert(const name_type name, const lexeme_point::token_type type)
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

	// todo: lexer
	class lexer
	{
	public:
		using buffer_type = ast_name_view;
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

		lexeme_point peek()
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

		static bool write_quoted_string(ast_name data);

		static void write_multi_line_string(ast_name data);

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

		constexpr void consume_until(std::invocable<char> auto func) noexcept { while (func(peek_char())) { consume(); } }

		constexpr void consume_if(std::invocable<char> auto func) noexcept { if (func(peek_char())) { consume(); } }

		[[nodiscard]] constexpr static char comment_begin() noexcept { return '#'; }

		[[nodiscard]] constexpr static offset_type comment_begin_length() noexcept { return 1; }

		[[nodiscard]] constexpr bool is_comment_begin() const noexcept
		{
			// # here are some comments
			return peek_char() == comment_begin();
		}

		constexpr void consume_comment_begin() noexcept { for (auto i = comment_begin_length(); i != 0; --i) { consume(); } }

		[[nodiscard]] constexpr static char multi_line_string_begin() noexcept { return '<'; }

		[[nodiscard]] constexpr static char multi_line_string_end() noexcept { return '>'; }

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

		[[nodiscard]] constexpr static char quoted_string_begin1() noexcept { return '\''; }

		[[nodiscard]] constexpr static char quoted_string_begin2() noexcept { return '"'; }

		[[nodiscard]] constexpr static offset_type quoted_string_begin_or_end_length() noexcept { return 3; }

		/**
		 * @brief """ string """ or ''' string '''
		 */
		[[nodiscard]] constexpr std::pair<bool, char> is_quoted_string_begin() const noexcept { return std::make_pair((peek_char(0) == quoted_string_begin1() && peek_char(1) == quoted_string_begin1() && peek_char(2) == quoted_string_begin1()) || (peek_char(0) == quoted_string_begin2() && peek_char(1) == quoted_string_begin2() && peek_char(2) == quoted_string_begin2()), peek_char()); }

		[[nodiscard]] constexpr bool is_quoted_string_end(const char c) const noexcept { return peek_char(0) == c && peek_char(1) == c && peek_char(2) == c; }

		GAL_ASSERT_CONSTEXPR void consume_quoted_string_begin_or_end() noexcept
		{
			gal_assert(is_quoted_string_begin().first || is_quoted_string_end(is_quoted_string_begin().second), "Wrong quoted string format!");

			for (auto i = quoted_string_begin_or_end_length(); i != 0; --i) { consume(); }
		}

		[[nodiscard]] lexeme_point read_quoted_string();

		[[nodiscard]] lexeme_point read_comment();

		[[nodiscard]] std::pair<ast_name_table::name_type, lexeme_point::token_type> read_name();

		[[nodiscard]] lexeme_point read_number(utils::position begin, offset_type start_offset);

		[[nodiscard]] lexeme_point read_utf8_error();

		[[nodiscard]] lexeme_point read_next();
	};
}

#endif // GAL_LANG_AST_LEXER_HPP
