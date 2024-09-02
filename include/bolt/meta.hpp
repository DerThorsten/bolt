#pragma once


namespace bolt
{   

    ////////////////////////////////////////////////////////////////////////////////
    // null type
    ////////////////////////////////////////////////////////////////////////////////
    struct null_type
    {
    };
    
    ///////////////////////////////////////////////////
    // type-list
    ////////////////////////////////////////////////////////////////////////////////
    template<class... Ts>
    struct type_list
    {
        static constexpr std::size_t size = sizeof...(Ts);
    };



    ////////////////////////////////////////////////////////////////////////////////
    // merge variadic templates
    ////////////////////////////////////////////////////////////////////////////////
    template<class A, class B>
    struct merge_variadic;

    template<template<class...> class A, class... AArgs, template<class...> class B, class... BArgs>
    struct merge_variadic<A<AArgs...>, B<BArgs...>>
    {
        using type = A<AArgs..., BArgs...>;
    };

    template<class A, class B>
    using merge_variadic_t = typename merge_variadic<A, B>::type;


    ////////////////////////////////////////////////////////////////////////////////
    // apply arguments of one variadic template to another
    ////////////////////////////////////////////////////////////////////////////////
    template< template<class...> class A, class B>  
    struct use_variadic_args_from;

    template<template<class...> class A, template<class...> class B, class... BArgs>
    struct use_variadic_args_from<A, B<BArgs...>>
    {
        using type = A<BArgs...>;
    };

    template< template<class...> class A, class B>
    using use_variadic_args_from_t = typename use_variadic_args_from<A, B>::type;


    ////////////////////////////////////////////////////////////////////////////
    // apply a meta-function to each type in a a variadic template
    ////////////////////////////////////////////////////////////////////////////
    template<template<class ...> class F, class VARIADIC>
    struct apply_to_variadic;

    template<template<class ...> class F, template<class ...> class C, class... ARGS>
    struct apply_to_variadic<F, C<ARGS...>>
    {
        using type = C<F<ARGS>...>;
    };

    template<template<class ...> class F, class VARIADIC>
    using apply_to_variadic_t = typename apply_to_variadic<F, VARIADIC>::type;




};  // namespace bolt