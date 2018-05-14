#include "connection.hpp"


#ifndef DISABLE_TESTS
#include <test/wrap_catch.hpp>
using namespace sig2;

TEST(signals_slots2, connection)
{
    int outer_fired = 0;
    int inner_fired = 0;
    int inner_fired2 = 0;
    Connection<> outer_disconnect;
    {
        Signal<> signal;

        signal.emit();
        {
            auto disconnect2 = signal.map([&inner_fired2]{ ++inner_fired2; });
            outer_disconnect = signal.map([&outer_fired]{ ++outer_fired; });
            auto disconnect = signal.map([&inner_fired]{ ++inner_fired; });
            signal.emit();
        }
        signal.emit();
    }

    EXPECT_EQ(1, inner_fired);
    EXPECT_EQ(1, inner_fired2);
    EXPECT_EQ(2, outer_fired);
}

TEST(signals_slots2, member_function)
{
    struct S
    {
        S(int a)
            : a(a)
        {
        }
        void foo(int a)
        {
            this->a = a;
        }
        int a;
    };
    Signal<S &, int> signal;
    auto disconnect = signal.map(&S::foo);
    S s((3));
    signal.emit(s, 4);
    EXPECT_EQ(4, s.a);
}

TEST(signals_slots2, moving)
{
    int num_fired = 0;
    Signal<> outer;
    {
        Connection<> disconnect;
        {
            Signal<> inner;
            disconnect = inner.map([&num_fired]{ ++num_fired; });
            outer = std::move(inner);
            inner.emit();
            EXPECT_EQ(0, num_fired);
            outer.emit();
            EXPECT_EQ(1, num_fired);
        }
        outer.emit();
        EXPECT_EQ(2, num_fired);
    }
    outer.emit();
    EXPECT_EQ(2, num_fired);
}

TEST(signals_slots2, mutable)
{
    int a = 0;
    int b = 0;
    Signal<int &> sig;
    auto disconnect = sig.map([a](int & b) mutable { b = ++a; });
    sig.emit(b);
    EXPECT_EQ(0, a);
    EXPECT_EQ(1, b);
    sig.emit(b);
    EXPECT_EQ(0, a);
    EXPECT_EQ(2, b);
}

TEST(signals_slots2, add_while_removing_self)
{
    int a = 0;
    int b = 0;
    Signal<> sig;
    sig.reserve(2);
    Connection<> inner_connection;
    Connection<> connection = sig.map([&]
    {
        ++a;
        connection.disconnect();
        inner_connection = sig.map([&]
        {
           ++b;
        });
    });
    sig.emit();
    REQUIRE(a == 1);
    REQUIRE(b == 0);
    sig.emit();
    REQUIRE(a == 1);
    REQUIRE(b == 1);
}

#include "memory/metrics.hpp"
TEST(signals_slots2, limit_signal_memory_growth_if_it_never_emits)
{
    Signal<> sig;
    sig.map([]{});
    size_t allocations_before = memory_metrics::allocations;
    size_t frees_before = memory_metrics::frees;
    for (size_t i = 0; i < sig.capacity() + 10; ++i)
        sig.map([]{});
    size_t allocations_after = memory_metrics::allocations;
    size_t frees_after = memory_metrics::frees;
    REQUIRE(allocations_after == allocations_before);
    REQUIRE(frees_after == frees_before);
}

TEST(signals_slots2, map)
{
    Signal<> sig;
    int a = 0;
    int c = 3;
    Connection<> connection = sig.map([a = 5]() mutable -> int
    {
        return ++a;
    }).map([&a](int b)
    {
        a = b;
    }).map([&c]
    {
        ++c;
    });
    sig.emit();
    REQUIRE(a == 6);
    REQUIRE(c == 4);
    sig.emit();
    REQUIRE(a == 7);
    REQUIRE(c == 5);
}

