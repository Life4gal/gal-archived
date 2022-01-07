#pragma once

#ifndef GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP
#define GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP

#include <ast/common.hpp>
#include <compiler/operand_codes.hpp>
#include <utils/hash_container.hpp>
#include <utils/hash.hpp>
#include <utils/assert.hpp>
#include <string>
#include <array>
#include <vector>
#include <variant>

namespace gal::compiler
{
	class bytecode_encoder
	{
	public:
		using encoder_require_type = operand_underlying_type;

		constexpr bytecode_encoder() = default;

		constexpr bytecode_encoder(const bytecode_encoder&) = delete;
		constexpr bytecode_encoder& operator=(const bytecode_encoder&) = delete;
		constexpr bytecode_encoder(bytecode_encoder&&) = delete;
		constexpr bytecode_encoder& operator=(bytecode_encoder&&) = delete;

		constexpr virtual ~bytecode_encoder() = 0;

		virtual encoder_require_type encode_operand(encoder_require_type operand) = 0;
	};

	class bytecode_builder
	{
	public:
		using index_type = std::uint32_t;
		using signed_index_type = std::int32_t;
		using function_id_type = std::uint32_t;
		using string_ref_type = std::string_view;
		using label_type = std::size_t;
		using label_offset_type = std::make_signed_t<label_type>;
		using debug_pc_type = index_type;
		using register_type = operand_abc_underlying_type;

		constexpr static signed_index_type constant_too_many_index = -1;

		constexpr static index_type max_constant_size = 1 << 23;
		constexpr static index_type max_closure_size = 1 << 15;
		constexpr static int max_jump_distance = 1 << 23;

		struct table_shape
		{
			constexpr static auto key_max_size = 32;

			// todo
			using data_type = std::int32_t;
			std::array<data_type, key_max_size> keys;
			decltype(keys.size()) length;

			[[nodiscard]] friend constexpr bool operator==(const table_shape& lhs, const table_shape& rhs) noexcept { return lhs.length == rhs.length && std::memcmp(lhs.keys.data(), rhs.keys.data(), lhs.length) == 0; }

			GAL_ASSERT_CONSTEXPR void append(const data_type data) noexcept
			{
				gal_assert(length + 1 < key_max_size);
				keys[length++] = data;
			}
		};

		enum class dump_flags : std::uint16_t
		{
			code = 1 << 0,
			line = 1 << 1,
			source = 1 << 2,
			locals = 1 << 3,
		};

		struct table_shape_hasher
		{
			constexpr std::size_t operator()(const table_shape& t) const noexcept { return utils::fnv1a_hash(t.keys); }
		};

		struct constant
		{
			using null_type = ast::gal_null_type;
			using boolean_type = ast::gal_boolean_type;
			using number_type = ast::gal_number_type;
			using string_type = index_type;
			using import_type = function_id_type;
			using table_type = index_type;
			using closure_type = function_id_type;

			using constant_type = std::variant<
				null_type,
				boolean_type,
				number_type,
				// string: index into string table
				string_type,
				// see operand::load_import (2 + 3 * 10)
				import_type,
				// table: index into bytecode_encoder.table_shapes_
				table_type,
				// closure: index of function in global list
				closure_type>;

			constexpr static std::size_t null_index = 0;
			constexpr static std::size_t boolean_index = 1;
			constexpr static std::size_t number_index = 2;
			constexpr static std::size_t string_index = 3;
			constexpr static std::size_t import_index = 4;
			constexpr static std::size_t table_index = 5;
			constexpr static std::size_t closure_index = 6;

			constant_type data;

			friend constexpr bool operator==(const constant& lhs, const constant& rhs) noexcept { return lhs.data == rhs.data; }

			template<std::size_t Index>
				requires(null_index <= Index && Index <= closure_index)
			[[nodiscard]] constexpr decltype(auto) get() noexcept { return std::get<Index>(data); }

			template<std::size_t Index>
				requires(null_index <= Index && Index <= closure_index)
			[[nodiscard]] constexpr decltype(auto) get() const noexcept { return std::get<Index>(data); }

			template<std::size_t Index>
				requires(null_index <= Index && Index <= closure_index)
			[[nodiscard]] constexpr auto get_if() noexcept { return std::get_if<Index>(&data); }

			template<std::size_t Index>
				requires(null_index <= Index && Index <= closure_index)
			[[nodiscard]] constexpr auto get_if() const noexcept { return std::get_if<Index>(&data); }

			/*
			 * @brief Unfortunately, we have duplicate types and are not allowed to use visit (because the correct match cannot be obtained), so we can only check by ourselves.
			 */
			// template<typename Visitor>
			// constexpr decltype(auto) visit(Visitor visitor) { return std::visit(visitor, data); }
			// template<typename Visitor>
			// constexpr decltype(auto) visit(Visitor visitor) const { return std::visit(visitor, data); }
		};

		struct constant_hasher
		{
			[[nodiscard]] std::size_t operator()(const constant& c) const noexcept { return std::hash<decltype(constant::data)>{}(c.data); }
		};

	private:
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

			register_type reg;
			debug_pc_type begin_pc;
			debug_pc_type end_pc;
		};

		struct debug_upvalue
		{
			index_type name;
		};

		struct jump
		{
			label_type source : 32;
			label_type target : 32;

			[[nodiscard]] constexpr label_offset_type distance() const noexcept { return static_cast<label_offset_type>(target) - static_cast<label_offset_type>(source) - 1; }
		};

		std::vector<function> functions_;
		function_id_type current_function_;
		function_id_type main_function_;

