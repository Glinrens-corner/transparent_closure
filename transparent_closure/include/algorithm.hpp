#ifndef TRANSPARENT_CLOSURE_ALGORITHM_HPP
#define TRANSPARENT_CLOSURE_ALGORITHM_HPP
#include <cstdlib>
#include <cstring>
#include <new>
#include <cassert>
#include <type_traits>
#include <typeindex>
#include <tuple>
#include "meta_util.hpp"

const auto void_index = std::type_index(typeid(void));

// 
// we simply assume that all scalar types are ok with this aligned
// there should be a static assert to make sure that it is always sufficient.

#define TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT 16

//MemcompareData
namespace transparent_closure{
  class IteratorStack;
  
  struct MemcompareData {
    const void * next_obj;
    MemcompareData(*next_function)(const void*, IteratorStack&);
    const void * obj;
    std::size_t size;
    std::type_index obj_index;
  };

}// transparent_closure

//IteratorStack
namespace transparent_closure {

  // TODO implement a smallbuffer optimization
  class IteratorStack {
    static constexpr std::size_t initial_max_size_ = 256;
  public:
    IteratorStack():size(0),max_size(initial_max_size_){
      this->stack_base = reinterpret_cast<char*>(std::aligned_alloc(TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT,this->initial_max_size_));
      if (!this->stack_base)throw std::bad_alloc();
    };

    // allocates a new T
    //    throws a bad_alloc if it cannot allocate enough storage
    template<class allocate_t>
    void * get_new(){
      static_assert( alignof(allocate_t)<TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT, "unsupported alignment");
      static_assert(std::is_trivially_copyable<allocate_t>::value, "cannot handle non-trivial copies");
      std::size_t old_size =  this->size;
      std::size_t new_size =  this->size + this->calculate_size_increase<allocate_t>();
      while (new_size > this->max_size) {
	this->reallocate();
      };
      assert(this->max_size >= new_size);
      this->size = new_size;
      return reinterpret_cast<void*>(this->stack_base+old_size);
    };

    // returns a reference to the last allocated object as if it was a T !
    //   note:If the last object was not a T it still returns a T&
    //   but its state is undefined.
    template<class allocate_t>
    allocate_t& get_last(){
      assert(this->size >= this->calculate_size_increase<allocate_t>());
      return *reinterpret_cast<allocate_t*>(this->stack_base+ ( this->size - this->calculate_size_increase<allocate_t>()) );
    };
    
    // deallocates the last T and returns it
    //   note:If the last object was not a T it still returns a T
    //   but its state is undefined.      
    template<class allocate_t>
    allocate_t pop_last(){
      assert(this->size>= this->calculate_size_increase<allocate_t>());
      const allocate_t ret = this->get_last<allocate_t>();
      this->size -= this->calculate_size_increase<allocate_t>();
      return ret;
    };
      
    ~IteratorStack(){
      std::free(this->stack_base);
    };

    private:
      template <class allocate_t>
      constexpr std::size_t calculate_size_increase() const{
	return (sizeof(allocate_t)/TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT
		+(sizeof(allocate_t) %TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT==0?0:1))
	  *TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT; 
      }
      
      void reallocate() {
	std::size_t new_max_size = 2*this->max_size;
	char *  new_base = reinterpret_cast<char*>(
	    std::aligned_alloc(TRANSPARENT_CLOSURE_MAX_SCALAR_ALIGNMENT,new_max_size)
	);
	if (!new_base)throw std::bad_alloc();
	std::memcpy(new_base, this->stack_base, this->max_size );
	std::free(this->stack_base);
	this->stack_base = new_base ;
	this->max_size = new_max_size;
      }
      
    public:// for testing
      constexpr static std::size_t get_init_max_size(){return initial_max_size_; };
      std::size_t get_size()const{return this->size;};
      std::size_t get_max_size()const{return this->max_size;};
      char* get_stack_base()const{return this->stack_base;};
    
  private:
    char * stack_base;
    std::size_t size;
    std::size_t max_size;
  };  
}// transparent_closure


namespace transparent_closure{
  namespace concepts{
    // note this is trivial in the mem_comparable_closure sense
    // a trivial type can be compared simply by memcmp'ing it.
    // so no padding, no pointers
    template<class T, class enable=void>
    struct is_trivial: std::false_type { };

    // a member_accessible type has a method get_members()
    //which returns a std::tuple of const references to its data_members 
    template<class T, class enable=void>
    struct is_member_accessible: std::false_type {};

    // is_specialized means there is a specialized version of
    // detail::get_mem_compare_info for this type
    template <class T, class enable =void>
    struct is_specialized:std::false_type{};
    
    // a protocol_compatible type has a
    //  MemcompareInfo get_memcompare_data(const void* next_obj,
    //                        next_function_t next_function,
    //                        detail::IteratorStack& stack)const
    // method.
    template<class T>
    struct is_protocol_compatible: std::false_type{};
    
    template<class T, class enable = void>
    struct is_transparent : std::false_type{ };

    template<class T >
    struct is_transparent<T, typename std::enable_if<
			       is_trivial<T>::value
			       or is_member_accessible<T>::value
			       or is_protocol_compatible<T>::value
			       or is_specialized<T>::value
			       >::type>: std::true_type {};

  }  // concepts

  // specializations for basic types
  namespace concepts{
    template<>
    struct is_trivial<bool> :std::true_type{};
    template<>
    struct is_trivial<char> :std::true_type{};
    template<>
    struct is_trivial<unsigned char> :std::true_type{};
    template<>
    struct is_trivial<signed char> :std::true_type{};
    
