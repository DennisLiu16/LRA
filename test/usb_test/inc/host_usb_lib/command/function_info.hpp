/*
 * File: function_info.hpp
 * Created Date: 2023-04-02
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Monday April 10th 2023 5:59:51 pm
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
};  // namespace lra::usb_lib
