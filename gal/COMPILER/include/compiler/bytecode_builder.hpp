#pragma once

#ifndef GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP
#define GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP

#include <ast/common.hpp>
#include <compiler/operand_codes.hpp>
#include <utils/hash_container.hpp>
#include <utils/hash.hpp>
#include <utils/string_utils.hpp>
#include <utils/assert.hpp>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <variant>

namespace gal::compiler
{
	class bytecode_encoder
	{
	public:
		constexpr bytecode_encoder() = default;

		constexpr bytecode_encoder(const bytecode_encoder&) = delete;
		constexpr bytecode_encoder& operator=(const bytecode_encoder&) = delete;
		constexpr bytecode_encoder(bytecode_encoder&&) = delete;
		constexpr bytecode_encoder& operator=(bytecode_encoder&&) = delete;

		constexpr virtual ~bytecode_encoder() = 0;

		virtual std::uint8_t encode_operand(std::uint8_t operand) = 0;
	};

	class bytecode_builder
	{
	public:
		using index_type = std::size_t;
		using id_type = std::uint32_t;
		using string_ref_type = std::string_view;

		struct table_shape
		{
			constexpr static auto key_max_size = 32;

			std::array<std::int32_t, key_max_size> keys;
		};

		enum class dump_flags : std::uint16_t
		{
			code = 1 << 0,
			line = 1 << 1,
			source = 1 << 2,
			locals = 1 << 3,
		};

	private:
		struct table_shape_hasher
		{
			constexpr std::size_t operator()(const table_shape& t) const noexcept { return utils::hash(t.keys); }
		};

		struct constant
		{
			using null_type = ast::gal_null_type;
			using boolean_type = ast::gal_boolean_type;
			using number_type = ast::gal_number_type;
			using string_type = std::size_t;
			using import_type = operand_underlying_type;
			using table_type = std::size_t;
			using closure_type = std::size_t;

			std::variant<
				ast::gal_null_type,
				ast::gal_boolean_type,
				ast::gal_number_type,
				// see operand::load_import (2 + 3 * 10)
				operand_underlying_type,
				/**
				 * string: index into string table
				 * table: index into bytecode_encoder.table_shapes_
				 * closure: index of function in global list
				 */
				std::size_t> data;

			friend constexpr bool operator==(const constant& lhs, const constant& rhs) noexcept { return lhs.data == rhs.data; }

			template<typename T>
				requires utils::is_any_type_of_v<T,
				                                 boolean_type,
				                                 number_type,
				                                 string_type,
				                                 import_type,
				                                 table_type,
				                                 closure_type>
			constexpr decltype(auto) get() noexcept
			{
				gal_assert(std::holds_alternative<T>(data));
				return std::get<T>(data);
			}

			template<typename T>
				requires utils::is_any_type_of_v<T,
				                                 boolean_type,
				                                 number_type,
				                                 string_type,
				                                 import_type,
				                                 table_type,
				                                 closure_type>
			constexpr decltype(auto) get() const noexcept
			{
				gal_assert(std::holds_alternative<T>(data));
				return std::as_const(std::get<T>(data));
			}
		};

		struct constant_hasher
		{
			constexpr std::size_t operator()(const constant& c) const noexcept { return std::hash<decltype(constant::data)>{}(c.data); }
		};

		struct function
		{
			std::string data;

			operand_abc_underlying_type max_stack_size{0};
			operand_abc_underlying_type num_params{0};
			operand_abc_underlying_type num_upvalues{0};
			bool is_vararg{false};

			index_type debug_name_index{0};

			std::string dump;
			std::string dump_name;
		};

		struct debug_local
		{
			index_type name;

			std::uint8_t reg;
			std::uint32_t begin_pc;
			std::uint32_t end_pc;
		};

		struct debug_upvalue
		{
			index_type name;
		};

		struct jump
		{
			std::uint32_t source;
			std::uint32_t target;
		};

		std::vector<function> functions_;
		std::uint32_t current_function;
		std::uint32_t main_function;

		std::vector<operand_underlying_type> instructions_;
		std::vector<int> lines_;
		std::vector<constant> constants_;
		std::vector<std::uint32_t> protos_;
		std::vector<jump> jumps_;

