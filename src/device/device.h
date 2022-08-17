#ifndef LRA_DEVICE_H_
#define LRA_DEVICE_H_

#include <memory/registers/registers.h>
#include <util/errors/errors.h>

namespace lra::device {

// notice
// You should check internal register address type by yourself (see tca.h)

// concept template
using ::lra::memory::registers::is_bitset;
using ::lra::memory::registers::is_register;
using ::lra::memory::registers::same_as_dv;

// struct and variant
using ::lra::memory::registers::Register_16;
using ::lra::memory::registers::Register_32;
using ::lra::memory::registers::Register_64;
using ::lra::memory::registers::Register_8;
using ::lra::memory::registers::Register_T;

// errors
using ::lra::errors_util::Errors;
using ::std::to_underlying;

// util functions
template <typename V>
bool IsNegative(const V& v) {
  if constexpr (std::integral<V>) {
    return (v < 0);
  }

  // bitset will never evaluate as negative number
  return false;
}

}  // namespace lra::device
#endif