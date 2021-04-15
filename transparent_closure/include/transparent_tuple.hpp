#ifndef TRANSPARENT_CLOSURE_TRANSPARENT_TUPLE_HPP
#define TRANSPARENT_CLOSURE_TRANSPARENT_TUPLE_HPP

#include <tuple>
#include "algorithm.hpp"

namespace tuple_adapter_detail{
  struct BasicIteratorSaveRecord{
    const void* next_obj;
    ::transparent_closure::adapter::next_function_t next_function; 
  };

  template<std::size_t position, class tuple_t>
  typename std::enable_if<
    (std::tuple_size<tuple_t>::value <= position),
      transparent_closure::MemcompareData
      >::type
  get_memcompare_data_continuation(
      const void* tuple_vptr,
      transparent_closure::IteratorStack & stack
  ){
    auto it = stack.pop_last< BasicIteratorSaveRecord>();
    return transparent_closure::MemcompareData{
      .next_obj = it.next_obj,
	.next_function=it.next_function,
	.obj = nullptr,
	.size = 0,
	.obj_index = void_index
	};
  };
  
  template<std::size_t position, class tuple_t>
  typename std::enable_if<
    (std::tuple_size<tuple_t>::value > position),
    transparent_closure::MemcompareData
      >::type
  get_memcompare_data_continuation(
      const void* tuple_vptr,
      transparent_closure::IteratorStack & stack
  ){
    const tuple_t& tuple = *static_cast<const tuple_t*>(tuple_vptr);
    return transparent_closure::adapter::get_memcompare_data(
	&std::get<position>(tuple),
	stack,
	tuple_vptr,
	get_memcompare_data_continuation<position+1,tuple_t>);
  };
  
  
}// tuple_adapter_detail

namespace transparent_closure {
  template<class ... argument_ts >
  struct concepts::is_specialized<std::tuple<argument_ts...>> : std::true_type{};

  namespace adapter {
    template <class ... argument_ts>
    class  Adapter<std::tuple<argument_ts...>> {
    private:
      using save_record_t = tuple_adapter_detail::BasicIteratorSaveRecord;
      using tuple_t = std::tuple<argument_ts...>;
    public:
      
      static MemcompareData get_memcompare_data(
	  const tuple_t* tuple,
	  IteratorStack&stack,
	  const void* next_obj,
	  next_function_t next_function
      ) {
	using tuple_adapter_detail::get_memcompare_data_continuation;
	auto it = new(stack.get_new<save_record_t>()) save_record_t{
	  .next_obj = next_obj,
	  .next_function = next_function
	};
	return  get_memcompare_data_continuation<0,tuple_t>(
	    tuple,
	    stack
	);
      };
    };
  }//  adapter
}//transparent_closure



#endif //TRANSPARENT_CLOSURE_TRANSPARENT_TUPLE_HPP