		std::vector<table_shape> table_shapes_;

		utils::hash_map<constant, std::int32_t, constant_hasher> constant_map_;
		utils::hash_map<table_shape, std::int32_t, table_shape_hasher> table_shape_map_;

		int debug_line_;
		std::vector<debug_local> debug_locals_;
		std::vector<debug_upvalue> debug_upvalues_;

		utils::hash_map<ast::gal_string_type, std::size_t> string_tables_;

		bytecode_encoder* encoder_;
		std::string bytecode_;

		// padding with dump flag
		bool has_long_jump_;

		std::underlying_type_t<dump_flags> dump_flags_;
		std::vector<std::string> dump_source_;
		using dump_handler_type = std::string (bytecode_builder::*)() const;
		dump_handler_type dump_handler_;

		void validate() const;

		std::string dump_current_function() const;
		const std::uint32_t* dump_instruction(const std::uint32_t* operand, std::string& output) const;

		void write_function(std::string& str, id_type id) const;
		void write_line_info(std::string& str) const;
		void write_string_table(std::string& str) const;

		std::int32_t add_constant(const constant& key, const constant& value);
		std::uint32_t add_string_table_entry(ast::gal_string_type value);

	public:
		explicit bytecode_builder(bytecode_encoder* encoder = nullptr)
			: current_function{~decltype(current_function){0}},
			  main_function{~decltype(main_function){0}},
			  debug_line_{0},
			  encoder_{encoder},
			  has_long_jump_{false},
			  dump_flags_{0},
			  dump_handler_{nullptr} { }

		std::uint32_t begin_function(std::uint8_t num_params, bool is_vararg = false);
		void end_function(std::uint8_t max_stack_size, std::uint8_t num_upvalues);

		void set_main_function(decltype(main_function) function_id);

		std::int32_t add_constant_null();
		std::int32_t add_constant_boolean(ast::gal_boolean_type value);
		std::int32_t add_constant_number(ast::gal_number_type value);
		std::int32_t add_constant_string(ast::gal_string_type value);
		std::int32_t add_import(id_type import_id);
		std::int32_t add_constant_table(const table_shape& shape);
		std::int32_t add_constant_closure(id_type function_id);

		std::int16_t add_child_function(id_type function_id);

		void emit_operand_abc(operands operand, std::uint8_t a, std::uint8_t b, std::uint8_t c);
		void emit_operand_ad(operands operand, std::uint8_t a, std::int16_t d);
		void emit_operand_e(operands operand, std::int32_t e);
		void emit_operand_aux(std::uint32_t aux);

		std::size_t emit_label();

		[[nodiscard]] bool patch_jump_d(std::size_t jump_label, std::size_t target_label);
		[[nodiscard]] bool patch_skip_c(std::size_t jump_label, std::size_t target_label);

		void fold_jumps();
		void expand_jumps();

		void set_debug_function_name(string_ref_type name);
		void set_debug_line(int line);
		void push_debug_local(string_ref_type name, std::uint8_t reg, std::uint32_t begin_pc, std::uint32_t end_pc);
		void push_debug_upvalue(string_ref_type name);
		std::uint32_t get_debug_pc() const;

		void finalize();

		void set_dump_source(const std::string& source);

		const std::string& get_bytecode() const noexcept
		{
			gal_assert(not bytecode_.empty(), "did you forget to call finalize?");
			return bytecode_;
		}

		std::string dump_function(id_type id) const;
		std::string dump_everything() const;

		constexpr static id_type get_import_id(const id_type id0) noexcept
		{
			gal_assert(id0 < 1024);

			return (1 << 30) | (id0 << 20);
		}

		constexpr static id_type get_import_id(const id_type id0, const id_type id1) noexcept
		{
			gal_assert((id0 | id1) < 1024);

			return (2 << 30) | (id0 << 20) | (id1 << 10);
		}

		constexpr static id_type get_import_id(const id_type id0, const id_type id1, const id_type id2) noexcept { return (3 << 30) | (id0 << 20) || (id1 << 10) | (id2); }
	};
}

#endif // GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP
