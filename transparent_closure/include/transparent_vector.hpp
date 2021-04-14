#ifndef TRANSPARENT_CLOSURE_TRANSPARENT_VECTOR_HPP
#define TRANSPARENT_CLOSURE_TRANSPARENT_VECTOR_HPP
#include "algorithm.hpp"
#include <vector>

namespace vector_adapter {
  struct VectorIteratorStackRecord{
    transparent_closure::adapter::next_function_t next_function;
    const void * next_obj;
    std::size_t counter;
    std::size_t size;
  };

 
      
} //vector_adapter

namespace transparent_closure{
  template<class value_t, class allocator_t >
  struct concepts::is_specialized<std::vector<value_t, allocator_t>> : std::true_type{};

  namespace adapter {
    template<class value_t, class allocator_t>
    struct Adapter<std::vector<value_t,allocator_t>, void>{
      static_assert(not std::is_same<value_t, bool>::value, "can't handle vectors of bool");
    private:
      using vector_t = std::vector<value_t,allocator_t>;
      template <class T>
      using self_t = Adapter<vector_t>;
    public:
      static MemcompareData get_memcompare_data_continuation(
	  const void * vec_vptr, 
	  IteratorStack&stack) {
	using vector_adapter::VectorIteratorStackRecord;
	const vector_t& vec = *static_cast<const vector_t*>(vec_vptr);
	auto it = stack.get_last<VectorIteratorStackRecord>();
	
	if (it.counter > vec.size()){
	  auto ielement = it.counter;
	  it.counter += 1;
	  return adapter::get_memcompare_data(
	      &vec[ielement],
	      stack,
	      vec_vptr,
	      self_t<int>::get_memcompare_data_continuation
	  );

	} else {
	  auto it2 = stack.pop_last<VectorIteratorStackRecord>();
	  return transparent_closure::MemcompareData{
	    .next_obj = it2.next_obj,
	      .next_function = it2.next_function,
	      .obj = nullptr,
	      .size = 0,
	      .obj_index = void_index
	      };
	};
      };

      static MemcompareData get_memcompare_data(
	  const std::vector<value_t, allocator_t>* vector,
	  IteratorStack & stack,
	  const void* next_obj,
	  next_function_t next_function
      ){
	using vector_adapter::VectorIteratorStackRecord;
	auto  it = new (stack.get_new<VectorIteratorStackRecord>() )
	  VectorIteratorStackRecord{
	  .next_function = next_function,
	  .next_obj = next_obj,
	  .counter = 0,
	  .size = vector->size()
	};
	return MemcompareData {
	  .next_obj = vector,
	    .next_function = self_t<int>::get_memcompare_data_continuation,
	    .obj = static_cast<const void*>(&it->size),
	    .size = sizeof(std::size_t),
	    .obj_index = void_index
	    };
      };


    };

  }// adapter
}// transparent_closure






#endif //TRANSPARENT_CLOSURE_TRANSPARENT_VECTOR_HPP
