#include <bus/spi/spi.h>
#include <bus_adapter/bus_adapter.h>

// spi driver
#include <linux/spi/spidev.h>

#include <memory>

/**
 * @issue_record:
 * 1. tx_buf 和 rx_buf 可以是 0?
 * 2. tx_buf 和 rx_buf 可以一樣?
 *
 */

/**
 * @spi_info:
 *   len: __u32
 *
 * @notice:
 *   1. You should always memset spi_ioc_transfer to avoid misjudgement
 *   2. 單個 ioctl 使用 SPI_IOC_MESSAGE(N)，N 值受到限制 (in spidev.h)。
 *      該長度限制是 (1 << _IOC_SIZEBITS) == 16384，_IOC_SIZEBITS 被定義為 14。
 *      而 sizeof(struct spi_ioc_transfer) 總共 32 個 bytes，計算下來 N 不可以超過 512。
 *      ref: https://forum.odroid.com/viewtopic.php?t=8842
 */

namespace lra::bus_adapter::spi {
// using
using lra::bus::Spi;

// struct
typedef struct SpiAdapterInit_S {
  bool isMSB_{false};           // 0x12 -> 0x0 ... 0x12 or ->0x12, 0x0 ...
  bool isRWbitFirst_{false};    // R/W | addr or addr | R/W
  uint8_t word_delay_us_{0};    // delay between word
  uint8_t bits_per_word_{8};    // n bits in one word
  uint16_t delay_us_{0};        // delay between spi transfer
  uint32_t flags_{0};           // see linux/spi/spidev.h, includeing all flags and mode
  uint32_t speed_hz_{1000000};  // default to 1M/sec
  const char* name_{"/dev/spidev0.0"};
  std::shared_ptr <Spi> bus_{nullptr};
} SpiAdapter_S;

// class
class SpiAdapter : public BusAdapter<SpiAdapter> {
 public:
  // external R/W functions
  ssize_t Xfer(uint8_t* val, const uint32_t& len);

  ssize_t XferMulti(uint8_t** const buf_arr, const uint32_t* const len_arr,
                    const bool* const chip_select_arr, uint16_t len_of_arr /* should make this < 512 */);

 private:
  friend BusAdapter;

  bool InitImpl(const SpiAdapterInit_S& init_s);

  // single register write
  template <is_register T, std::integral U>
  ssize_t WriteImpl(const T& reg, const U& val) {}

  // array write
  ssize_t WriteImpl(const uint64_t& iaddr, const uint8_t* val, const uint32_t& len);

  // vector write
  ssize_t WriteImpl(const uint64_t& iaddr, const std::vector<uint8_t>& val);

  // single register read
  template <is_register T, std::integral U>
  ssize_t ReadImpl(const T& reg, U& val) {}

  // read one byte
  // use auto& instead of uint8_t& to enable integral type bind on val_r
  ssize_t ReadImpl(const uint64_t& iaddr, std::integral auto& val_r) {}

  // read to array
  ssize_t ReadImpl(const uint64_t& iaddr, uint8_t* val, const uint32_t& len);

  // generate spi_ioc_transfer struct according to this SPI adapter settings (modify by Init(...))
  // properties in SpiAdapterInit_S will be set in this functions
  spi_ioc_transfer GenSpiIocXfer();

  SpiAdapter_S info_{0};
};
}  // namespace lra::bus_adapter::spi