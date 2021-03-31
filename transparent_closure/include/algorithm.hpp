#ifndef TRANSPARENT_CLOSURE_ALGORITHM_HPP
#define TRANSPARENT_CLOSURE_ALGORITHM_HPP
#include <cstdlib>
#include <cstring>
#include <new>
#include <cassert>
#include <type_traits>
// we simply assume that all scalar types are ok with being 16 byte aligned
#define MAX_SCALAR_ALIGNMENT 16

//IteratorStack
namespace transparent_closure {
  class IteratorStack {
    // initial maximum size
    static constexpr std::size_t init_max_size_ = 256;
  public:
    IteratorStack():size(0),max_size(init_max_size_){
      this->stack_base = reinterpret_cast<char*>(std::aligned_alloc(MAX_SCALAR_ALIGNMENT,init_max_size_));
      if (!this->stack_base)throw std::bad_alloc();
    };

    // allocates a new T
    //    throws a bad_alloc if it cannot allocate enough storage
    template<class allocate_t>
    void * get_new(){
      static_assert( alignof(allocate_t)<MAX_SCALAR_ALIGNMENT, "unsupported alignment");
      static_assert(std::is_trivially_copyable<allocate_t>::value, "cannot handle non-trivial copies");
      std::size_t old_size =  this->size;
      std::size_t new_size =  this->size + this->calculate_size_increase<allocate_t>();
      while (new_size > this->max_size) {
	this->reallocate();
      };
      assert(this->max_size >= new_size);
      this->size = new_size;
      return reinterpret_cast<void*>(this->stack_base+old_size);
    }

    // returns a reference to the last allocated object as if it was a T !
    //   note:If the last object was not a T it still returns a T&
    //   but its state is undefined.
    template<class allocate_t>
    allocate_t& get_last(){
      assert(this->size >= this->calculate_size_increase<allocate_t>());
      return *reinterpret_cast<allocate_t*>(this->stack_base+ ( this->size - this->calculate_size_increase<allocate_t>()) );
    }
    // deallocates the last T and returns it
    //   note:If the last object was not a T it still returns a T
    //   but its state is undefined.      
    template<class allocate_t>
    allocate_t pop_last(){
      assert(this->size>= this->calculate_size_increase<allocate_t>());
      const allocate_t ret = this->get_last<allocate_t>();
      this->size -= this->calculate_size_increase<allocate_t>();
      return ret;
    }
      
    ~IteratorStack(){
      std::free(this->stack_base);
    };

    private:
      template <class allocate_t>
      constexpr std::size_t calculate_size_increase() const{
	return (sizeof(allocate_t)/MAX_SCALAR_ALIGNMENT+ (sizeof(allocate_t) %MAX_SCALAR_ALIGNMENT==0?0:1))*MAX_SCALAR_ALIGNMENT; 
      }
      
      void reallocate() {
	std::size_t new_max_size = 2*this->max_size;
	char *  new_base = reinterpret_cast<char*>(std::aligned_alloc(MAX_SCALAR_ALIGNMENT,new_max_size));
	if (!new_base)throw std::bad_alloc();
	std::memcpy(new_base, this->stack_base, this->max_size );
	std::free(this->stack_base);
	this->stack_base = new_base ;
	this->max_size = new_max_size;
      }
      
    public:// for testing
      constexpr static std::size_t get_init_max_size(){return init_max_size_; };
      std::size_t get_size()const{return this->size;};
      std::size_t get_max_size()const{return this->max_size;};
      char* get_stack_base()const{return this->stack_base;};
    
  private:
    char * stack_base;
    std::size_t size;
    std::size_t max_size;
  };
  
}// transparent_closure

#endif // TRANSPARENT_CLOSURE_ALGORITHM_HPP 