TEST(signals_slots2, take_n)
{
    Signal<> sig;
    int a = 0;
    Connection<> connection = take_n(sig.map([&a]{ ++a; }), 3);
    sig.emit();
    REQUIRE(a == 1);
    sig.emit();
    REQUIRE(a == 2);
    sig.emit();
    REQUIRE(a == 3);
    sig.emit();
    REQUIRE(a == 3);
}
TEST(signals_slots2, filter)
{
    Signal<int> sig;
    int a = 0;
    auto connection = sig.filter([](int n)
    {
        return n > 5 ? ConnectionFilter::Call : ConnectionFilter::Skip;
    }).map([&a](int n){ a += n; });
    sig.emit(5);
    REQUIRE(a == 0);
    sig.emit(6);
    REQUIRE(a == 6);
    sig.emit(4);
    REQUIRE(a == 6);
    sig.emit(7);
    REQUIRE(a == 13);
}
TEST(signals_slots2, filter_then_map)
{
    Signal<int> sig;
    int a = 0;
    auto connection = sig.filter([](int n)
    {
        return n > 5 ? ConnectionFilter::Call : ConnectionFilter::Skip;
    }).map([&a](int n){ a += n; });
    sig.emit(13);
    int b = 0;
    auto new_connection = connection.map([&b]{ return ++b; });
    sig.emit(5);
    REQUIRE(a == 13);
    REQUIRE(b == 0);
    sig.emit(6);
    REQUIRE(a == 19);
    REQUIRE(b == 1);
    int c = 0;
    auto mapped_again = new_connection.map([&c](int x){ c = x + 5; });
    sig.emit(7);
    REQUIRE(a == 26);
    REQUIRE(b == 2);
    REQUIRE(c == 7);
    sig.emit(1);
    REQUIRE(a == 26);
    REQUIRE(b == 2);
    REQUIRE(c == 7);
}
TEST(signals_slots2, map_then_filter)
{
    Signal<int> sig;
    int a = 0;
    auto connection = sig.map([](int n){ return 2 * n; }).filter([](int n)
    {
        return n > 5 ? ConnectionFilter::Call : ConnectionFilter::Skip;
    }).map([&a](int n){ a += n; });
    sig.emit(2);
    REQUIRE(a == 0);
    sig.emit(3);
    REQUIRE(a == 6);
    sig.emit(4);
    REQUIRE(a == 14);
    sig.emit(1);
    REQUIRE(a == 14);
}
TEST(signals_slots2, until)
{
    Signal<int> sig;
    Signal<> sig2;
    int a = 0;
    auto connection = until(sig, sig2).map([&a](int n){ a = n; });
    sig.emit(5);
    REQUIRE(a == 5);
    sig.emit(6);
    REQUIRE(a == 6);
    sig2.emit();
    sig.emit(7);
    REQUIRE(a == 6);
}

#include "memory/metrics.hpp"

TEST(signals_slots2, mouse_drag)
{
    RequireNoLeaks no_leaks;

    Signal<std::pair<int, int>> mouse_down;
    Signal<std::pair<int, int>> mouse_move;
    Signal<std::pair<int, int>> mouse_up;
    std::vector<std::pair<int, int>> path;
    auto mouse_down_connection = mouse_down.map([&, until_mouse_up = Connection<std::pair<int, int>>()](std::pair<int, int> begin_pos) mutable
    {
        path = { begin_pos };
        until_mouse_up = until(mouse_move, mouse_up).map([positions = path, &path](std::pair<int, int> next_pos) mutable
        {
            positions.push_back(next_pos);
            path = positions;
        });
    });
    mouse_move.emit({ 10, 10 });
    mouse_down.emit({ 10, 10 });
    mouse_move.emit({ 11, 10 });
    REQUIRE(path == (std::vector<std::pair<int, int>>{ { 10, 10 }, { 11, 10 } }));
    mouse_move.emit({ 12, 10 });
    mouse_up.emit({ 12, 10 });
    mouse_move.emit({ 13, 10 });
    REQUIRE(path == (std::vector<std::pair<int, int>>{ { 10, 10 }, { 11, 10 }, { 12, 10 } }));

    mouse_down.emit({ 13, 10 });
    REQUIRE(path == (std::vector<std::pair<int, int>>{ { 13, 10 } }));
    mouse_move.emit({ 14, 10 });
    REQUIRE(path == (std::vector<std::pair<int, int>>{ { 13, 10 }, { 14, 10 } }));
}

#endif
