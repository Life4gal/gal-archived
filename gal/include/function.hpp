#pragma once

#ifndef GAL_LANG_FUNCTION_HPP
	#define GAL_LANG_FUNCTION_HPP

	#include <ast_node.hpp>
	#include <vector>

namespace gal
{
	class gal_function;
	using function_type = std::unique_ptr<gal_function>;

	/*! Represents a args_pack. */
	class gal_args_pack final
	{
	public:
		using args_pack_type  = std::vector<gal_identifier>;
		using value_type	  = args_pack_type::value_type;
		using size_type		  = args_pack_type::size_type;
		using reference		  = args_pack_type::reference;
		using const_reference = args_pack_type::const_reference;
		using iterator		  = args_pack_type::iterator;
		using const_iterator  = args_pack_type::const_iterator;

	private:
		args_pack_type args_;

	public:
		explicit gal_args_pack(args_pack_type args = {}) : args_(std::move(args)) {}

		/*! Push more args if needed. */
		decltype(auto)				  push_arg(value_type arg) { return args_.emplace_back(std::move(arg)); }

		/*! Get an arg by operator[]. */
		[[nodiscard]] reference		  operator[](size_type index) noexcept { return args_[index]; }

		/*! Get an arg by operator[]. */
		[[nodiscard]] const_reference operator[](size_type index) const noexcept { return args_[index]; }

		/*! Get an args begin iterator. */
		[[nodiscard]] iterator		  begin() noexcept { return args_.begin(); }

		/*! Get an args begin iterator. */
		[[nodiscard]] const_iterator  begin() const noexcept { return args_.begin(); }

		/*! Get an args end iterator. */
		[[nodiscard]] iterator		  end() noexcept { return args_.end(); }

		/*! Get an args end iterator. */
		[[nodiscard]] const_iterator  end() const noexcept { return args_.end(); }
	};

	/*! Represents a prototype for a function. */
	class gal_prototype final
	{
	public:
		using args_pack_type  = gal_args_pack;
		using value_type	  = args_pack_type::value_type;
		using size_type		  = args_pack_type::size_type;
		using reference		  = args_pack_type::reference;
		using const_reference = args_pack_type::const_reference;
		using iterator		  = args_pack_type::iterator;
		using const_iterator  = args_pack_type::const_iterator;

	private:
		identifier_type name_;
		gal_args_pack	args_;

	public:
		explicit gal_prototype(identifier_type name, gal_args_pack args = gal_args_pack{})
			: name_(std::move(name)),
			  args_(std::move(args)) {}

		/*! Get prototype name. */
		[[nodiscard]] const identifier_type& get_prototype_name() const noexcept { return name_; }

		/*! Push more args if needed. */
		decltype(auto)						 push_arg(value_type arg) { return args_.push_arg(std::move(arg)); }

		/*! Get an arg by operator[]. */
		[[nodiscard]] reference				 operator[](size_type index) noexcept { return args_[index]; }

		/*! Get an arg by operator[]. */
		[[nodiscard]] const_reference		 operator[](size_type index) const noexcept { return args_[index]; }

		/*! Get an args begin iterator. */
		[[nodiscard]] iterator				 begin() noexcept { return args_.begin(); }

		/*! Get an args begin iterator. */
		[[nodiscard]] const_iterator		 begin() const noexcept { return args_.begin(); }

		/*! Get an args end iterator. */
		[[nodiscard]] iterator				 end() noexcept { return args_.end(); }

		/*! Get an args end iterator. */
		[[nodiscard]] const_iterator		 end() const noexcept { return args_.end(); }
	};

	/*! Represents an function. */
	// todo: do we really need another type function?
	class gal_function : public gal_expression
	{
	private:
		gal_prototype	prototype_;
		expression_type body_;

	public:
		gal_function(gal_prototype prototype, expression_type body)
			: prototype_(std::move(prototype)),
			  body_(std::move(body)) {}

		[[nodiscard]] constexpr e_expression_type get_type() const noexcept override { return e_expression_type::function_t; }
		[[nodiscard]] std::string				  to_string() const noexcept override { return "function"; }

		/*! Get function name. */
		[[nodiscard]] decltype(auto)			  get_function_name() const noexcept { return prototype_.get_prototype_name(); }

		// todo: function interface
	};
}// namespace gal

#endif//GAL_LANG_FUNCTION_HPP
