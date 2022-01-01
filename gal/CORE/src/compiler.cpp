#include <compiler/compiler.hpp>
#include <ast/common.hpp>
#include <variant>
#include <utils/hash_container.hpp>
#include <ast/parser.hpp>

namespace gal::compiler
{
	class compiler
	{
	public:
		struct constant
		{
			using null_type = ast::gal_null_type;
			using boolean_type = ast::gal_boolean_type;
			using number_type = ast::gal_number_type;
			using string_type = ast::gal_string_type;

			using constant_type = std::variant<
				null_type,
				boolean_type,
				number_type,
				string_type>;

			constant_type data;

			constexpr explicit operator bool() const noexcept { return not std::holds_alternative<null_type>(data) && not(std::holds_alternative<boolean_type>(data) && std::get<boolean_type>(data) == false); }
		};

		struct register_scope
		{
			compiler* self;
		};

	private:
		struct function {};

		struct local {};

		struct global {};

		bytecode_builder& bytecode_;

		compile_options options_;

		utils::hash_map<ast::ast_expression_function*, function> functions_;
		utils::hash_map<ast::ast_local*, local> locals_;
		utils::hash_map<ast::ast_name, global> globals_;
		utils::hash_map<ast::ast_expression*, constant> constants_;
		utils::hash_map<ast::ast_expression_table*, std::pair<std::size_t, std::size_t>> predicted_table_size_;
	};
}
