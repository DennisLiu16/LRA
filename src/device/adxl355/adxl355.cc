#include <device/adxl355/adxl355.h>

namespace lra::device {
std::string Adxl355::getDeviceRegInfo() { // device 繼承
  constexpr auto regsinfo = lra::memory::registers::_getRegistersInfo(regs_);
  constexpr auto total_bytes = lra::memory::registers::_SumArray(regsinfo.nbytes_);
  constexpr auto total_regs = lra::memory::registers::_SumArray(regsinfo.nregs_);
  constexpr auto overlay_len = lra::memory::registers::_getOverlayLen<total_bytes>(regs_);
  constexpr auto overlay_regs = lra::memory::registers::_getOverlayReg<total_bytes, overlay_len>(regs_);

  std::string s;

  s += spdlog::fmt_lib::format("total bytes: {}\n", total_bytes);
  s += spdlog::fmt_lib::format("total registers: {}\n", total_regs);
  s += spdlog::fmt_lib::format("overlay len: {}\n\n", overlay_len);

  // make a constexpr for (integral only) here
  lra::memory::registers::constexpr_for<0, overlay_regs.size(), 1>([&overlay_regs,&s](auto i) {  // add visit func
    uint64_t addr1 = lra::memory::registers::_VisitAddr(*overlay_regs.at(i).reg1_);
    uint64_t addr2 = lra::memory::registers::_VisitAddr(*overlay_regs.at(i).reg2_);
    constexpr uint8_t v_idx1 = overlay_regs.at(i).reg1_->index();
    constexpr uint8_t v_idx2 = overlay_regs.at(i).reg2_->index();
    std::string name1 = boost::core::demangle(typeid(std::variant_alternative_t<v_idx1, Register_T>).name());
    std::string name2 = boost::core::demangle(typeid(std::variant_alternative_t<v_idx2, Register_T>).name());
    s += spdlog::fmt_lib::format("@: {0:#0x}\n {2}({1:#0x}), {4}({3:#0x})\n", overlay_regs.at(i).overlap_addr_, addr1, name1, addr2,
                     name2);
  });
  // spdlog::fmt_lib::print(spdlog::fmt_lib::runtime(s)); fmt problem solve
  return s;
}
}