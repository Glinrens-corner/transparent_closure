#include "doctest/doctest.h"
#include <utility>
#include <tuple>
#include <vector>
#include <memory>
#include "closure.hpp"

namespace {
  template<class T>
  struct not_int :public std::true_type{};
  
  template<>
  struct not_int<int> :public std::false_type{};
}//

TEST_CASE("metaprogramming" ){

  using namespace transparent_closure;
  
  SUBCASE("is_open_argument" ){
    // is_open_argument 
    CHECK(detail::is_open_argument<int>::value);
    CHECK_FALSE(detail::is_open_argument<enclosed_argument<int,int>>::value);
  };
  
  SUBCASE("has_open_argument,has_only_open_arguments"){
    // has_open_argument checks if a list of types has at least one that is not an enclosed argument
    CHECK(detail::has_open_argument<int,int,enclosed_argument<int,int>>::value);
    CHECK_FALSE(detail::has_open_argument<>::value);
    
    CHECK_FALSE(detail::has_open_argument<enclosed_argument<int,int>,enclosed_argument<float,float>>::value);
    
    CHECK_FALSE(detail::has_only_open_arguments<enclosed_argument<int,long>,enclosed_argument<int, long>,enclosed_argument<int,long>>::value);
    
    CHECK(detail::has_only_open_arguments<int, long, double>::value);
    // Important !!!!
    // has_only_open_arguments really means has no enclosed arguments
    // TODO rename it.
    CHECK(detail::has_only_open_arguments<>::value);
  };
};

namespace {
  int test_function(){
    return 1;
  };
  namespace {
    long test_fn(int i, int j){
      return i*j;
    };
  };
} //

