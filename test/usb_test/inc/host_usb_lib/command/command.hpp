/*
 * File: command.hpp
 * Created Date: 2023-04-11
 * Author: Dennis Liu
 * Contact: <liusx880630@gmail.com>
 *
 * Last Modified: Thursday July 6th 2023 5:29:37 pm
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
#include <drv_stm_lib/lra_usb_defines.h>

#include <host_usb_lib/cdcDevice/rcws_info.hpp>
#include <variant>

#include "command_impl.hpp"
#include "function_info.hpp"

namespace lra::usb_lib {

/* std::variant util */
template <typename... Ts>
struct variant_cat_s;

template <typename... As, typename... Bs, typename... Rest>
struct variant_cat_s<std::variant<As...>, std::variant<Bs...>, Rest...> {
  using type =
      typename variant_cat_s<std::variant<As..., Bs...>, Rest...>::type;
};

template <typename... As, typename... Bs>
struct variant_cat_s<std::variant<As...>, std::variant<Bs...>> {
  using type = std::variant<As..., Bs...>;
};

template <typename... Ts>
using variant_cat = typename variant_cat_s<Ts...>::type;

/* defined rcws command vector */
using StringCmdType =
    std::variant<std::monostate, Command<FuncInfo<void, std::string>>>;

using UiCmdType = variant_cat<StringCmdType, RcwsCmdType>;
}  // namespace lra::usb_lib
