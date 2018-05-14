#include "util/shared_ptr.hpp"

static_assert(sizeof(ptr::small_shared_ptr<int>) == sizeof(void *), "expecting the small_shared_ptr to be the size of just one pointer");

#ifndef DISABLE_TESTS
#include <test/wrap_catch.hpp>

TEST(small_shared_ptr, simple)
{
	struct DestructorCounter
	{
		DestructorCounter(int & count)
			: count(count)
		{
		}
		~DestructorCounter()
		{
			++count;
		}
		int & count;
	};

	int count = 0;
	{
		ptr::small_shared_ptr<DestructorCounter> a = ptr::make_shared<DestructorCounter>(count);
		ptr::small_shared_ptr<DestructorCounter> b = std::move(a);
		ptr::small_shared_ptr<DestructorCounter> c;
		{
			ptr::small_shared_ptr<DestructorCounter> d = b;
			c = d;
		}
        REQUIRE(count == 0);
	}
    REQUIRE(count == 1);
}

TEST(small_shared_ptr, get)
{
    ptr::small_shared_ptr<int> a;
    REQUIRE(!a.get());
    REQUIRE(!a);
    a = ptr::make_shared<int>(1);
    REQUIRE(a.get());
    REQUIRE(a);
}

struct ForwardDeclared;
struct ContainsForwardDeclared
{
    ~ContainsForwardDeclared();
    ptr::small_shared_ptr<ForwardDeclared> foo;
};

TEST(small_shared_ptr, forward_declared)
{
    ContainsForwardDeclared a;
}

struct ForwardDeclared
{
};

ContainsForwardDeclared::~ContainsForwardDeclared() = default;

TEST(small_shared_ptr, convert_to_const)
{
    ptr::small_shared_ptr<int> a(ptr::make_shared<int>(5));
    ptr::small_shared_ptr<const int> b = a;
}

#endif
