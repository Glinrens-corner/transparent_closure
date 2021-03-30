#ifndef TRANSPARENT_CLOSURE_META_UTIL_HPP
#define TRANSPARENT_CLOSURE_META_UTIL_HPP
#include <type_traits>

template <class ...Ts>
struct show;


// making some standard type_trait types and templates our own
//   because they are awesome
//      ... and good developer need to steal from time to time.
namespace transparent_closure{
  using std::false_type;
  using std::true_type;
  using std::enable_if;
} // transparent_closure

// type_container
namespace transparent_closure{
  template<class ...Arg_ts>
  struct type_container{
    template <template <class >typename F>
    using map = type_container<F<Arg_ts>...>  ;
    template <template <class... >typename F>
    using apply = F<Arg_ts...>  ;
    static constexpr std::size_t size = sizeof...(Arg_ts); 
  };
} // transparent_closure


namespace transparent_closure{
  template<class ...Arg_ts>
  struct concat_impl;

  template<>
  struct concat_impl<>{
    using type=type_container<>;
  };
  
  template<class ...Arg_ts>
  struct concat_impl<type_container<Arg_ts...>>{
    using type = type_container<Arg_ts...>;
  };
  
  template<class ...Arg1_ts, class ...Arg2_ts >
  struct concat_impl<type_container<Arg1_ts...>,type_container<Arg2_ts...>>{
    using type=type_container<Arg1_ts ... ,Arg2_ts...>;
  };
  
  template<class cont1_t ,class ... cont_ts >
  struct concat_impl<cont1_t,cont_ts...>{
    using type=
      typename concat_impl<cont1_t,
      typename concat_impl<cont_ts...>::type
      >::type;
  };
  
  template <class ...Arg_ts>
  using concat = typename concat_impl<Arg_ts...>::type;
}// transparent_closure

namespace transparent_closure{
  template<bool t, class D, class E>
  struct if_else_impl{
    using type = D;
  };
  
  template<class D, class E>
  struct if_else_impl<false , D, E>{
    using type = E;
  };

  template<bool t, class D, class E>
  using if_else = typename if_else_impl<t,D,E>::type;
}// transparent_closure

// filter
namespace transparent_closure{
  template<class T>
  struct unwrap_impl;

  template<class T>
  struct unwrap_impl<type_container<T>>{
    using type = T;
  };

  template<class wrapped_t>
  using unwrap = typename unwrap_impl<wrapped_t>::type;

  
  template<template<class >typename predicate_t, class ... arg_ts>
  struct filter_impl{
    template<class T>
    using fn = if_else<
      predicate_t<T>::value,
      type_container<T>,
      type_container<>
      >;
    
    using type = concat<fn<arg_ts>... >;
  };

  template<template<class >typename predicate_t,class ... arg_ts>
  using filter = typename filter_impl<predicate_t,arg_ts...>::type;
}//transparent_closure

// NoneType
namespace transparent_closure{
  struct NoneType{};
}//transparent_closure

namespace transparent_closure{
  template<
    template <class >typename predicate_T,
    class first_arg_t,
    class ... arg_ts>
  struct find_first_impl{
    using type = if_else<predicate_T<first_arg_t>::value, first_arg_t,

			 typename find_first_impl<predicate_T, arg_ts...>::type >;
  };

  template <template<class> typename predicate_T, class only_arg_t>
  struct find_first_impl<predicate_T,only_arg_t>{
    using type = if_else<predicate_T<only_arg_t>::value, only_arg_t, NoneType>;
  };

  template<template <class >typename predicate_T,
	   class ... Arg_ts>
  using find_first = typename find_first_impl<predicate_T, Arg_ts...>::type;
}//transparent_closure

// transform_first_matching_impl
namespace transparent_closure{
  template<
    template<class>typename predicate_T,
    template<class>typename transformer_T,
    class ...arg_ts>
  struct transform_first_matching_impl;

  template<
    template<class>typename predicate_T,
    template<class>typename transformer_T,
    class first_t,
    class ...arg_ts>
  struct transform_first_matching_impl<
    predicate_T,
    transformer_T,
    first_t,
    arg_ts...>{
    using type =
      if_else<
      predicate_T<first_t>::value,
      concat<type_container<transformer_T<first_t>>,
	     type_container<arg_ts...>>,
      concat<type_container<first_t>,
	     typename transform_first_matching_impl<
	       predicate_T,
	       transformer_T,
	       arg_ts...>::type
	     >
      >;
  };
  
  template<
    template<class>typename predicate_T,
    template<class>typename transformer_T
    >
  struct transform_first_matching_impl<predicate_T,transformer_T>{
    using type = type_container<>;
  };
  
  template<
    template<class>typename predicate_T,
    template<class>typename transformer_T,
    class ...arg_ts>
  using transform_first_matching=
    typename transform_first_matching_impl<predicate_T, transformer_T,arg_ts... >::type;

}//transparent_closure

namespace transparent_closure{
  template<
    std::size_t i,
    template<class >typename predicate_T,
    class ... arg_ts>
  struct index_of_first_matching_impl;

  
  template<
    std::size_t i,
    template<class >typename predicate_T,
    class first_t,
    class ... arg_ts>
  struct index_of_first_matching_impl<i, predicate_T, first_t, arg_ts...>{
    static constexpr std::size_t value =
      predicate_T<first_t>::value
      ? i
      : index_of_first_matching_impl<i+1,predicate_T,arg_ts...>::value;
  };

  template<
    std::size_t i,
    template<class >typename predicate_T>
  struct index_of_first_matching_impl<i, predicate_T>{
    static constexpr std::size_t value = i;
  };

  template<template<class >typename predicate_T, class ... arg_ts>
  using index_of_first_matching = index_of_first_matching_impl<0,predicate_T, arg_ts...>;
  
}//transparent_closure

namespace transparent_closure{
  template<std::size_t pos, template<class> typename transform_T, class ...arg_ts>
  struct apply_at_position_impl;

  
  template<std::size_t pos, template<class> typename transform_T, class first_t, class ...arg_ts>
  struct apply_at_position_impl<pos, transform_T,first_t,arg_ts...>{
    using type = concat<
      type_container<first_t>,
      typename apply_at_position_impl<pos-1, transform_T, arg_ts...>::type>;
    
  };

  template< template<class> typename transform_T,class first_t,  class ...arg_ts>
  struct apply_at_position_impl<0, transform_T, first_t, arg_ts...>{
    using type = type_container<transform_T<first_t>, arg_ts...>;
  };
  template< std::size_t pos, template<class> typename transform_T>
  struct apply_at_position_impl<pos, transform_T>{
    using type = type_container<>;
  };

  
  template<std::size_t pos, template<class> typename transform_T, class ...arg_ts>
  using apply_at_position = typename apply_at_position_impl<pos,transform_T,arg_ts...>::type;
}// transparent_closure
#endif // TRANSPARENT_CLOSURE_META_UTIL_HPP
