#ifndef TRANSPARENT_CLOSURE_CLOSURE_HPP
#define TRANSPARENT_CLOSURE_CLOSURE_HPP

#include <cassert>
#include <memory>
#include "meta_util.hpp"


// ArgumentContainer
namespace transparent_closure{

  template<class arg_t, class enclosed_t>
  struct enclosed_argument {
    static_assert( std::is_convertible<const enclosed_t, arg_t>::value, "can't convert the enclosed_arg in a const context"); 

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

  namespace detail {

    template<std::size_t pos, class enclosed_t>
    struct enclose_argument_at_position{
      template<class arg_t>
      using transform_T = enclosed_argument<arg_t, enclosed_t>;
      
      template<class ...arg_ts>
      using conversion_fn =  apply_at_position<pos, transform_T, arg_ts...>;
      template<class T>
      using apply_to = typename T::template apply<conversion_fn>;
    };

    template<std::size_t pos, class enclosed_t, class superclass_t>
    struct derive_new_self_class_impl;

    
    template<std::size_t pos, class enclosed_t, class return_t, class args_t>
    struct derive_new_self_class_impl<pos, enclosed_t, ArgumentContainer<return_t, args_t>>{
      using new_args_t  = typename enclose_argument_at_position<pos,enclosed_t>
	::template apply_to<args_t>;
      using type = ArgumentContainer<return_t,new_args_t>;
    };
    
    template<std::size_t pos, class enclosed_t, class superclass_t>
    using derive_new_self_class = typename derive_new_self_class_impl<
      pos, enclosed_t, superclass_t>::type;
  }//detail
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
    using return_type = return_t;
    using arguments_type = arguments_t;

  public:
    template <class arg_t>
    explicit ArgumentContainer(arg_t in)
      :function_ptr_(static_cast<function_ptr_t>(in)){
      assert(this->function_ptr_);
    };
    
    template<std::size_t pos, class enclosed_t>
    struct enclose_argument_at_position{
      template<class arg_t>
      using transform_t = enclosed_argument<arg_t, enclosed_t>;
      
      template<class ...arg_ts>
      using conversion_fn =  apply_at_position<pos, transform_t, arg_ts...>;
      template<class T>
      using apply_to = typename T::template apply<conversion_fn>;
    };
    
    template <std::size_t pos, class bind_t> 
    decltype(auto) bind_at(bind_t&& bind_arg ){
      static_assert( pos<sizeof...(argument_ts), "can't bind at non_existent position");

      using new_arguments_t = typename enclose_argument_at_position<
	pos,
	typename std::decay<bind_t>::type>::template apply_to< arguments_t>;
      return ArgumentContainer<return_t,new_arguments_t > {std::move(*this), std::forward<bind_t>(bind_arg)}; 
    };
    
    template<class ... argument2_ts>
    return_t operator() (argument2_ts ...args )const{
      return this->apply(
	  detail::ApplyTuplePacker<apply_tuple_t>::pack(std::forward<argument_ts>(args)...)
      );
    };
    
    return_t apply(  apply_tuple_t && apply_tuple)const{
      return std::apply(this->function_ptr_,std::move(apply_tuple) );
    };

  private:
    function_ptr_t function_ptr_{};
  };

  namespace detail{
    
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
    
    static constexpr std::size_t enclosed_position = total_arguments_t
      ::template apply<index_of_first_enclosed_argument>
      :: value;

        template<std::size_t pos, class enclosed_t>
    struct enclose_argument_at_position{
      template<class arg_t>
      using transform_t = enclosed_argument<arg_t, enclosed_t>;
      
      template<class ...arg_ts>
      using conversion_fn =  apply_at_position<pos, transform_t, arg_ts...>;
      template<class T>
      using apply_to = typename T::template apply<conversion_fn>;
    };

  public:
    using enclosed_type = enclosed_t;
    using return_type = return_t;
    using arguments_type = total_arguments_t;
  public:
    template<class input_t>
    ArgumentContainer(superclass_t&& super, input_t&& enclosed_argument  )
      :superclass_t(std::move(super))
      ,enclosed_argument_(std::forward<input_t>(enclosed_argument)) {};