    template<>
    struct is_trivial<int> :std::true_type{};
    template<>
    struct is_trivial<unsigned int> :std::true_type{};
    //      template<>
    //      struct is_trivial<signed int> :std::true_type{};

    template<>
    struct is_trivial<short int> :std::true_type{};
    template<>
    struct is_trivial<unsigned short int> :std::true_type{};
    //      template<>
    //      struct is_trivial<signed short int> :std::true_type{};
      
    template<>
    struct is_trivial<long int> :std::true_type{};
    template<>
    struct is_trivial<unsigned long int> :std::true_type{};
    //      template<>
    //      struct is_trivial<signed long int> :std::true_type{};


    template<>
    struct is_trivial<long long int> :std::true_type{};
    template<>
    struct is_trivial<unsigned long long int> :std::true_type{};
    //      template<>
    //      struct is_trivial<signed long long int> :std::true_type{};

    template<>
    struct is_trivial<float> :std::true_type{};
    template<>
    struct is_trivial<double> :std::true_type{};
    
    template<>
    struct is_trivial<long double> :std::true_type{};
    template<>
    struct is_trivial<wchar_t> :std::true_type{};

    // variant for all enums
    template<class enum_t>
    struct is_trivial<enum_t,typename std::enable_if<std::is_enum<enum_t>::value>::type> : std::true_type{};
  } // concepts

  namespace adapter{
    using next_function_t = MemcompareData(*)(const void*, IteratorStack&);
    // is_protocol_compatible
    template <class T>
    typename std::enable_if<
      concepts::is_protocol_compatible<T>::value,
      MemcompareData
      >::type
    get_memcompare_data(
	const T* obj,
	IteratorStack& stack,
	const void* next_obj,
	next_function_t next_function) {
      return obj->get_memcompare_data(
	  stack,
	  next_obj,
	  next_function);
    };
    
    //is_trivial
    template<class T>
    typename std::enable_if<
      concepts::is_trivial<T>::value,
      MemcompareData
      >::type
    get_memcompare_data(const T* obj,
			IteratorStack& stack,
			const void* next_obj,
		        next_function_t next_function){
      return MemcompareData{
	.next_obj=next_obj,
	  .next_function = next_function,
	  .obj = static_cast<const void*>(obj),
	  .size=sizeof(T),
	  .obj_index = void_index
	  };
    };    
  }// adapter

  // adapter is public with the intention that user may put their own template specializations into it. 
  // I decided to not even put a sub namespace into it.  
  namespace adapter_detail{
    using adapter::next_function_t;
    // a simple struct to push onto the iterator stack.
    // for those cases when no iterator state is needed to be saved.
    struct BasicIterationSave {
      const void * next_obj;
      next_function_t  next_function;
    };

    template<class T>
    constexpr std::size_t member_tuple_size(){
      using tuple_t = decltype(static_cast<T*>(nullptr)->get_member_access());
      return std::tuple_size<tuple_t>::value;
    };

    template<std::size_t position, class T>
    typename std::enable_if<
      (position >= member_tuple_size<T>()),
	MemcompareData
	>::type
    get_memcompare_data_member_tuple(
	const void*,
	IteratorStack& stack
    ){
      auto it = stack.pop_last<BasicIterationSave>();
      return MemcompareData {
	.next_obj = it.next_obj,
	  .next_function = it.next_function,
	  .obj = nullptr,
	  .size = 0,
	  .obj_index = void_index
	  };
    };

    template<std::size_t position, class T>
    typename std::enable_if<
      (position < member_tuple_size<T>()),
	MemcompareData
	>::type
    get_memcompare_data_member_tuple(
	const void* vobj,
	IteratorStack& stack
	){
      const T* obj = static_cast<const T*>(vobj);
      using member_pointer_t = typename std::tuple_element<position, decltype(obj->get_member_access())>::type;
      // we want to assert that T is a pointer
      // if T is not a pointer std::remove_pointer<member_ptr_t>::type is the same as T.
      // so is_same is true if T is not a pointer.
      static_assert(
	  not
	  std::is_same<
	  member_pointer_t,
	  typename std::remove_pointer<member_pointer_t>::type
	  >::value, "this class returns a non-pointer in the tuple of get_member_access");
      member_pointer_t member_pointer = std::get<position>(obj->get_member_access());
   
      next_function_t next_function = &get_memcompare_data_member_tuple<position+1, T>;
      return adapter::get_memcompare_data(
	  member_pointer,
	  stack,
	  obj,
	  next_function);
    };

  } // adapter_detail
  
  // specializations of get_memcompare_data
  namespace adapter {
    
    template<class T>
    typename std::enable_if<
      concepts::is_member_accessible<T>::value,
      MemcompareData
      >::type
    get_memcompare_data(const T* obj,
			IteratorStack& stack,
			const void* next_obj,
		        next_function_t next_function){
      new ( stack.get_new<adapter_detail::BasicIterationSave>( ))
	adapter_detail::BasicIterationSave{
	.next_obj = next_obj,
	  .next_function = next_function
	  };
      return adapter_detail::get_memcompare_data_member_tuple<0,T>(
	  obj,
	  stack);
    };

    

  } // adapter
} // transparent_closure
#endif // TRANSPARENT_CLOSURE_ALGORITHM_HPP 