		std::vector<operand_underlying_type> instructions_;
		std::vector<int> lines_;
		std::vector<constant> constants_;
		std::vector<function_id_type> protos_;
		std::vector<jump> jumps_;

		std::vector<table_shape> table_shapes_;

		utils::hash_map<constant, signed_index_type, constant_hasher> constant_map_;
		utils::hash_map<table_shape, signed_index_type, table_shape_hasher> table_shape_map_;

		int debug_line_;
		std::vector<debug_local> debug_locals_;
		std::vector<debug_upvalue> debug_upvalues_;

		utils::hash_map<string_ref_type, index_type> string_tables_;

		bytecode_encoder* encoder_;
		std::string bytecode_;

		// padding with dump flag
		bool has_long_jump_;

		std::underlying_type_t<dump_flags> dump_flags_;
		std::vector<std::string> dump_source_;
		using dump_handler_type = std::string (bytecode_builder::*)() const;
		dump_handler_type dump_handler_;

		void validate() const;

		[[nodiscard]] std::string dump_current_function() const;
		const operand_underlying_type* dump_instruction(const operand_underlying_type* code, std::string& output) const;

		void write_function(std::string& str, function_id_type id) const;
		void write_line_info(std::string& str) const;
		void write_string_table(std::string& str) const;

		signed_index_type add_constant(const constant& key, const constant& value);
		index_type add_string_table_entry(string_ref_type value);

	public:
		explicit bytecode_builder(bytecode_encoder* encoder = nullptr)
			: current_function_{~decltype(current_function_){0}},
			  main_function_{~decltype(main_function_){0}},
			  debug_line_{0},
			  encoder_{encoder},
			  has_long_jump_{false},
			  dump_flags_{0},
			  dump_handler_{nullptr} { }

		function_id_type begin_function(operand_abc_underlying_type num_params, bool is_vararg = false);
		void end_function(operand_abc_underlying_type max_stack_size, operand_abc_underlying_type num_upvalues);

		void set_main_function(function_id_type function_id);

		signed_index_type add_constant_null();
		signed_index_type add_constant_boolean(constant::boolean_type value);
		signed_index_type add_constant_number(constant::number_type value);
		signed_index_type add_constant_string(string_ref_type value);
		signed_index_type add_import(constant::import_type import_id);
		signed_index_type add_constant_table(const table_shape& shape);
		signed_index_type add_constant_closure(constant::closure_type function_id);

		signed_index_type add_child_function(function_id_type function_id);

		constexpr void emit_operand_abc(operands operand, operand_abc_underlying_type a, operand_abc_underlying_type b, operand_abc_underlying_type c);
		constexpr void emit_operand_ad(operands operand, operand_abc_underlying_type a, operand_d_underlying_type d);
		constexpr void emit_operand_e(operands operand, operand_e_underlying_type e);
		constexpr void emit_operand_aux(operand_aux_underlying_type aux);

		[[nodiscard]] constexpr label_type emit_label() const noexcept;

		[[nodiscard]] bool patch_jump_d(label_type jump_label, label_type target_label);
		[[nodiscard]] bool patch_skip_c(label_type jump_label, label_type target_label);

		void fold_jumps();
		void expand_jumps();

		void set_debug_function_name(string_ref_type name);
		void set_debug_line(const int line) noexcept { debug_line_ = line; }
		void push_debug_local(string_ref_type name, register_type reg, debug_pc_type begin_pc, debug_pc_type end_pc);
		void push_debug_upvalue(string_ref_type name);
		[[nodiscard]] constexpr debug_pc_type get_debug_pc() const noexcept;

		void finalize();

		constexpr void set_dump_flags(std::underlying_type_t<dump_flags> flags)
		{
			dump_flags_ = flags;
			dump_handler_ = &bytecode_builder::dump_current_function;
		}

		void set_dump_source(const std::string& source);

		[[nodiscard]] GAL_ASSERT_CONSTEXPR const std::string& get_bytecode() const noexcept
		{
			gal_assert(not bytecode_.empty(), "did you forget to call finalize?");
			return bytecode_;
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR auto&& move_bytecode() noexcept
		{
			gal_assert(not bytecode_.empty(), "did you forget to call finalize?");
			return std::move(bytecode_);
		}

		[[nodiscard]] GAL_ASSERT_CONSTEXPR std::string dump_function(const function_id_type id) const
		{
			gal_assert(id < static_cast<function_id_type>(functions_.size()));

			return functions_[id].dump;
		}

		[[nodiscard]] std::string dump_everything() const;

		GAL_ASSERT_CONSTEXPR static function_id_type get_import_id(const function_id_type id0) noexcept
		{
			gal_assert(id0 < 1024);

			return (static_cast<function_id_type>(1) << 30) | (id0 << 20);
		}

		GAL_ASSERT_CONSTEXPR static function_id_type get_import_id(const function_id_type id0, const function_id_type id1) noexcept
		{
			gal_assert((id0 | id1) < 1024);

			return (static_cast<function_id_type>(2) << 30) | (id0 << 20) | (id1 << 10);
		}

		GAL_ASSERT_CONSTEXPR static function_id_type get_import_id(const function_id_type id0, const function_id_type id1, const function_id_type id2) noexcept
		{
			gal_assert((id0 | id1 | id2) < 1024);

			return (static_cast<function_id_type>(3) << 30) | (id0 << 20) || (id1 << 10) | (id2);
		}
	};
}

#endif // GAL_LANG_COMPILER_BYTECODE_BUILDER_HPP
