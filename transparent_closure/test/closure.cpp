#include "doctest/doctest.h"
#include "closure.hpp"

namespace transparent_closure{
  template<class ...Arg_ts>
  struct concat;
  
  template<class ...Arg_ts>
  struct concat<type_container<Arg_ts...>>{
    using type = type_container<Arg_ts...>;
  };
  template<class ...Arg1_ts, class ...Arg2_ts >
  struct concat<type_container<Arg1_ts...>,type_container<Arg2_ts...>>{
    using type=type_container<Arg1_ts ... ,Arg2_ts...>;
  };
  template<class cont1_t, class cont2_t, class cont3_t ,class ... cont_ts >
  struct concat<cont1_t,cont2_t,cont3_t,cont_ts...>{
    using type=
      typename concat<
      typename concat<cont1_t,cont2_t>::type,
      typename concat<cont3_t,cont_ts...>::type
      >::type;
  };
  
};

TEST_CASE("asdf" ){
  using namespace transparent_closure;
  CHECK(detail::is_open_slot<argument_slot<int>>::value);
  CHECK_FALSE(detail::is_open_slot<int>::value);
  CHECK(detail::has_open_slots<int,int,argument_slot<int>>::value);
  CHECK(detail::has_only_open_slots<argument_slot<int>,argument_slot<int>,argument_slot<int>>::value);
  CHECK_FALSE(detail::has_open_slots<>::value);
  CHECK(detail::has_only_open_slots<>::value);
  CHECK(std::is_same<
	type_container<int, long> ,
	typename concat<
	   type_container<int>,
	   type_container<long>
	>::type
	>::value);
  CHECK(std::is_same<
	type_container<int, long, float> ,
	typename concat<
	   type_container<int>,
	   type_container<long>,
	   type_container<float>
	>::type
	>::value);
  CHECK(std::is_same<
	type_container<int, long, float, double,int> ,
	typename concat<
	   type_container<int>,
	   type_container<long>,
	   type_container<float>,
	   type_container<double>,
	   type_container<>,
	   type_container<int>
	>::type
	>::value);
  ArgumentContainer<int, type_container<>> a_cont{};
};
