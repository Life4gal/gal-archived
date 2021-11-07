#pragma once

#ifndef GAL_LANG_OBJECT_HPP
	#define GAL_LANG_OBJECT_HPP

	#include <ast_node.hpp>

namespace gal
{
	class gal_object;
	using object_type = std::unique_ptr<gal_object>;

	/*! Represents an object. */
	class gal_object : public gal_expression
	{
	public:
		/*! Represents an object type. */
		enum class e_object_type
		{
			integer_t,
			number_t,
			string_t,
			boolean_t
		};

		/*! Represents an integer value. */
		using integer_type = std::int64_t;
		/*! Represents a number value. */
		using number_type  = double;
		/*! Represents a string value. */
		using string_type  = std::string;
		/*! Represents a boolean value. */
		using boolean_type = std::uint_fast8_t;

		[[nodiscard]] constexpr e_expression_type	  get_type() const noexcept override { return e_expression_type::object_t; }

		[[nodiscard]] std::string					  to_string() const noexcept override { return "object"; }

		[[nodiscard]] constexpr virtual e_object_type get_object_type() const noexcept = 0;
	};

	/*! Represents an integer. */
	class gal_integer final : public gal_object
	{
	public:
		using value_type = gal_object::integer_type;

	private:
		value_type value_;

	public:
		constexpr explicit gal_integer(value_type value) : value_(value) {}

		[[nodiscard]] std::string			  to_string() const noexcept override { return "integer"; }

		[[nodiscard]] constexpr e_object_type get_object_type() const noexcept override { return e_object_type::integer_t; }

		[[nodiscard]] value_type			  get_value() const noexcept { return value_; }

		void								  set_value(value_type value) noexcept { value_ = value; }
	};

	/*! Represents a number. */
	class gal_number final : public gal_object
	{
	public:
		using value_type = gal_object::number_type;

	private:
		value_type value_;

	public:
		constexpr explicit gal_number(value_type value) : value_(value) {}

		[[nodiscard]] std::string			  to_string() const noexcept override { return "number"; }

		[[nodiscard]] constexpr e_object_type get_object_type() const noexcept override { return e_object_type::number_t; }

		[[nodiscard]] value_type			  get_value() const noexcept { return value_; }

		void								  set_value(value_type value) noexcept { value_ = value; }
	};

	/*! Represents a string. */
	class gal_string final : public gal_object
	{
	public:
		using value_type = gal_object::string_type;

	private:
		value_type value_;

	public:
		explicit gal_string(value_type value) : value_(std::move(value)) {}

		[[nodiscard]] std::string			  to_string() const noexcept override { return "string"; }

		[[nodiscard]] constexpr e_object_type get_object_type() const noexcept override { return e_object_type::string_t; }

		[[nodiscard]] const value_type&		  get_value() const& noexcept { return value_; }

		[[nodiscard]] value_type&&			  get_value() && noexcept { return std::move(value_); }

		void								  set_value(value_type value) noexcept { value_ = std::move(value); }
	};

	/*! Represents a boolean. */
	class gal_boolean final : public gal_object
	{
	public:
		using value_type = gal_object::boolean_type;

	private:
		value_type value_;

	public:
		constexpr explicit gal_boolean(value_type value) : value_(value) {}

		[[nodiscard]] std::string			  to_string() const noexcept override { return "boolean"; }

		[[nodiscard]] constexpr e_object_type get_object_type() const noexcept override { return e_object_type::boolean_t; }

		constexpr explicit					  operator bool() const noexcept { return bool(value_); }

		constexpr bool						  flip() noexcept { return std::exchange(value_, !value_); }
	};
}// namespace gal

#endif//GAL_LANG_OBJECT_HPP
