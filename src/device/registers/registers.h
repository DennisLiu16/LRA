#ifndef LRA_DEVICE_REGISTERS_H_
#define LRA_DEVICE_REGISTERS_H_

#include <tuple>
#include <util/concepts/concepts.h>
namespace lra::device::registers { 
// This is for Peripherals (those you use bus to communicate to)
// So you can't use memory model dircectly

// register define
// No need of volatile cause it's a const internal address
// 
// usage:
// e.g. Register_16 a = 0x12; express register a's address from 0x12 to 0x13 (2 bytes)

typedef unsigned char Register_8;
typedef unsigned short Register_16;
typedef unsigned int Register_32;
typedef unsigned long long Register_64;
using Register_T = std::tuple<Register_8, Register_16, Register_32, Register_64>;

//concepts
template <typename T>
concept IsRegister =  lra::concepts_util::IsSubsetOf<std::tuple<T>, Register_T>;

}  // namespace lra::device::registers
#endif