  public:
    template<class ... argument2_ts>
    return_t operator() (argument2_ts&& ...args )const{
      return this->apply(
	  detail::ApplyTuplePacker<apply_tuple_t>::pack(std::forward<argument2_ts>(args)...)
      );
    };
    
    template <std::size_t pos, class bind_t> 
    decltype(auto) bind_at(bind_t&& bind_arg )&&{
      static_assert( pos<sizeof...(argument_ts), "can't bind at non_existent position");
      return std::move(*this).template bind_at_impl<pos>(std::forward<bind_t>(bind_arg));
      
    };
  private:
    template<std::size_t pos, class bind_t,  typename enable_if< (pos<enclosed_position ) , bool>::type = true>
    decltype(auto) bind_at_impl( bind_t&& bind_arg)&&{
      using new_arguments_t = typename enclose_argument_at_position<
	pos,
	typename std::decay<bind_t>::type>::template apply_to<total_arguments_t>;
      return ArgumentContainer<return_t,new_arguments_t > {std::move(*this), std::forward<bind_t>(bind_arg)}; 
    };

    template<std::size_t pos, class bind_t, typename enable_if<not (pos<enclosed_position ) , bool>::type = true>
    decltype(auto) bind_at_impl( bind_t&& bind_arg)&&{
      auto  new_super = superclass_t::template bind_at<pos+1, bind_t>(std::forward<bind_t>(bind_arg));
      using new_superclass_t = decltype( new_super);
      using new_self_class_t =
	detail::derive_new_self_class<
	  enclosed_position, enclosed_t, new_superclass_t>;
      return new_self_class_t{std::move(new_super), std::move(this->enclosed_argument_)};
    };
  public:
    return_t apply(  apply_tuple_t && apply_tuple)const{
      return superclass_t::apply(  
	  detail::insert_into_tuple<enclosed_position,argument_t>(apply_tuple,   this->enclosed_argument_)
      );    
    };
  private:
    enclosed_t enclosed_argument_;
  };

}// transparent_closure

//
namespace transparent_closure{
  template<class return_t, class arguments_t>
  class ArgumentContainerHolderBase{
    using apply_tuple_t = typename arguments_t
      ::template apply<std::tuple>;
  public:
    virtual return_t apply(apply_tuple_t&& )const=0;
    virtual ~ArgumentContainerHolderBase(){};
  };
  
  template<class ... arg_ts>
  using filter_open_arguments = filter<detail::is_open_argument, arg_ts... >;

  template<class return_t,class arguments_t>
  class ArgumentContainerHolder
    : public ArgumentContainerHolderBase<
    return_t,
    typename arguments_t
    ::template apply<filter_open_arguments>
    >{
    
    using apply_tuple_t =
      typename arguments_t
      ::template apply<filter_open_arguments>
  ::template apply<std::tuple>;
    
  public:
    ArgumentContainerHolder(
	ArgumentContainer<return_t, arguments_t> container)
      :container_(std::move(container)){};
    return_t apply(apply_tuple_t&& tuple)const{
      return this->container_.apply(std::move(tuple));
    };
  private:
    ArgumentContainer<return_t, arguments_t> container_;
  };
  template<class return_t, class arguments_t>
  class Function;

  template<class return_t, class ... argument_ts>
  class Function<return_t, type_container<argument_ts...>>{
    using arguments_t = type_container<argument_ts...>;
    using apply_tuple_t =
      typename arguments_t
      ::template apply<filter_open_arguments>
      ::template apply<std::tuple>;
   
  public:
    explicit Function(std::shared_ptr<ArgumentContainerHolderBase<return_t, type_container<argument_ts...> > > container)
      :container_(std::move(container)){};
    template<class ... argument2_ts>
    return_t operator()(argument2_ts&& ...arguments)const{
      return this->container_->apply(
	  apply_tuple_t( std::forward<argument2_ts>(arguments)...)
      );
    };
  private:
    std::shared_ptr<ArgumentContainerHolderBase<return_t, type_container<argument_ts...> > > container_;
  };
  

};

// Closure
namespace transparent_closure{

  template<class return_t, class arguments_t>
  class Closure{
  private:
    using container_t = ArgumentContainer<return_t,arguments_t>;

    template<class ... arg_ts>
    using filter_open_arguments = filter<detail::is_open_argument, arg_ts... >;
    
