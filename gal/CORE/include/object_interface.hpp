#pragma once

#ifndef GAL_LANG_OBJECT_INTERFACE_HPP
#define GAL_LANG_OBJECT_INTERFACE_HPP

#include<type_traits>

namespace gal::lang::traits
{
	template<typename HostClassType>
	struct object_life_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_life_interface<host_class_type>;

		using allocate_type = std::false_type;
		using deallocate_type = std::false_type;

		using construct_type = std::false_type;
		using initial_type = std::false_type;
		using finalize_type = std::false_type;
		using clear_type = std::false_type;
		using destroy_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_traverse_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_traverse_interface<host_class_type>;

		using traverse_type = std::false_type;
	};

	template<typename HostClassType, gal_size_type DummyId>
	struct object_inquire_interface
	{
		using host_class_type = HostClassType;
		constexpr static gal_size_type dummy_id = DummyId;
		using interface_type = object_inquire_interface<host_class_type, dummy_id>;

		using inquire_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_length_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_length_interface<host_class_type>;

		using length_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_represent_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_represent_interface<host_class_type>;

		using represent_type = std::false_type;
		using string_type = represent_type;
	};

	template<typename HostClassType>
	struct object_hash_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_hash_interface<host_class_type>;

		using hash_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_compare_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_compare_interface<host_class_type>;

		using compare_type = std::false_type;

		enum class compare_operand
		{
			equal,
			not_equal,
			less,
			less_equal,
			greater,
			greater_equal
		};
	};

	template<typename HostClassType>
	struct object_iteration_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_iteration_interface<host_class_type>;

		using iteration_begin_type = std::false_type;
		using iteration_end_type = std::false_type;
		using iteration_next_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_descriptor_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_descriptor_interface<host_class_type>;

		using descriptor_get_type = std::false_type;
		using descriptor_set_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_invoke_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_invoke_interface<host_class_type>;

		using invoke_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_attribute_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_attribute_interface<host_class_type>;

		using name_get_type = std::false_type;
		using name_set_type = std::false_type;

		using object_get_type = std::false_type;
		using object_set_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_mathematical_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_mathematical_interface<host_class_type>;

		using plus_type = std::false_type;
		using minus_type = std::false_type;
		using multiply_type = std::false_type;
		using floor_divide_type = std::false_type;
		using real_divide_type = std::false_type;
		using divide_modulus_type = std::false_type;
		using remainder_type = std::false_type;
		using power_type = std::false_type;
		using negative_type = std::false_type;
		using positive_type = std::false_type;
		using absolute_type = std::false_type;
		using invert_type = std::false_type;
		using bit_left_shift_type = std::false_type;
		using bit_right_shift_type = std::false_type;
		using bit_and_type = std::false_type;
		using bit_or_type = std::false_type;
		using to_boolean = std::false_type;
		using to_integer = std::false_type;
		using to_float = std::false_type;
		using plus_assign = std::false_type;
		using minus_assign = std::false_type;
		using multiply_assign = std::false_type;
		using floor_divide_assign = std::false_type;
		using real_divide_assign = std::false_type;
		using remainder_assign = std::false_type;
		using power_assign = std::false_type;
		using bit_left_shift_assign = std::false_type;
		using bit_right_shift_assign = std::false_type;
		using bit_and_assign = std::false_type;
		using bit_or_assign = std::false_type;
		using bit_xor_assign = std::false_type;
	};

	template<typename HostClassType>
	struct object_sequence_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_sequence_interface<host_class_type>;

		using length_type = std::false_type;
		using concat_type = std::false_type;
		using repeat_type = std::false_type;
		using item_get_type = std::false_type;
		using item_set_type = std::false_type;
		using item_contains = std::false_type;
		using concat_assign_type = std::false_type;
		using repeat_assign_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_mapping_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_mapping_interface<host_class_type>;

		using length_type = std::false_type;
		using item_get_type = std::false_type;
		using item_set_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_async_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_async_interface<host_class_type>;

		using await_type = std::false_type;
		using iterator_type = std::false_type;
		using next_type = std::false_type;
	};

	template<typename HostClassType>
	struct object_buffer_interface
	{
		using host_class_type = HostClassType;
		using interface_type = object_buffer_interface<host_class_type>;

		using get_type = std::false_type;
		using release_type = std::false_type;
	};
}

#endif // GAL_LANG_OBJECT_INTERFACE_HPP
