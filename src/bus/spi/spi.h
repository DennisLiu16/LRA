#ifndef LRA_BUS_SPI_H_
#define LRA_BUS_SPI_H_

#include <bus/bus.h>

namespace lra::bus {
struct SpiInit_S {};

class Spi : public Bus<Spi> {
  public:

  enum class SpiMethod {kIoc};

  private:
  friend Bus;
};

}  // namespace lra::bus

#endif