    using open_arguments_t =
      typename arguments_t
      ::template apply<filter_open_arguments>;

  public:
    explicit Closure(container_t container):container_(std::move(container)){};
    template<class ...argument_ts>
    return_t operator()(argument_ts...arguments)const{
      return this->container_(std::forward<argument_ts>(arguments)...);
    };

    template<std::size_t pos, class bind_t>
    decltype(auto) bind_at(bind_t&& bind_arg)&&{
      auto new_container = std::move(this->container_).template bind_at<pos>(std::forward<bind_t>(bind_arg));
      return Closure<return_t, typename decltype(new_container)::arguments_type>(std::move(new_container)
      );
    };
    
    decltype(auto) as_fun()&&{
      return Function<return_t, open_arguments_t>{
	std::shared_ptr<ArgumentContainerHolderBase<return_t, open_arguments_t>>{
	  new ArgumentContainerHolder<return_t, arguments_t>{std::move(this->container_ )}}};
    };
    
    template<class bind_first_t, class ...bind_ts>
    decltype(auto) bind(bind_first_t&& bind_first_arg, bind_ts&&... bind_args)&&{
      return std::move(*this).template bind_at<0>( std::forward<bind_first_t>(bind_first_arg)).bind(std::forward<bind_ts>(bind_args)...);
    };

    decltype(auto) bind()&&{
      return std::move(*this);
    };
  private:
     container_t container_;
  };
  
  namespace detail {
    template <class function_t>
    using to_function =  typename std::remove_pointer<
      typename std::decay<function_t>::type
      >::type;
   
    template<class function_t>
    struct make_closure_helper_function_data;

    template<class return_t, class ...arg_ts>
    struct make_closure_helper_function_data<return_t(arg_ts...)>{
      using return_type = return_t;
      using arguments_type = type_container< arg_ts...>;
      using function_pointer_type = return_t(*)(arg_ts...);
      
    };

   
    template<class lambda_call_t>
    struct make_closure_helper_lambda_data;

    template<class return_t, class lambda_t, class ...arg_ts>
    struct make_closure_helper_lambda_data<return_t(lambda_t::*)(arg_ts...)>{
      using return_type = return_t;
      using arguments_t = type_container< arg_ts...>;
      using function_pointer_type = return_t(*)(arg_ts...);
    };
    
    template<class return_t, class lambda_t, class ...arg_ts>
    struct make_closure_helper_lambda_data<return_t(lambda_t::*)(arg_ts...)const>{
      using return_type = return_t;
      using arguments_type = type_container< arg_ts...>;
      using function_pointer_type = return_t(*)(arg_ts...);
    };
    
    template<class function_t, class enable = void>
    struct make_closure_helper;

    template<
      class function_t>
    struct make_closure_helper<
      function_t,
      typename
      std::enable_if<
	std::is_function<
	  to_function<function_t>
	  >::value
	>::type>{
      using data = make_closure_helper_function_data<function_t>;
      
      static Closure<
	typename data::return_type,
	typename data::arguments_type> make(typename data::function_pointer_type arg ){
	return Closure<typename data::return_type, typename data::arguments_type>{
	  ArgumentContainer<typename data::return_type, typename data::arguments_type >{
	    static_cast<typename data::function_pointer_type>(arg )
	    }
	};
      };
    };


    template<
      class function_t>
    struct make_closure_helper<
      function_t,
      typename
      std::enable_if<
	not std::is_function<
	  to_function<function_t>
	  >::value
	>::type
      >{
      using data = make_closure_helper_lambda_data<decltype(&function_t::operator())>;

      template<class arg_t>
      static Closure<
	typename data::return_type,
	typename data::arguments_type> make(arg_t arg ){
	return Closure<typename data::return_type, typename data::arguments_type>{
	  ArgumentContainer<typename data::return_type, typename data::arguments_type >{
	    static_cast<typename data::function_pointer_type>(arg )
	    }
	};
      };
    };
  };
  
  template<class function_t >
  decltype(auto) make_closure(function_t&& function){
    return detail::make_closure_helper<
      detail::to_function<function_t>
      >::make(function);
  };
}// transparent_closure

#endif //TRANSPARENT_CLOSURE_CLOSURE_HPP
