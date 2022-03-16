#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <utils/format.hpp>
#include <utils/source_location.hpp>
#include <gal/boxed_cast.hpp>
#include <gal/foundation/boxed_number.hpp>

using namespace gal::lang;
using namespace foundation;

template<typename T>
void consume(T) {}

template<typename To>
bool run_cast(const boxed_value& object, bool expected_pass, const std_source_location& location = std_source_location::current())
{
	auto result = [&object, expected_pass]
	{
		try { consume(boxed_cast<To>(object)); }
		catch (const exception::bad_boxed_cast& e)
		{
			if (expected_pass)
			{
				std::cerr << std_format::format("Failure when attempting cast object. reason: '{}' from '{}' to '{}'", e.what(), e.from.name(), e.to ? e.to->name() : "unknown") << '\n';
				return false;
			}
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << std_format::format("Unexpected std::exception when attempting cast object. reason: '{}'", e.what()) << '\n';
			return false;
		}
		catch (...)
		{
			std::cerr << "Unexpected unknown exception when attempting cast object." << '\n';
			return false;
		}

		return expected_pass;
	}();

	if (not result)
	{
		std::cerr << std_format::format(
				"Error with type conversion test in '(Line: {}, Function: {})'. "
				"From '{}({})' to '{}({})', "
				"Test was expected to '{}' but did not.\n",
				location.line(),
				location.function_name(),
				object.type_info().name(),
				object.is_const() ? "immutable" : "mutable",
				typeid(To).name(),
				std::is_const_v<To> ? "immutable" : "mutable",
				expected_pass ? "succeed" : "fail");
	}

	return result;
}

