
#ifndef CPPCONTRACTS_CONTRACTS_HPP_
#define CPPCONTRACTS_CONTRACTS_HPP_

//	https://bitbucket.org/alexknvl/cpp0x-contracts/src/e248d0b3adfada561ee4f3e64fc3858c2111416c/src/CppContracts/?at=default


// Awesome contracts implementation (c) 2012 Alexander Konovalov

// No __result__ or __old__ yet =(
// I have an idea how to make __result__ work but it will 
// be very platform-specific, and will require a lot of
// assembly hackery.

#include <tr1/functional>

#ifndef assert
  #include <cassert>
#endif

/******************************************************************************
 * Preprocessor magic
 *****************************************************************************/
#define PP_STRING(a) #a
#define PP_CONCAT2(a,b)  a##b
#define PP_CONCAT3(a, b, c) a##b##c
#define PP_UNIQUE_LABEL_(prefix, suffix) PP_CONCAT2(prefix, suffix)
#define PP_UNIQUE_LABEL(prefix) PP_UNIQUE_LABEL_(prefix, __LINE__)

/******************************************************************************
 * on_scope_init and on_scope_exit macroses
 *****************************************************************************/
struct __call_on_constructor {
    template<class Func> inline __call_on_constructor(Func func) {
        func();
    }
};
#define on_scope_init(function) \
    __call_on_constructor PP_UNIQUE_LABEL(on_init) (function)

struct __call_on_destructor {
    std::tr1::function<void()> _function;
    template<class Func> inline __call_on_destructor(Func func) {
        _function = func;
    }
    inline ~__call_on_destructor() {
        _function();
    }
};
#define on_scope_exit(function) \
    __call_on_destructor PP_UNIQUE_LABEL(on_exit) (function)

/******************************************************************************
 * Contracts
 *****************************************************************************/
#define requires(expression) \
  on_scope_init([&] () { assert(expression); })
#define ensures(expression) \
  on_scope_exit([&] () { assert(expression); })
  
#define invariant() \
  on_scope_init([&] () { __invariant(); }); \
  on_scope_exit([&] () { __invariant(); })

#endif//CPPCONTRACTS_CONTRACTS_HPP_