TEST_CASE("ArgumentContainer"){
  using namespace transparent_closure;
  SUBCASE("instantiation"){
    
    // A trivial Argument Container can be filled by a function,  function_pointer, or a captureless! lambda.
    ArgumentContainer<int, type_container<>> arg_cont1{test_function};
    ArgumentContainer<int, type_container<>> arg_cont2{&test_function};
    ArgumentContainer<int, type_container<>> arg_cont3{[]()->int{return 1;}};
    ArgumentContainer<double, type_container<double,int>> arg_cont4{
      [](double a,int b)->double{return a*b; }
    };
    
    // Assertion Error
    // an argument container may not be created over a nullptr.
    // ArgumentContainer<int, type_container<>> a_cont{nullptr};
  };
  
  SUBCASE("instanciate enclosing closure"){
    
    using enclosing_cont1_t = ArgumentContainer<
      double,
      type_container<enclosed_argument<const float&,float>,int&> >  ;
    
    using cont1_t = ArgumentContainer<
      double,
      type_container<const float&,int&>>;
    
    CHECK(std::is_same<enclosing_cont1_t::enclosed_type, float>::value);
    enclosing_cont1_t cont1{[]()->cont1_t{
	return cont1_t([](const float& i, int& j)->double{return i*j;});
      }(),3.0
	 };
    int a = 1;
    
    cont1(a);
  };

  SUBCASE("make_closure"){
    Closure<long, type_container<int,int>> cl = make_closure([](int,int)->long{return 56;});
    
    CHECK(cl(4,5) == 56);
    
    Closure<long, type_container<int,int>> cl2 = make_closure(test_fn);
    
    CHECK(cl2(4,5) == 20);
  };
  
  SUBCASE("invocation"){
    // an ArgumentContainer can be called.
    
    SUBCASE("by_value"){
      // arguments can be transferred by value 
      struct test_struct{
	int member = 4;
      };
      ArgumentContainer<int, type_container<test_struct, int>>
	container{
	[](test_struct tst,int i )->int{return tst.member*i;}
      };

      CHECK(container(test_struct{6},4) == 24);

      CHECK(container(test_struct{},4) == 16);
    };

    SUBCASE("references"){
      //  it can correctly handle l-references, const l-references
      //     and r-value references 
      int i = 1;
      int& j = i;
      long k = 22;
      float l = 4.0;
    
      ArgumentContainer<double, type_container<int&&, const long&, float&>> arg_cont0{
	[]( int&&, const long &, float&)->double{return 2.0;}
      };
    
      CHECK(arg_cont0(std::move(i), k, l) ==  2.0);
      
      CHECK(std::is_same<decltype(arg_cont0(std::move(i), k, l)),double >::value);
    };
  };
  
  SUBCASE("bind and invoke"){

    // we can also bind arguments before calling them.
    using argument_container_t = 
      ArgumentContainer<double, type_container<const float&, const int&>>;
    argument_container_t cont1{[](const float& a, const int& b)->double{return a*b;} };
    
    ArgumentContainer<double, type_container<const float&, enclosed_argument<const int&,int>>>  bound_cont1= cont1.bind_at<1,int>(2);
    
    auto fully_bound_cont1 = std::move(bound_cont1).bind_at<0>( static_cast<float>(3.0));
    
    CHECK(fully_bound_cont1() == 6.0 );
    CHECK(fully_bound_cont1() == 6.0 );


    
    argument_container_t cont2{[](const float& a, const int& b)->double{return a*b;} };
    ArgumentContainer<double, type_container<enclosed_argument<const float&, float>, const int&>>  bound_cont2= cont2.bind_at<0,float>(3.0);
    int i = 2;
    int& j = i;
    auto fully_bound_cont2 = std::move(bound_cont2).bind_at<0>( j);
    
    CHECK(fully_bound_cont2() == 6.0 );
    // Note arguments are stored as decay<input_type>::type
    // so references are stored as values!
    j = 3;
    CHECK(fully_bound_cont2() == 6.0 );
    
    // Note, that operator() is annotated as const.
    //  that means that an enclosed argument cannot be
    //  transferred to the function as a non-const reference
    //  (or any other way that doesn't guarantee constness).

    // this doesn't work:
    // int k = 99;
    // auto container = ArgumentContainer<....> ={
    //  [](int& i )->long {return i; }
    // }.bind_at<0>(k);
  };
  
  SUBCASE("bind and invoke with conversion"){
    // the bound type does not have to be the same type as
    // the argument type of the function
    
    ArgumentContainer<double, type_container<float, int>> cont1{
      []( float a, int b)->double{return a*b;}
    };
    
    ArgumentContainer<double, type_container<enclosed_argument<float,double>, enclosed_argument<int,char>>>
      cont1_bound =cont1.bind_at<0>(static_cast<double>( 3.0)).bind_at<0>(static_cast<char>(2 )); 
  };
  
  SUBCASE("bind multiple"){
    // of course you can bind multiple values
    // up to all values of the enclosed function
    Closure<double, type_container<float, int>>cont1 {
	ArgumentContainer<double, type_container<float, int> >{
	  []( float a, int b)->double{return a*b;}
	}
    };
    using bound_cont_t =
      Closure<double, type_container<enclosed_argument<float,float>, enclosed_argument<int,int>>>;
    // for some god forsaken reason this generates a SIGILL --illegal instruction (but only if not in debug mode)...
    // clang version 7.0.1 (tags/RELEASE_701/final 349238)
    // Target: x86_64-unknown-linux-gnu
    
    //    bound_cont_t bound_cont{
    //  std::move(cont1).bind(
    //	  static_cast<float>(2.0),
    //	  static_cast<int>(3) )};

    bound_cont_t bound_cont{
      std::move(cont1)
	.bind_at<0>(static_cast<float>(2.0))
	.bind_at<0>(static_cast<int>(3))
    };
    CHECK(bound_cont() == 6);
    SUBCASE("Function"){
      Function<double,type_container<>>  fun = std::move(bound_cont).as_fun();
      CHECK(fun() == 6.0);
      CHECK(fun() == 6.0);
      // a function can be copied.
      //  a function wraps a shared pointer to the ArgumentContainer
      Function<double,type_container<>> fun2 = fun;
      CHECK(fun2() == 6.0);
      CHECK(fun() == 6.0);
    };
  };
};
