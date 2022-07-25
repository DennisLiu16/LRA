#ifndef LRA_MEMORY_REGISTERS_H_
#define LRA_MEMORY_REGISTERS_H_

// sum up
// - struct inherit with different arguments

#include <util/concepts/concepts.h>

#include <bitset>
#include <cstdint>
#include <functional>
#include <tuple>
#include <variant>


namespace lra::memory::registers {
using std::bitset;
// This is for peripherals (those devices use bus to communicate to)
// Multiple thread may operate the registers.
// So we don't use memory model(store registers value in memory) dircectly.
struct Register {
  const uint64_t addr_;
  const uint8_t bytelen_;

  constexpr Register(const uint64_t &addr, const uint8_t &bytelen) : addr_(addr), bytelen_(bytelen) {}
  // for unordered_map
  bool operator==(const Register &rhs) const { return addr_ == rhs.addr_ && bytelen_ == rhs.bytelen_; }
};

/**
 * @brief Register struct
 * @param dv_ default_value_ in short
 */
struct Register_8 : Register {
  bitset<8> dv_;
  constexpr Register_8(const uint64_t &addr, const bitset<8> &val) : Register(addr, sizeof(uint8_t)), dv_(val) {};
};
struct Register_16 : Register {
  bitset<16> dv_;
  constexpr Register_16(const uint64_t &addr, const bitset<16> &val) : Register(addr, sizeof(uint16_t)), dv_(val) {};
};
struct Register_32 : Register {
  bitset<32> dv_;
  constexpr Register_32(const uint64_t &addr, const bitset<32> &val) : Register(addr, sizeof(uint32_t)), dv_(val) {};
};
struct Register_64 : Register {
  bitset<64> dv_;
  constexpr Register_64(const uint64_t &addr, const bitset<64> &val) : Register(addr, sizeof(uint64_t)), dv_(val) {};
};

// TODO: Register(any_length)

using Register_T = std::variant<Register_8, Register_16, Register_32, Register_64>;

// concepts
template <typename T>
concept is_register = std::convertible_to<T, Register_T>;

template <typename T, typename U>
concept same_as_dv_ = requires (T reg, U val){
  {reg.dv_}->std::same_as<U>;
};

template <typename T, typename U>
concept is_valid_regOperation = requires (T reg, U val) {
  // wrong: because it's ok to do this operation e.g. std::convertible_to<bitset<16>, bitset<8>>;
  // std::convertible_to<U, decltype(reg.dv_)>;

  {val}->std::convertible_to<decltype(reg.dv_)>;
  
  // you can also write something like
  // {val}->std::integral; 
  // equal to std::integral<decltype(val)>; 
  // (but you can't write here, for concept only check validation of expression. No calculation takes place)
};

// TODO:
// create a consteval / constexpr to eval repeat address

}  // namespace lra::device::registers

// hash function should be defined in namespace std
namespace std {
template <lra::memory::registers::is_register T>
struct hash<T> {
  std::size_t operator()(const T &k) const {
    using std::hash;

    // Compute individual hash values for first,
    // second and third and combine them using XOR
    // and bit shifting:

    return (hash<uint64_t>()(k.addr_)) ^ (hash<uint8_t>()(k.bytelen_) << 1);
  }
};
}  // namespace std

#endif