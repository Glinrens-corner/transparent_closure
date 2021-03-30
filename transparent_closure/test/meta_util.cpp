#include "doctest/doctest.h"
#include "meta_util.hpp"

namespace {
  template<class T>
  struct not_int :public std::true_type{};
  template<>
  struct not_int<int> :public std::false_type{};
}//

template <class ...arg_ts>
using filter_not_int = transparent_closure::filter<not_int, arg_ts...>;

TEST_CASE("metaprogramming utility"){
  using namespace transparent_closure;
  SUBCASE("concat"){
    CHECK(std::is_same<
	  type_container<int, long> ,
	   concat<
	   type_container<int>,
	   type_container<long>
	>
	>::value);
    
    CHECK(std::is_same<
	  type_container<int, long, float> ,
	   concat<
	  type_container<int>,
	  type_container<long>,
	   type_container<float>
	  >
	  >::value);
    CHECK(std::is_same<
	  type_container<int, long, float, double,int> ,
	  concat<
	  type_container<int>,
	  type_container<long>,
	  type_container<float>,
	  type_container<double>,
	  type_container<>,
	  type_container<int>
	  >
	  >::value);
  };
  
  SUBCASE("filter"){
    
    CHECK(std::is_same<
	  type_container<long, float, double>,
	  typename type_container<int, long, float, double,int>::template apply<filter_not_int>	
	  >::value);
  };

};



