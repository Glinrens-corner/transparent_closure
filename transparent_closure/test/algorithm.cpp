#include "doctest/doctest.h"
#include "algorithm.hpp"
#include <array>


TEST_CASE("IteratorStack"){
  using transparent_closure::IteratorStack;
  SUBCASE("basic test"){
    IteratorStack stack{};
    
    REQUIRE(stack.get_size()==0 );
    new (stack.get_new<int>()) int{5};
    CHECK(stack.get_size() >= sizeof(int));
    CHECK( stack.get_last<int>()== 5);
    stack.pop_last<int>();
    CHECK(stack.get_size() == 0);
  };
  SUBCASE("multiple"){
    IteratorStack stack{};

    new (stack.get_new<long>()) long{5};
    new (stack.get_new<int>()) int{4};
    new (stack.get_new<short>()) short{3};
    CHECK( stack.get_last<short>()== 3);
    stack.pop_last<short>();
    CHECK( stack.get_last<int>()== 4);
    stack.pop_last<int>();
    CHECK( stack.get_last<long>()== 5);
    stack.pop_last<long>();
  };
  
  SUBCASE("basic test"){
    IteratorStack stack{};
    
    constexpr std::size_t stack_init_max_size = IteratorStack::get_init_max_size();
    using array_t = std::array<char , stack_init_max_size>;
    new (stack.get_new<int>()) int{4};
    stack.get_new<array_t>();
    CHECK( stack.get_size()>=(stack_init_max_size+sizeof(int)));
    new (stack.get_new<int>()) int{16};
    CHECK( stack.get_last<int>()== 16);
    stack.pop_last<int>();
    stack.pop_last<array_t>();
    CHECK( stack.get_last<int>()== 4);
    stack.pop_last<int>();
    CHECK(stack.get_size() == 0 );
  };
};
