#ifndef LRA_UTIL_CONCEPTS_H_
#define LRA_UTIL_CONCEPTS_H_

#include <concepts>
namespace lra::concepts_util {  // general concepts

// constexpr
template <typename T, typename... Ts>
constexpr bool contains = (std::is_same<T, Ts>{} || ...);

template <typename Subset, typename Set>
constexpr bool is_subset_of = false;

template <typename... Ts, typename... Us>
constexpr bool is_subset_of<std::tuple<Ts...>, std::tuple<Us...>> = (contains<Ts, Us...> && ...);

// concepts

// SameType
template <typename T, typename U>
concept SameType = std::is_same_v<T, U>;

// Included, T in Ts...
template <typename T, typename... Ts>
concept Contains = (SameType<T, Ts> || ...);

// IsSubsetOf
// ref: https://stackoverflow.com/a/42581655/17408307
template <typename T, typename U>
concept IsSubsetOf = is_subset_of<T, U>;

// T is derived class of U
// e.g.
// template<Derived<Base> T>
// void f(T); // T is constrained by Derived<T, Base>
template <class T, class U>
concept Derived = std::is_base_of_v<U, T>;


}  // namespace lra::concepts_util

#endif