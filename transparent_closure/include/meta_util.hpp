#ifndef TRANSPARENT_CLOSURE_META_UTIL_HPP
#define TRANSPARENT_CLOSURE_META_UTIL_HPP
#include <type_traits>

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

  template <class ...Arg_ts>
  using concat = typename concat_impl<Arg_ts...>::type;
  
  template<template<class >typename predicate_t, class collection_t>
  struct filter_impl{
    template<class T>
    using fn = if_else<
      predicate_t<T>::value,
      type_container<T>,
      type_container<>
      >;
    

    
    using type =
      typename collection_t
      ::template map<fn >
      ::template apply<concat>;
  };

  template<template<class >typename predicate_t,class ... arg_ts>
  using filter = typename filter_impl<predicate_t,arg_ts...>::type;

  struct NoneType{};
  
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


#endif // TRANSPARENT_CLOSURE_META_UTIL_HPP
