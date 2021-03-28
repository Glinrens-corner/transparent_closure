#ifndef TRANSPARENT_CLOSURE_CLOSURE_HPP
#define TRANSPARENT_CLOSURE_CLOSURE_HPP

#include <cassert>
#include "meta_util.hpp"

namespace transparent_closure{

  template<class arg_t, class enclosed_t>
  struct enclosed_argument {
    using argument_type = arg_t;
    using enclosed_type = enclosed_t;
  };
  
  namespace detail {
    template<class T>
    struct is_open_argument :public true_type{};

    template<class T1, class T2>
    struct is_open_argument<enclosed_argument<T1,T2>> :public false_type{};

    template<class T>
    struct is_enclosed_argument {
      static constexpr bool value = not is_open_argument<T>::value;
    };
    
    template<class ... Arg_ts>
    struct has_open_argument{
      static constexpr bool value = (is_open_argument<Arg_ts>::value or ...);
    };

    template<class ... Arg_ts>
    struct has_only_open_arguments{
      static constexpr bool value = (is_open_argument<Arg_ts>::value and ...);
    };

    
  }//detail

  // ApplyTuplePacker
  namespace detail{
    template<class tuple_t>
    struct ApplyTuplePacker;

    template<class ... Arg_ts>
    struct ApplyTuplePacker<std::tuple<Arg_ts...>>{
      using apply_tuple_t =std::tuple<Arg_ts...>;
      static apply_tuple_t pack(Arg_ts&&... args){
	return  apply_tuple_t(std::forward<Arg_ts>(args)... );
      };
    };

  }//detail

  template <class return_t, class arguments_t, class enable=void> 
  class ArgumentContainer;
  
  // derive_superclass
  namespace detail{
    // derive superclass is a little helper class
    //  it is used to generate 
    template<class return_t,  class arguments_t >
    struct derive_superclass_impl{
      template<class T>
      struct to_argument_type_impl{
	using type = void;
      };
      template<class T, class V>
      struct to_argument_type_impl<enclosed_argument<T,V>>{
	using type = typename enclosed_argument<T,V>::argument_type;
      };
      template <class T >
      using to_argument_type = typename to_argument_type_impl<T>::type;
	
      template <class ... arg_ts>
      using open_first_closed_argument =
	transform_first_matching<
	detail::is_enclosed_argument,
	to_argument_type,
	arg_ts...>;
	
      using new_arguments_t= typename arguments_t
	::template apply<open_first_closed_argument>;

      using type = ArgumentContainer<return_t, new_arguments_t>;
    };
    template<class return_t,  class arguments_t >
    using derive_superclass = typename
      derive_superclass_impl<return_t, arguments_t>::type;
  }//detail

  namespace detail{
    template<std::size_t offset, class ... tuple_ts, size_t ... indices>
    decltype(auto) extract_from_tuple(std::tuple<tuple_ts...> atuple,std::index_sequence<indices...> seq){
      using tuple_t = std::tuple<tuple_ts...>;
      return std::tuple<typename std::tuple_element<offset+indices,tuple_t>::type...>(std::get<offset+indices>(atuple)...);
    };
    
    template<std::size_t pos, class insert_t, class  tuple_t>
    decltype(auto) insert_into_tuple (tuple_t tuple, insert_t insert ){
      return std::tuple_cat(
	  extract_from_tuple<0>( tuple, std::make_index_sequence<pos>()),
	  std::tuple<insert_t>(insert),
	  extract_from_tuple<pos>(tuple, std::make_index_sequence<std::tuple_size<tuple_t>::value-pos>())
      );
    };
  }// detail
  
  template<class return_t, class ...argument_ts >
  class  ArgumentContainer<
    return_t,
    type_container<argument_ts...>,
    typename enable_if<
      type_container<argument_ts...>
      ::template apply<detail::has_only_open_arguments>::value,
      void>::type>
    {
  private:
    using arguments_t = type_container<argument_ts...>;
    
    template <class ... arg_ts>
    using to_function_ptr_fn = return_t(*)(arg_ts...);
    
    using function_ptr_t =
      typename arguments_t::template apply<to_function_ptr_fn> ;

    using apply_tuple_t =
      typename arguments_t::template apply<std::tuple> ;

  public:
    using function_ptr_type = function_ptr_t;
    
  public:
    template <class arg_t>
    explicit ArgumentContainer(arg_t in)
      :function_ptr_(static_cast<function_ptr_t>(in)){
      assert(this->function_ptr_);
    };

    template<class ... argument2_ts>
    return_t operator() (argument2_ts ...args )const{
      return this->apply(
	  detail::ApplyTuplePacker<apply_tuple_t>::pack(std::forward<argument_ts>(args)...)
      );
    };
    
  protected:
    return_t apply(  apply_tuple_t && apply_tuple)const{
      return std::apply(this->function_ptr_,std::move(apply_tuple) );
    };

  private:
    function_ptr_t function_ptr_{};
  };


  template<class return_t, class ...argument_ts >
  class  ArgumentContainer<
    return_t,
    type_container<argument_ts...>,
    typename enable_if<
      not
      type_container<argument_ts...>
      ::template apply<detail::has_only_open_arguments>::value,
      void>::type>:
  // TODO is it necessary that this inheritance is public?
    public detail::derive_superclass<return_t,type_container<argument_ts...> >{
  private:
    using total_arguments_t = type_container<argument_ts...>;

    template<class ...arg_ts>
    using find_first_enclosed = find_first<
      detail::is_enclosed_argument, arg_ts... >;

    using enclosed_argument_holder_t = typename  total_arguments_t
      ::template apply<find_first_enclosed>;

    using enclosed_t = typename enclosed_argument_holder_t::enclosed_type;
    
    using argument_t = typename enclosed_argument_holder_t::argument_type;

    using superclass_t = detail::derive_superclass<return_t,type_container<argument_ts...> >;

       template <class ... arg_ts>
    using index_of_first_enclosed_argument = index_of_first_matching<
      detail::is_enclosed_argument,
      arg_ts...
      >;
    template<class ... arg_ts>
    using filter_open_arguments = filter<detail::is_open_argument, arg_ts... >;
    
    using apply_tuple_t =
      typename total_arguments_t
      ::template apply<filter_open_arguments>
      ::template apply<std::tuple>;

  public:
    using enclosed_type = enclosed_t;
  public:
    ArgumentContainer(superclass_t&& super, enclosed_t&& enclosed_argument  )
      :superclass_t(std::move(super))
      ,enclosed_argument_(std::forward<enclosed_t>(enclosed_argument)) {};

    template<class ... argument2_ts>
    return_t operator() (argument2_ts&& ...args )const{
      return this->apply(
	  detail::ApplyTuplePacker<apply_tuple_t>::pack(std::forward<argument2_ts>(args)...)
      );
    };
    
  protected:
    return_t apply(  apply_tuple_t && apply_tuple)const{
      constexpr std::size_t position = total_arguments_t
	::template apply<index_of_first_enclosed_argument>
	:: value;
      return superclass_t::apply(  
	  detail::insert_into_tuple<position,argument_t>(apply_tuple,   this->enclosed_argument_)
      );
	    
	};
  private:
    enclosed_t enclosed_argument_;
  };

}// transparent_closure


#endif //TRANSPARENT_CLOSURE_CLOSURE_HPP
