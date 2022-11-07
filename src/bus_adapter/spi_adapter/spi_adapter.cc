#include <bus_adapter/spi_adapter/spi_adapter.h>

// TODO: Implement it, also spi bus

namespace lra::bus_adapter::spi {

ssize_t SpiAdapter::Xfer(uint8_t* val, const uint32_t& len) {
  spi_ioc_transfer xfer = GenSpiIocXfer();

  // TODO: find out a better way to avoid memcpy
  xfer.len = len;
  xfer.rx_buf = (unsigned long)val;
  xfer.tx_buf = (unsigned long)val;

  info_.bus_->Write<Spi::SpiMethod::kIoc>(&xfer);
}

ssize_t SpiAdapter::XferMulti(uint8_t** buf_arr, const uint32_t* const len_arr,
                  const bool* const chip_select_arr, uint16_t len_of_arr /* should make this < 512 */){}

// private
bool SpiAdapter::InitImpl(const SpiAdapterInit_S& init_s){

}

ssize_t SpiAdapter::WriteImpl(const uint64_t& iaddr, const uint8_t* val, const uint32_t& len){
  // decide addr len
  uint8_t addr_len = FindAddrLen(iaddr);


  // create a buf 
  uint8_t buf[len + sizeof(uint64_t)];

}

ssize_t SpiAdapter::WriteImpl(const uint64_t& iaddr, const std::vector<uint8_t>& val){}

ssize_t SpiAdapter::ReadImpl(const uint64_t& iaddr, uint8_t* val, const uint32_t& len){}

spi_ioc_transfer SpiAdapter::GenSpiIocXfer(){
  struct spi_ioc_transfer xfer;
  memset(&xfer, 0, sizeof(struct spi_ioc_transfer));

  xfer.bits_per_word = info_.bits_per_word_;
  xfer.delay_usecs = info_.delay_us_;
  xfer.word_delay_usecs = info_.word_delay_us_;
  xfer.speed_hz = info_.speed_hz_;

  return xfer;
}

}  // namespace lra::bus_adapter::spi