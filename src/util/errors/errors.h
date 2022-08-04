#ifndef LRA_UTIL_ERRORS_H_
#define LRA_UTIL_ERRORS_H_

#include <type_traits>

namespace lra::errors_util {
// error numbers
enum class Errors { kOk = 0, kWrite = -1, kRead = -2, kModify = -3 };
}  // namespace lra::errors_util

namespace std {
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept {
  return static_cast<typename std::underlying_type<E>::type>(e);
}
}  // namespace std

#endif