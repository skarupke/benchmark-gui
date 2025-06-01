#include "assert_settings.hpp"
#include <iostream>

namespace assert
{
namespace
{
struct AssertCallbackStorage
{
	AssertCallbackStorage()
		: assert_callback([](const AssertContext & context)
	{
		std::cerr << "Assertion failed in file \"" << context.file << "\" line " << context.line <<":\n" << context.condition << std::endl;
		return ShouldBreak;
	})
	{
	}

	func::function<AssertBreakPoint (const AssertContext &)> assert_callback;
};
}
static AssertCallbackStorage & assert_callback_storage()
{
	static AssertCallbackStorage storage;
	return storage;
}

func::function<AssertBreakPoint (const AssertContext &)> SetAssertCallback(func::function<AssertBreakPoint (const AssertContext &)> callback)
{
	using std::swap;
	swap(assert_callback_storage().assert_callback, callback);
	return callback;
}
const func::function<AssertBreakPoint (const AssertContext &)> & GetAssertCallback()
{
	return assert_callback_storage().assert_callback;
}
}

#ifndef DISABLE_TESTS
#include <test/include_test.hpp>

static std::string escape_for_regex(const char * message)
{
	std::string result;
	for (const char * c = message; c && *c; ++c)
	{
		switch(*c)
		{
		case '.':
			result.push_back('\\');
			result.push_back('.');
			break;
		case '\\':
			result.push_back('\\');
			result.push_back('\\');
			break;
		default:
			result.push_back(*c);
			break;
		}
	}
	return result;
}

TEST(assert_settingsDeathTest, SLOW_TEST(default_crashes))
{
	bool assert_condition = true;
    auto dying_function = [&]{ RAW_ASSERT(!assert_condition); };
	ASSERT_DEATH(dying_function(), escape_for_regex(__FILE__ ) + ".*" + std::to_string(__LINE__ - 1) + ".*!assert_condition.*");
}

TEST(assert_settings, disable_breakpoint)
{
	bool did_assert = false;
	::assert::ScopedSetAssertCallback change_callback([&](const ::assert::AssertContext &)
	{
		did_assert = true;
		return ::assert::ShouldNotBreak;
	});
    RAW_ASSERT(false);
	ASSERT_TRUE(did_assert);
}

#endif
