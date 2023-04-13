/*
 * File: function_info.hpp
 * Created Date: 2023-04-02
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Tuesday April 11th 2023 7:57:12 pm
 *
 * Copyright (c) 2023 None
 *
 * -----
 * HISTORY:
 * Date      	 By	Comments
 * ----------	---
 * ----------------------------------------------------------
 */

#pragma once

#include <functional>
#include <tuple>

namespace lra::usb_lib {

/**
 * Ref: https://godbolt.org/z/GTzYcTvPn
 */
template <typename ReturnType, typename... Args>
struct FuncInfo {
  using return_type = ReturnType;
  using args_type = std::tuple<Args...>;

  std::function<ReturnType(Args...)> func;
  std::tuple<Args...> default_args;

  FuncInfo(std::function<ReturnType(Args...)> f, Args... args)
      : func(f), default_args(std::make_tuple(args...)) {}

  template <typename... CallArgs>
  ReturnType call(CallArgs... args) {
    if constexpr (sizeof...(CallArgs) == 0) {
      auto default_args_tuple = default_args;
      return call(std::make_index_sequence<sizeof...(Args)>{},
                  default_args_tuple);
    } else {
      return func(args...);
    }
  }

 private:
  template <std::size_t... Is>
  ReturnType call(std::index_sequence<Is...>, std::tuple<Args...>& args_tuple) {
    return func(std::get<Is>(args_tuple)...);
  }
};

// FuncInfo util, How it works: https://godbolt.org/z/WdWWo8zaK

/**
 * For member functions
 */
template <typename R, typename C, typename... Args>
FuncInfo<R, Args...> make_func_info(R (C::*mem_func)(Args...), C* obj,
                                    Args... default_args) {
  return FuncInfo<R, Args...>(
      [=](Args... args) { return (obj->*mem_func)(args...); }, default_args...);
}

// For free functions
template <typename R, typename... Args>
FuncInfo<R, Args...> make_func_info(R (*free_func)(Args...),
                                    Args... default_args) {
  return FuncInfo<R, Args...>(free_func, default_args...);
}
};  // namespace lra::usb_lib
