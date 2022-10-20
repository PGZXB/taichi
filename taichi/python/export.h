/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#pragma once

#include <type_traits>
#include <utility>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

#include "pybind11/stl.h"
#include "pybind11/cast.h"
#include "pybind11/numpy.h"
#include "pybind11/pytypes.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "taichi/common/core.h"

namespace taichi {

// namespace py = pybind11;
namespace py { // To profile exported APIs

namespace detail = pybind11::detail;
using pybind11::arg;
using pybind11::dict;
using pybind11::tuple;
using pybind11::array_t;
using pybind11::capsule;
using pybind11::list;
using pybind11::bool_;
// using pybind11::module;
using pybind11::enum_;
using pybind11::class_;
using pybind11::self;
using pybind11::init;
using pybind11::cast;
using pybind11::pickle;
using pybind11::make_tuple;
using pybind11::arithmetic;
using pybind11::overload_cast;
using pybind11::register_exception;
using pybind11::gil_scoped_release;
using pybind11::return_value_policy;
using pybind11::register_exception_translator;

// function traits, see https://www.cnblogs.com/yaoyu126/p/12427845.html
namespace func_traits {
template<typename T>
struct function_traits;

template<typename ReturnType, typename... Args>
struct function_traits<ReturnType(Args...)> {
  enum { arity = sizeof...(Args) };
  using return_type = ReturnType;
  using function_type = ReturnType(Args...);
  using stl_function_type = std::function<function_type>;
  using pointer = ReturnType(*)(Args...);

  template<size_t I>
  struct args {
    static_assert(I < arity, "index is out of range, index must less than sizeof Args");
    using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
  };

  using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
  using bare_tuple_type = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;
};

template<typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

template<typename ReturnType, typename... Args>
struct function_traits<ReturnType(&)(Args...)> : function_traits<ReturnType(Args...)> {};

template<typename ReturnType, typename... Args>
struct function_traits<std::function<ReturnType(Args...)>> : function_traits<ReturnType(Args...)> {};

#define FUNCTION_TRAITS(...)\
template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits<ReturnType(Args...)>{};\

FUNCTION_TRAITS()
FUNCTION_TRAITS(const)
FUNCTION_TRAITS(volatile)
FUNCTION_TRAITS(const volatile)
#undef FUNCTION_TRAITS

template<typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())> {};

template<typename Function>
typename function_traits<Function>::stl_function_type to_function(const Function& lambda) {
  return static_cast<typename function_traits<Function>::stl_function_type>(lambda);
}

template<typename Function>
typename function_traits<Function>::stl_function_type to_function(Function&& lambda) {
  return static_cast<typename function_traits<Function>::stl_function_type>(std::forward<Function>(lambda));
}

template<typename Function>
typename function_traits<Function>::pointer to_function_pointer(const Function& lambda) {
  return static_cast<typename function_traits<Function>::pointer>(lambda);
}
}  // namespace func_traits

struct GTiCoreStat {
  std::unordered_map<const char *, std::size_t> stat;
  
  ~GTiCoreStat() {
    std::cerr << "============Summary of calling ticore APIs============\n";
    std::vector<std::pair<const char *, std::size_t>> stat_copy(stat.begin(), stat.end());
    std::sort(stat_copy.begin(), stat_copy.end(), [](const auto &a, const auto &b) {
      return a.second > b.second;
    });
    std::ofstream ofs("./ticore_stat.csv");
    for (const auto &[k, v] : stat_copy) {
      std::cerr << k << " : " << v << "\n";
      ofs << k << ", " << v << '\n';
    }
    std::cerr << "======================================================\n";
  }
};
extern GTiCoreStat g_ticore_stat;

template<typename Func, typename FnTraits, std::size_t ...Is>
auto wrap_fn_impl(const char *name, Func &&f, std::index_sequence<Is...>) {
  using Res = typename FnTraits::return_type;
  return [f, name] (typename FnTraits::template args<Is>::type ...args) -> Res {
    ++g_ticore_stat.stat[name];
    return f(std::forward<decltype(args)>(args)...);
  };
}

template<typename Func>
auto wrap_fn(const char *name, Func &&f) {
  using fn_traits = func_traits::function_traits<Func>;
  constexpr auto indexes = std::make_index_sequence<fn_traits::arity>{};
  return wrap_fn_impl<Func, fn_traits>(name, std::forward<Func>(f), indexes);
}

#define FORAWRD_(func_name)                                      \
  template<typename ...Args>                                     \
  auto func_name(Args &&...args) {                               \
    return module_impl_->func_name(std::forward<Args>(args)...); \
  }

class module_wrapper {
 public:
  pybind11::module *module_impl_{nullptr};

  operator pybind11::module&() {
    return *module_impl_;
  }

  template <typename Func, typename... Extra>
  module_wrapper &def(const char *name_, Func &&f, const Extra& ... extra) {
    module_impl_->def(name_, wrap_fn(name_, std::forward<Func>(f)), extra...);
    return *this;
  }

  FORAWRD_(attr);
};

#undef FORAWRD_

using module = module_wrapper;

}  // namespace py

void export_lang(py::module &m);

void export_math(py::module &m);

void export_misc(py::module &m);

void export_visual(py::module &m);

void export_ggui(py::module &m);

}  // namespace taichi
