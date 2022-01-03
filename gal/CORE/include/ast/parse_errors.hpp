#pragma once

#ifndef GAL_LANG_AST_PARSE_ERRORS_HPP
#define GAL_LANG_AST_PARSE_ERRORS_HPP

#include <string>
#include <vector>
#include <exception>
#include <utils/point.hpp>
#include <utils/format.hpp>
#include <utils/assert.hpp>

namespace gal::ast
{
	class parse_error final : public std::exception
	{
	private:
		utils::location loc_;
		std::string message_;
	public:
		parse_error(const utils::location loc, std::string message)
			: loc_{loc},
			  message_{std::move(message)} {}

		[[nodiscard]] constexpr utils::location where_error() const noexcept { return loc_; }

		[[nodiscard]] constexpr const std::string& what_error() const noexcept { return message_; }

		[[deprecated("use what_error instead")]][[nodiscard]] const char* what() const override { return message_.c_str(); }
	};

	class parse_errors final : public std::exception
	{
	public:
		using parse_errors_container_type = std::vector<parse_error>;

	private:
		parse_errors_container_type errors_;
		std::string message_;

	public:
		explicit parse_errors(parse_errors_container_type errors)
			: errors_{std::move(errors)},
			  message_{errors_.size() == 1 ? errors_.front().what_error() : std_format::format("Total {} error happened", errors_.size())} { gal_assert(not errors_.empty(), "At least one error needs to occur!"); }

		[[nodiscard]] constexpr const std::string& what_error() const noexcept { return message_; }

		[[deprecated("use what_error instead")]] [[nodiscard]] const char* what() const override { return message_.c_str(); }

		[[nodiscard]] constexpr auto errors_size() const noexcept { return errors_.size(); }

		constexpr void handle_errors(std::invocable<parse_error> auto func, std::invocable<parse_errors_container_type> auto finisher)
		{
			for (auto& error: errors_) { func(error); }

			finisher(errors_);
		}
	};
}

#endif // GAL_LANG_AST_PARSE_ERRORS_HPP
