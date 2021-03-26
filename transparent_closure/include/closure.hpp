#ifndef TRANSPARENT_CLOSURE_CLOSURE_HPP
#define TRANSPARENT_CLOSURE_CLOSURE_HPP
#include "meta_util.hpp"

namespace transparent_closure{
  template<class arg_t>
  struct argument_slot{
    using type = arg_t;
  };

  namespace detail {
    template<class T>
    struct is_open_slot :public false_type{};

    template<class T>
    struct is_open_slot<argument_slot<T>> :public true_type{};

    template<class ... Arg_ts>
    struct has_open_slots{
      static constexpr bool value = (is_open_slot<Arg_ts>::value or ...);
    };

    template<class ... Arg_ts>
    struct has_only_open_slots{
      static constexpr bool value = (is_open_slot<Arg_ts>::value and ...);
    };
  }//detail
  
  template <class return_t, class arguments_t, class enable=void> 
  class ArgumentContainer;

  template<class return_t, class arguments_t >
  class  ArgumentContainer<
    return_t,
    arguments_t,
    typename enable_if<
      arguments_t::template apply<detail::has_only_open_slots>::value,
      void>::type>{

  };


}// transparent_closure


#endif //TRANSPARENT_CLOSURE_CLOSURE_HPP
