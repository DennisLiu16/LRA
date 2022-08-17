#ifndef LRA_MEMORY_REGISTERS_H_
#define LRA_MEMORY_REGISTERS_H_

// sum up
// - struct inherit with different arguments

#include <util/concepts/concepts.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <variant>

// TODO: V2 - make Register_T like
// Register_8 reg{
//    Field2 field_name_1(start, dv);
//    Field2 field_name_2(start, dv);
//    Field2 field_name_3[2](start, dv); //
// }

// see this link --> how to use registers.h
// https://godbolt.org/z/j5axKK46P

namespace lra::memory::registers {
using std::bitset;
// This is for peripherals (those devices use bus to communicate to)
// Multiple thread may operate the registers.
// So we don't use memory model(store registers value in memory) dircectly.
struct Register {
  const uint64_t addr_;
  const uint8_t bytelen_;  // data length

  constexpr Register(const uint64_t &addr, const uint8_t &bytelen) : addr_(addr), bytelen_(bytelen) {}
  // for unordered_map
  consteval bool operator==(const Register &rhs) const { return addr_ == rhs.addr_ && bytelen_ == rhs.bytelen_; }
  consteval uint64_t operator()() const { return addr_; }
};

/**
 * @brief Register struct
 * @param dv_ default_value_ in short
 */
struct Register_8 : Register {
  bitset<8> dv_;
  typedef uint8_t val_t;
  constexpr Register_8(const uint64_t &addr, const bitset<8> &val) : Register(addr, sizeof(uint8_t)), dv_(val){};
};
struct Register_16 : Register {
  bitset<16> dv_;
  typedef uint16_t val_t;
  constexpr Register_16(const uint64_t &addr, const bitset<16> &val) : Register(addr, sizeof(uint16_t)), dv_(val){};
};
struct Register_32 : Register {
  bitset<32> dv_;
  typedef uint32_t val_t;
  constexpr Register_32(const uint64_t &addr, const bitset<32> &val) : Register(addr, sizeof(uint32_t)), dv_(val){};
};
struct Register_64 : Register {
  bitset<64> dv_;
  typedef uint64_t val_t;
  constexpr Register_64(const uint64_t &addr, const bitset<64> &val) : Register(addr, sizeof(uint64_t)), dv_(val){};
};

// TODO: Register(any_length)

using Register_T = std::variant<Register_8, Register_16, Register_32, Register_64>;

consteval auto _VisitAddr(const Register_T &reg) {
  return std::visit([](auto reg) { return reg.addr_; }, reg);
}

consteval auto _VisitBytelen(const Register_T &reg) {
  return std::visit([](auto reg) { return reg.bytelen_; }, reg);
}

// util
template <std::integral T, std::size_t N>
consteval uint64_t _SumArray(const std::array<T, N> &array) {
  uint64_t total{0};

  for (auto &element : array) {
    total += element;
  }

  return total;
}

// overlay record data struct
struct OverlayRegs {
  const Register_T *reg1_{nullptr};
  const Register_T *reg2_{nullptr};
  uint64_t overlap_addr_{0};
  constexpr OverlayRegs(){};
  constexpr OverlayRegs(const Register_T *reg1, const Register_T *reg2, uint64_t overlap_addr)
      : reg1_(reg1), reg2_(reg2), overlap_addr_(overlap_addr) {}

  constexpr bool operator<(const OverlayRegs &other) const { return overlap_addr_ < other.overlap_addr_; }
};

struct RegistersInfo {
  const std::array<uint64_t, std::variant_size_v<Register_T>> nbytes_;  // 紀錄每種 Register Type 對應的 byte 數
  const std::array<uint64_t, std::variant_size_v<Register_T>> nregs_;  // 紀錄每種 Register Type 對應的 register 數

