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
			constexpr static auto build() noexcept
			{
				using return_type = std::array<std::array<bool, static_cast<std::size_t>(alphabet::alphabet_size)>, max_alphabet_length>;

				return_type alphabets{};

				auto setter = [&alphabets](alphabet which_kind, const int what) { alphabets[static_cast<std::size_t>(which_kind)][static_cast<std::size_t>(what)] = true; };

				setter(alphabet::symbol, lang::operator_assign_name::value[0]);

				setter(alphabet::symbol, lang::operator_less_than_name::value[0]);
				setter(alphabet::symbol, lang::operator_greater_than_name::value[0]);

				setter(alphabet::symbol, lang::operator_plus_name::value[0]);
				setter(alphabet::symbol, lang::operator_minus_name::value[0]);
				setter(alphabet::symbol, lang::operator_multiply_name::value[0]);
				setter(alphabet::symbol, lang::operator_divide_name::value[0]);
				setter(alphabet::symbol, lang::operator_remainder_name::value[0]);

				setter(alphabet::symbol, lang::operator_bitwise_and_name::value[0]);
				setter(alphabet::symbol, lang::operator_bitwise_or_name::value[0]);
				setter(alphabet::symbol, lang::operator_bitwise_xor_name::value[0]);

				setter(alphabet::symbol, '.');
				setter(alphabet::floating_point, '.');
				setter(alphabet::keyword, '_');
				setter(alphabet::identifier, '_');

				setter(alphabet::whitespace, ' ');
				setter(alphabet::whitespace, '\t');

				for (auto i = 'a'; i < 'z'; ++i)
				{
					setter(alphabet::keyword, i);
					setter(alphabet::keyword, i + ('a' - 'A'));

					setter(alphabet::identifier, i);
					setter(alphabet::identifier, i + ('a' - 'A'));
				}

				for (auto i = '0'; i < '9'; ++i)
				{
					setter(alphabet::keyword, i);

					setter(alphabet::integer, i);
					setter(alphabet::floating_point, i);
					setter(alphabet::hex, i);
				}

				setter(alphabet::integer_suffix, 'l');
				setter(alphabet::integer_suffix, 'L');
				setter(alphabet::integer_suffix, 'u');
				setter(alphabet::integer_suffix, 'U');
				setter(alphabet::floating_point_suffix, 'l');
				setter(alphabet::floating_point_suffix, 'L');
				setter(alphabet::floating_point_suffix, 'f');
				setter(alphabet::floating_point_suffix, 'F');

				for (auto i = '0'; i < '1'; ++i) { setter(alphabet::bin, i); }
				setter(alphabet::bin_prefix, 'b');
				setter(alphabet::bin_prefix, 'B');

				for (auto i = 'a'; i < 'f'; ++i)
				{
					setter(alphabet::hex, i);
					setter(alphabet::hex, i + ('a' - 'A'));
				}
				setter(alphabet::hex_prefix, 'x');
				setter(alphabet::hex_prefix, 'X');

				return alphabets;
			}

			constexpr static auto alphabets = build();

		public:
			[[nodiscard]] constexpr static bool belong(const char c, const alphabet a) noexcept { return alphabets[static_cast<std::size_t>(a)][c]; }
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

			[[nodiscard]] std::string_view str(const parse_point& end) const noexcept
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
				gal_assert(offset > 0);
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
				gal_assert(offset > 0);
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

			[[nodiscard]] constexpr char peek() const noexcept
			{
				if (not finish()) { return *current_; }
				return invalid_char;
			}
		};

		struct operator_matcher
		{
			using operator_name_type = std::string_view;
			using group_id_type = std::size_t;

			constexpr group_id_type group_ids[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

			consteval operator_name_type m1[]{
					{lang::operator_logical_and_name::value}};
			constexpr operator_name_type m2[]{
					{lang::operator_logical_or_name::value}};
			constexpr operator_name_type m3[]{
					{lang::operator_bitwise_or_name::value}};
			constexpr operator_name_type m4[]{
					{lang::operator_bitwise_xor_name::value}};
			constexpr operator_name_type m5[]{
					{lang::operator_bitwise_and_name::value}};
			constexpr operator_name_type m6[]{
					{lang::operator_equal_name::value},
					{lang::operator_not_equal_name::value}};
			constexpr operator_name_type m7[]{
					{lang::operator_less_than_name::value},
					{lang::operator_less_equal_name::value},
					{lang::operator_greater_than_name::value},
					{lang::operator_greater_equal_name::value}};
			constexpr operator_name_type m8[]{
					{lang::operator_bitwise_shift_left_name::value},
					{lang::operator_bitwise_shift_right_name::value}};
			// We share precedence here but then separate them later
			constexpr operator_name_type m9[]{
					{lang::operator_plus_name::value},
					{lang::operator_minus_name::value}};
			constexpr operator_name_type m10[]{
					{lang::operator_multiply_name::value},
					{lang::operator_divide_name::value},
					{lang::operator_remainder_name::value}};
			constexpr operator_name_type m11[]{
					{lang::operator_unary_not_name::value},
					{lang::operator_unary_plus_name::value},
					{lang::operator_unary_minus_name::value},
					{lang::operator_unary_bitwise_complement_name::value}};

			template<typename Predicate>
			[[nodiscard]] constexpr bool any_of(const group_id_type group_id, Predicate&& predicate) const noexcept
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

			[[nodiscard]] constexpr bool match(const group_id_type group_id, const operator_name_type name) const noexcept { return this->any_of(group_id, [name](const auto& n) { return name == n; }); }

			[[nodiscard]] constexpr bool match(const operator_name_type name) const noexcept { return std::ranges::any_of(group_ids, [name, this](const auto id) { return this->match(id, name); }); }
		};
	}


	namespace lang
	{
		template<typename VisitorType, typename OptimizerType, std::size_t MaxParseDepth = 512>
		class parser final : public parser_detail::parser_base
		{
		public:
			using visitor_type = VisitorType;
			using optimizer_type = OptimizerType;

			constexpr static std::size_t max_parse_depth = MaxParseDepth;

		private:
			struct scoped_parser : utils::scoped_object<scoped_parser>
			{
			private:
				std::reference_wrapper<parser> p_;

			public:
				constexpr explicit scoped_parser(parser& p)
					: p_{p} {}

			private:
				constexpr void do_construct()
				{
					if (auto& depth = p_.get().current_parse_depth_;
						depth >= max_parse_depth)
					{
						throw exception::eval_error{
								std_format::format("Maximum parse depth '{}' exceeded", max_parse_depth),
								*p_.get().filename_,
								p_.get().point_
						};
					}
					else { ++depth; }
				}

				constexpr void do_destruct() noexcept { --p_.get().current_parse_depth_; }
			};

			parser_detail::parse_point point_;

			visitor_type visitor_;
			optimizer_type optimizer_;

			std::size_t current_parse_depth_;

			std::shared_ptr<std::string> filename_;
			std::vector<ast_node_ptr> match_stack_;

			/**
			 * @throw eval_error if is not a valid object name
			 */
			void check_object_name(const name_validator::name_type name) const
			{
				if (not name_validator::is_valid_object_name(name))
				{
					throw exception::eval_error{
							std_format::format("Object name '{}' is an invalid name!"),
							*filename_,
							point_};
				}
			}

			[[nodiscard]] void* get_visitor_ptr() override { return &visitor_; }

		public:
			[[nodiscard]] visitor_type& get_visitor() noexcept { return visitor_; }

			[[nodiscard]] optimizer_type& get_optimizer() noexcept { return optimizer_; }

			[[nodiscard]] ast_node_ptr parse(std::string_view input, std::string_view filename) override;

			/**
			 * @brief Prints the parsed ast_nodes as a tree
			 */
			void debug_print_to(std::string& dest, const ast_node& node, std::string_view prepend) const override
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
			[[nodiscard]] std::string debug_print(const ast_node& node, std::string_view prepend) const override
			{
				std::string ret{};
				debug_print_to(ret, node, prepend);
				return ret;
			}
		};
	}
}

#endif // GAL_LANG_LANGUAGE_PARSER_HPP
