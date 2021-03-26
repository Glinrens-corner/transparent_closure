#ifndef TRANSPARENT_CLOSURE_META_UTIL_HPP
#define TRANSPARENT_CLOSURE_META_UTIL_HPP
#include <type_traits>
namespace transparent_closure{
  using std::false_type;
  using std::true_type;
  using std::enable_if;
  
  template<class ...Arg_ts>
  struct type_container{
    template <template <class >typename F>
    using map = type_container<F<Arg_ts>...>  ;
    template <template <class... >typename F>
    using apply = F<Arg_ts...>  ;
    
  };


  

} // transparent_closure

#endif // TRANSPARENT_CLOSURE_META_UTIL_HPP