template<typename To>
bool do_cast(
		[[maybe_unused]] const boxed_value& object,
		[[maybe_unused]] const bool t,
		[[maybe_unused]] const bool const_t,
		[[maybe_unused]] const bool ref_t,
		[[maybe_unused]] const bool const_ref_t,
		[[maybe_unused]] const bool ptr_t,
		[[maybe_unused]] const bool const_ptr_t,
		[[maybe_unused]] const bool ptr_const_t,
		[[maybe_unused]] const bool ptr_const_ref_t,
		[[maybe_unused]] const bool const_ptr_const_t,
		[[maybe_unused]] const bool const_ptr_const_ref_t,
		[[maybe_unused]] const bool shared_ptr_t,
		[[maybe_unused]] const bool shared_const_ptr_t,
		[[maybe_unused]] const bool shared_ptr_ref_t,
		[[maybe_unused]] const bool const_shared_ptr_t,
		[[maybe_unused]] const bool const_shared_const_ptr_t,
		[[maybe_unused]] const bool const_shared_ptr_ref_t,
		[[maybe_unused]] const bool const_shared_ptr_const_ref_t,
		[[maybe_unused]] const bool wrapped_ref_t,
		[[maybe_unused]] const bool wrapped_const_ref_t,
		[[maybe_unused]] const bool const_wrapped_ref_t,
		[[maybe_unused]] const bool const_wrapped_const_ref_t,
		[[maybe_unused]] const bool const_wrapped_ref_ref_t,
		[[maybe_unused]] const bool const_wrapped_const_ref_ref_t,
		[[maybe_unused]] const bool number_t,
		[[maybe_unused]] const bool const_number_t,
		[[maybe_unused]] const bool const_number_ref_t
		)
{
	bool passed = true;

	passed &= run_cast<boxed_value>(object, true);
	passed &= run_cast<const boxed_value>(object, true);
	passed &= run_cast<const boxed_value&>(object, true);

	passed &= run_cast<To>(object, t);
	passed &= run_cast<const To>(object, const_t);
	// passed &= run_cast<To&>(object, ref_t);
	passed &= run_cast<const To&>(object, const_ref_t);
	// passed &= run_cast<To*>(object, ptr_t);
	passed &= run_cast<To*&>(object, false);
	passed &= run_cast<const To*>(object, const_ptr_t);
	passed &= run_cast<const To*&>(object, false);
	// passed &= run_cast<To* const&>(object, ptr_const_ref_t);
	// passed &= run_cast<To* const>(object, ptr_const_t);
	passed &= run_cast<const To* const>(object, const_ptr_const_t);
	// passed &= run_cast<const To* const&>(object, const_ptr_const_ref_t);
	// passed &= run_cast<std::shared_ptr<To>>(object, shared_ptr_t);
	// passed &= run_cast<std::shared_ptr<const To>>(object, shared_const_ptr_t);
	// passed &= run_cast<std::shared_ptr<To>&>(object, shared_ptr_ref_t);
	// passed &= run_cast<const std::shared_ptr<To>>(object, const_shared_ptr_t);
	// passed &= run_cast<const std::shared_ptr<const To>>(object, const_shared_const_ptr_t);
	// passed &= run_cast<const std::shared_ptr<To>&>(object, const_shared_ptr_ref_t);
	// passed &= run_cast<const std::shared_ptr<const To>&>(object, const_shared_ptr_const_ref_t);
	// passed &= run_cast<std::reference_wrapper<To>>(object, wrapped_ref_t);
	passed &= run_cast<std::reference_wrapper<const To>>(object, wrapped_const_ref_t);
	passed &= run_cast<std::reference_wrapper<To>&>(object, false);
	passed &= run_cast<std::reference_wrapper<const To>&>(object, false);
	// passed &= run_cast<const std::reference_wrapper<To>>(object, const_wrapped_ref_t);
	passed &= run_cast<const std::reference_wrapper<const To>>(object, const_wrapped_const_ref_t);
	// passed &= run_cast<const std::reference_wrapper<To>&>(object, const_wrapped_ref_ref_t);
	passed &= run_cast<const std::reference_wrapper<const To>&>(object, const_wrapped_const_ref_ref_t);
	// passed &= run_cast<boxed_number>(object, number_t);
	// passed &= run_cast<const boxed_number>(object, const_number_t);
	// passed &= run_cast<boxed_number&>(object, false);
	// passed &= run_cast<const boxed_number&>(object, const_number_ref_t);
	// passed &= run_cast<boxed_number*>(object, false);
	passed &= run_cast<const boxed_number*>(object, false);
	passed &= run_cast<boxed_number*const>(object, false);
	passed &= run_cast<const boxed_number*const>(object, false);
	passed &= run_cast<const boxed_number*const>(object, false);

	return passed;
}

template<typename To>
bool built_in_type_test(const To& initial, const bool is_pod)
{
	bool passed = true;

	/** value tests **/
	auto i = initial;
	passed &= do_cast<To>(
			var(i),
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			const_var(i),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			false,
			true,
			true,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			var(i),
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			true,
			true,
			true,
			true,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			const_var(&i),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			var(std::ref(i)),
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			true,
			true,
			true,
			true,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			var(std::cref(i)),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod);

	/** Const Reference Variable tests */

	// This reference will be copied on input, which is expected
	const auto& ir = i;
	passed &= do_cast<To>(
			var(ir),
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			is_pod,
			is_pod,
			is_pod);
	// But a pointer or reference to it should be necessarily const
	passed &= do_cast<To>(
			var(&ir),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod);
	passed &= do_cast<To>(
			var(std::ref(ir)),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);
	// Make sure const of const works too
	passed &= do_cast<To>(
			const_var(&ir),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);
	passed &= do_cast<To>(
			const_var(std::ref(ir)),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);

	/** Const Reference Variable tests */

	// This will always be seen as a const
	const auto* cip = &i;
	passed &= do_cast<To>(
			var(cip),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);
	// make sure const of const works
	passed &= do_cast<To>(
			const_var(cip),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);

	/** shared_ptr tests **/

	auto ip = std::make_shared<To>(initial);
	passed &= do_cast<To>(
			var(ip),
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			true,
			is_pod,
			is_pod,
			is_pod
			);
	passed &= do_cast<To>(
			const_var(ip),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			true,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);

	/** const shared_ptr tests **/
	auto ipc = std::make_shared<const To>(initial);
	passed &= do_cast<To>(
			var(ipc),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			true,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod
			);
	// const of this should be the same, making sure it compiles
	passed &= do_cast<To>(
			const_var(ipc),
			true,
			true,
			false,
			true,
			false,
			true,
			false,
			false,
			true,
			true,
			false,
			true,
			false,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			false,
			true,
			is_pod,
			is_pod,
			is_pod);

	return passed;
}

