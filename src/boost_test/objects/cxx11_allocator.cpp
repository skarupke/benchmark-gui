#include "boost_test/objects/cxx11_allocator.hpp"
#include "boost_test/objects/exception.hpp"
#include "boost_test/objects/test.hpp"

namespace std
{
template class std::allocator<test::object>;
template class std::allocator<int>;
}
namespace test
{
// no idea why I need to explicitly instantiate this.
// I get linker errors otherwise. which I think is an error in GCC somehow
template struct cxx11_allocator<object, select_copy, void>;
template struct cxx11_allocator<object, no_select_copy, void>;
template struct cxx11_allocator<object, propagate_move, void>;
template struct cxx11_allocator<object, no_propagate_move, void>;
template class allocator1<object>;
template class allocator2<movable>;
template class allocator2<object>;
namespace exception
{
template class allocator2<object>;
}
}

