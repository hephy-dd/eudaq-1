#include <iostream>
#include <stdint.h>

#ifndef MPW3_DEFS_H
#define MPW3_DEFS_H

namespace DefsMpw3 {

  static constexpr std::size_t dimSensorCol = 64;
  static constexpr std::size_t dimSensorRow = 64;

  /* the following methods are used for extraction of different data in word
   * from 32bit EOC-word EOC data format:
   *
   * |31  29 | 28    24 |   23   | 22    16 | 15  8 | 7   0 |
   * |3'b000 | EOC_ADDR | 1'b1/0 | PIX_ADDR | TS_TE | TS_LE |
   *
   * bit 23 might be 1 or 0 to destinguish between base and piggy board
   */

  int inline extractDcol(uint32_t word) { // EOC_ADDR
    return (word & (0x1F << 24)) >> 24;
  };
  int inline extractPix(uint32_t word) { // PIX_ADDR
    return (word & (0x7F << 16)) >> 16;
  };
  int inline extractTsTe(uint32_t word) { // TS_TE
    return (word & (0xFF << 8)) >> 8;
  };
  int inline extractTsLe(uint32_t word) { return word & 0xFF; }; // TS_LE
  int inline extractPiggy(uint32_t word) {
    return (word & (1 << 23)) >> 23;
  }; // piggy or base

} // namespace DefsMpw3

#endif