  constexpr RegistersInfo(const auto nbytes, const auto nregs) : nbytes_(nbytes), nregs_(nregs) {}
};

consteval auto _getRegistersInfo(const auto &arr) {
  // get total reg bytes
  std::array<uint64_t, std::variant_size_v<Register_T>> n_bytes{0};
  std::array<uint64_t, std::variant_size_v<Register_T>> n_regs{0};

  for (auto &reg : arr) {
    // get bytelen
    auto bytelen = _VisitBytelen(reg);
    n_bytes[reg.index()] += bytelen;

    // calculate regs number corresponding to type
    n_regs[reg.index()] += 1;
  }

  return RegistersInfo(n_bytes, n_regs);
}

/*
@param arr std::array<Register_T, size_t N>, from device

@tparam LEN Sum of RegistersInfo.nbytes_
*/
template <uint64_t LEN>
consteval uint64_t _getOverlayLen(const auto &arr) {
  std::array<uint64_t, LEN> addr_overlap{0};  // 儲存可能衝突地址
  std::array<uint16_t, LEN> valid{0};         // 衝突次數陣列

  uint64_t overlay_num{0};
  uint64_t global_idx{0};

  for (auto &reg : arr) {
    auto bytelen = _VisitBytelen(reg);
    auto start_addr = _VisitAddr(reg);

    // write all address controlled by reg to addr_overlap
    for (auto i = 0; i < bytelen; i++) {
      // Addr already exists?
      if (auto it = std::find(addr_overlap.begin(), addr_overlap.end(), start_addr + i),
          valid_idx = it - addr_overlap.begin();
          it != addr_overlap.end() && valid.at(valid_idx)) {  // 有找到且有效 (非初始值，主要預防地址 0x0 誤判)

        overlay_num += valid.at(valid_idx);
        valid.at(valid_idx)++;

      } else {
        addr_overlap.at(global_idx) = start_addr + i;
        valid.at(global_idx++) = 1;
      }
    }
  }

  return overlay_num;
}

/**
@tparam LEN: Sum of RegistersInfo.nbytes_
@tparam OVERLAY_LEN: From _getOverlayLen, number of pairs of overlayed registers

@return: array<OverlayRegs>
*/
template <uint64_t LEN, uint64_t OVERLAY_LEN>
consteval auto _getOverlayReg(const auto &arr) {
  std::array<uint64_t, LEN> addr_overlap_arr;
  std::array<const Register_T *, LEN> reg1_arr{nullptr};
  std::array<OverlayRegs, OVERLAY_LEN> regs_overlap_arr;

  uint64_t overlay_idx{0};
  uint64_t global_idx{0};
  const auto begin = addr_overlap_arr.begin();
  const auto end = addr_overlap_arr.end();

  for (auto &reg : arr) {  // 待修改演算法和儲存結構 (有序...)
    auto bytelen = _VisitBytelen(reg);
    auto start_addr = _VisitAddr(reg);

    for (auto i = 0; i < bytelen; i++) {
      // only address overlaping will enter this loop
      // 1. begin + global_idx -> 限制了有效的長度 (不用每次都找超過有效的區域)
      // 2. itr++ -> 找的起點往後偏移一個，避免卡在迴圈
      for (auto itr = begin;
           (itr = std::find(itr, begin + global_idx, start_addr + i)) != end &&  // find overlaping address
           reg1_arr.at(itr - begin);                                             // not nullptr
           itr++) {
        auto overlap_addr = *itr;
        auto idx = itr - begin;

        // generate OverlayRegs struct
        regs_overlap_arr.at(overlay_idx++) = OverlayRegs(reg1_arr.at(idx), &reg, overlap_addr);
      }
      addr_overlap_arr.at(global_idx) = start_addr + i;
      reg1_arr.at(global_idx++) = &reg;
    }
  }
  // sort here
  std::sort(regs_overlap_arr.begin(), regs_overlap_arr.end());
  return regs_overlap_arr;
}

// constexpr for impl
// ref: https://artificial-mind.net/blog/2020/10/31/constexpr-for
// 透過 lambda 和模板實現
// TODO: move to util/util.h
template <auto Start, auto End, auto Inc, class F>
constexpr void constexpr_for(F &&f) {
  if constexpr (Start < End) {
    f(std::integral_constant<decltype(Start), Start>());
    constexpr_for<Start + Inc, End, Inc>(f);
  }
}

// concepts

template <typename T>
concept is_register = std::convertible_to<T, Register_T>;

// same as default value
template <typename T, typename U>
concept same_as_dv = requires(T reg, U val) {
  { reg.dv_ } -> std::same_as<U>;
};

template <typename T, typename U>
concept convertible_to_dv_type = requires(T reg, U val) {
  // wrong: because it's ok to do this operation e.g. std::convertible_to<bitset<16>, bitset<8>>;
  // std::convertible_to<U, decltype(reg.dv_)>;

  { val } -> std::convertible_to<decltype(reg.dv_)>;

  // you can also write something like
  // {val}->std::integral;
  // equal to std::integral<decltype(val)>;
  // (but you can't write here, for concept only check validation of expression. No calculation takes place)
};

template <typename T>
concept is_bitset = requires(T bitset) {
  bitset.set();
  bitset.reset();
  bitset.flip();
  bitset.all();
  bitset.any();
  bitset.none();
  bitset.count();
  bitset.size();
  bitset.to_string();
  bitset.to_ulong();
  bitset.to_ullong();
};

// TODO: create a consteval / constexpr to eval repeat address

}  // namespace lra::memory::registers

#endif