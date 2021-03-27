#include "doctest/doctest.h"
#include <utility>
#include <tuple>
#include "closure.hpp"


template<class T>
struct not_int :public std::true_type{};
template<>
struct not_int<int> :public std::false_type{};

TEST_CASE("metaprogramming" ){

  using namespace transparent_closure;
  SUBCASE("is_open_slot" ){
    CHECK(detail::is_open_argument<int>::value);
    CHECK_FALSE(detail::is_open_argument<enclosed_argument<int,int>>::value);
  };
  
  SUBCASE("has_open_argument,has_only_open_arguments"){
    CHECK(detail::has_open_argument<int,int,enclosed_argument<int,int>>::value);
    CHECK_FALSE(detail::has_only_open_arguments<enclosed_argument<int,long>,enclosed_argument<int, long>,enclosed_argument<int,long>>::value);
    CHECK(detail::has_only_open_arguments<int, long, double>::value);
    // Important !!!!
    // has_only_open_arguments really means has no enclosed arguments
    // TODO rename it.
    CHECK_FALSE(detail::has_open_argument<>::value);
    CHECK(detail::has_only_open_arguments<>::value);
  };
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
	  filter<not_int,type_container<int, long, float, double,int>> 
	  >::value);
  };

  
};

int test_function(){
  return 1;
};

template<class T>
struct show;

TEST_CASE("ArgumentContainer"){
  using namespace transparent_closure;
  SUBCASE("instantiation"){
    // Assertion Error
    // ArgumentContainer<int, type_container<>> a_cont{nullptr};
    // A trivial Argument Container can be filled by a function,  function_pointer, or a lambda.
    ArgumentContainer<int, type_container<>> arg_cont1{test_function};
    ArgumentContainer<int, type_container<>> arg_cont2{&test_function};
    ArgumentContainer<int, type_container<>> arg_cont3{[]()->int{return 1;}};
    ArgumentContainer<double, type_container<double,int>> arg_cont4{
      [](double a,int b)->double{return a*b; }
    };
  };
  SUBCASE("instanciate enclosing closure"){
    using enclosing_cont1_t = ArgumentContainer<double, type_container<enclosed_argument<float,double>,int>>  ;
    using cont1_t = ArgumentContainer<double, type_container<float,int>>;
    CHECK(std::is_same<enclosing_cont1_t::enclosed_type, double>::value);
    enclosing_cont1_t cont1{[]()->cont1_t{
	return cont1_t([](float i, int j)->double{return i*j;});
      }(),3.0
	 };
  };
  SUBCASE("invocation"){
    int i = 1;
    int& j = i;
    int& m = std::forward<int&>(j);
    //    show<decltype(m)>{};
    long k = 22;
    float l = 4.0;
    ArgumentContainer<double, type_container<int&&, const long&, float&>> arg_cont0{
      []( int&&, const long &, float&)->double{return 2.0;}
    };
    CHECK(arg_cont0(std::move(i), k, l) ==  2.0);
    CHECK(std::is_same<decltype(arg_cont0(std::move(i), k, l)),double >::value);
  };

};
