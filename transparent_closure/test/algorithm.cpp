#include <array>
#include <iostream>

#include "doctest/doctest.h"
#include "transparent_vector.hpp"
#include "transparent_tuple.hpp"
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

namespace {
  transparent_closure::MemcompareData
  test_fn1 (const void*, transparent_closure::IteratorStack &stack ){
    return transparent_closure::MemcompareData{
      .next_obj=nullptr,
	.next_function = nullptr,
	.obj = nullptr,
	.size=0,
	.obj_index = void_index
	};
  };

  struct MemberAccessibleStruct {
  public:
    std::tuple<const int*, const float*>
    get_member_access( )const{
      return std::tuple<const int*, const float*>(&this->i,&this->j);
    };
    int i = 0;
    float j = 1.0;
  };
} //

template<>
struct transparent_closure::concepts::is_member_accessible<MemberAccessibleStruct>: std::true_type{};

TEST_CASE("get_memcompare_data"){
  using namespace transparent_closure;
  SUBCASE("trivial") {
    // for a trivial type
    // get_mem_compare_data fills the Memcompare data struct with
    //   -- the passed next_function
    //   -- the passed next_obj ptr
    //   -- the size of the type
    //   -- a pointer to the value
    //   -- the void_index (for non polymorphic types type comparison is not needed.)
  
    static_assert(concepts::is_trivial<int>::value, "int should be trivially transparent");
    float next_obj =2.0;
    void * next_obj_vptr = &next_obj;
    adapter::next_function_t next_function = &test_fn1;
  
    int i=1;
    IteratorStack stack{};
    auto memcompare_data = adapter::get_memcompare_data<int>(
	&i,
	stack,
	next_obj_vptr,
	next_function
    );
    CHECK(memcompare_data.next_function == next_function);
    CHECK(memcompare_data.next_obj == next_obj_vptr);
    CHECK(memcompare_data.size == sizeof(int));
    CHECK(memcompare_data.obj == static_cast<const void*>(&i));
    CHECK(memcompare_data.obj_index == void_index);
  };

  SUBCASE("member_accessible"){
    float next_obj =2.0;
    void * next_obj_vptr = &next_obj;
    adapter::next_function_t next_function = &test_fn1;

    MemberAccessibleStruct obj{};
    IteratorStack stack{};
    
    auto memcompare_data = adapter::get_memcompare_data(
	&obj,
	stack,
	next_obj_vptr,
	next_function
    );
    REQUIRE( static_cast<bool>(memcompare_data.next_obj ));
    REQUIRE( static_cast<bool>(memcompare_data.next_function ));
    memcompare_data = std::move(memcompare_data.next_function(
	memcompare_data.next_obj,
	stack));
  };  
}


TEST_CASE("compare objects"){
  using namespace transparent_closure;
  SUBCASE("trivial"){
    int i=1;
    int j=1;
    int k=2;
    CHECK(compare_transparent_objects(i,j));
    CHECK_FALSE(compare_transparent_objects(i,k));
  };
  
  SUBCASE("vector"){
    std::vector<int>vec1 {12,543,776 };
    std::vector<int>vec2 {12,543,776 };
    std::vector<int>vec3 {12,543,776, 765 };
    std::vector<int>vec4 {12,543,777 };
    IteratorStack stack{};
    adapter::get_memcompare_data(&vec1, stack, nullptr, nullptr);
    CHECK(compare_transparent_objects(vec1,vec2));
    CHECK_FALSE(compare_transparent_objects(vec1,vec3));
    CHECK_FALSE(compare_transparent_objects(vec1,vec4));
  };
  
  SUBCASE("tuple"){  
    std::vector<int>vec1 {12,543,776 };
    std::vector<int>vec2 {12,543,777 };
    std::tuple< double,std::vector<int>, int> tuple1{
      2.4,
	vec1,
	134
    };

    std::tuple< double,std::vector<int>, int> tuple2{
      2.4,
	vec1,
	134
    };

    std::tuple< double,std::vector<int>, int> tuple3{
      2.4,
	vec2,
	134
    };
    
    CHECK(compare_transparent_objects(tuple1,tuple2));
    CHECK_FALSE(compare_transparent_objects(tuple1,tuple3));
    
  };
};