template<typename To>
bool pointer_test(const To& initial, const To& new_value)
{
	auto up = std::make_unique<To>(initial);
	auto p = &*up;


	// we store a pointer to a pointer, so we can get a pointer to a pointer
	try
	{
		auto** result = boxed_cast<To**>(var(&p));
		**result = new_value;

		if (p != *result)
		{
			std::cerr << "Pointer passed in different than one returned\n";
			return false;
		}

		if (*p != **result)
		{
			std::cerr << "Somehow de-referenced pointer values are not the same?\n";
			return false;
		}

		return true;
	}
	catch (const exception::bad_boxed_cast&)
	{
		std::cerr << "Bad boxed cast performing ** to ** test\n";
		return false;
	}
	catch (...)
	{
		std::cerr << "Unknown exception performing ** to ** test\n";
		return false;
	}
}

constexpr auto test_boxed_cast_out_filename = "boxed_cast.log";

TEST(TestBoxedCast, TestBuiltInType)
{
	const std::ofstream file{test_boxed_cast_out_filename};
	auto* prev_rdbuf = std::cerr.rdbuf();
	std::cerr.set_rdbuf(file.rdbuf());

	ASSERT_TRUE(built_in_type_test(false, true));
	ASSERT_TRUE(built_in_type_test(42, true));
	ASSERT_TRUE(built_in_type_test(42u, true));
	ASSERT_TRUE(built_in_type_test(42l, true));
	ASSERT_TRUE(built_in_type_test(42ll, true));
	ASSERT_TRUE(built_in_type_test(42ull, true));
	ASSERT_TRUE(built_in_type_test(42.f, true));
	ASSERT_TRUE(built_in_type_test(42.0, true));
	ASSERT_TRUE(built_in_type_test('a', true));
	ASSERT_TRUE(built_in_type_test(u8'a', true));
	ASSERT_TRUE(built_in_type_test(L'a', true));
	ASSERT_TRUE(built_in_type_test(u'a', true));
	ASSERT_TRUE(built_in_type_test(U'a', true));
	ASSERT_TRUE(built_in_type_test(std::string{"hello world"}, true));

	std::cerr.set_rdbuf(prev_rdbuf);
}

TEST(TestBoxedCast, TestPointer)
{
	const std::ofstream file{test_boxed_cast_out_filename};
	auto* prev_rdbuf = std::cerr.rdbuf();
	std::cerr.set_rdbuf(file.rdbuf());

	ASSERT_TRUE(pointer_test(false, true));
	ASSERT_TRUE(pointer_test(42, 123));
	ASSERT_TRUE(pointer_test(42u, 123u));
	ASSERT_TRUE(pointer_test(42l, 123l));
	ASSERT_TRUE(pointer_test(42ll, 123ll));
	ASSERT_TRUE(pointer_test(42ull, 123ull));
	ASSERT_TRUE(pointer_test(42.f, 123.f));
	ASSERT_TRUE(pointer_test(42.0, 123.0));
	ASSERT_TRUE(pointer_test('a', 'z'));
	ASSERT_TRUE(pointer_test(u8'a', u8'z'));
	ASSERT_TRUE(pointer_test(L'a', L'z'));
	ASSERT_TRUE(pointer_test(u'a', u'z'));
	ASSERT_TRUE(pointer_test(U'a', U'z'));
	ASSERT_TRUE(pointer_test(std::string{"hello world"}, std::string{"Hello GAL"}));

	std::cerr.set_rdbuf(prev_rdbuf);
}
