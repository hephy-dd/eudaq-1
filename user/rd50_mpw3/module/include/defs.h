#include "../tools/Defs.h"
#include <iostream>
#include <stdint.h>

#ifndef MPW3_DEFS_H
#define MPW3_DEFS_H

namespace DefsMpw3 {

  struct PixelIndex {
    int col;
    int row;
  };

  using word_t = SVD::Defs::VMEData_t;
  using ts_t = uint64_t;

  static constexpr std::size_t dimSensorCol = 64;
  static constexpr std::size_t dimSensorRow = 64;

  static constexpr ts_t dTPerTsLsb =
      25; // number of [ns] per LSB in TS_LE and TS_TE
  static constexpr ts_t dTPerOvflw =
      dTPerTsLsb *
      256; // number of [ns] per timestamp-overflow (integer from FPGA)
  static constexpr ts_t dTSameEvt =
      0; // the timewindow in [ns] in which multiple hits are considered to
         // belong to the same event
  static constexpr ts_t dtPerOvflwOfOvfl = dTPerOvflw * (1 << 23);
  // ovflCnt comes with 23 bit, 1 ovflw ~ 6.4us => 1 ovflw of the ovflw ~ 54s

  /* the following methods are used for extraction of different data in a 32bit
   * EOC-word
   * EOC data format:
   *
   * |31  29 | 28    24 |   23   | 22    16 | 15  8 | 7   0 |
   * |3'b000 | EOC_ADDR | 1'b1/0 | PIX_ADDR | TS_TE | TS_LE |
   *
   * bit 23 might be 1 or 0 to destinguish between base and piggy board
   */

  word_t inline extractDcol(word_t word) { // EOC_ADDR
    return (word & (0x1F << 24)) >> 24;
  };
  word_t inline extractPix(word_t word) { // PIX_ADDR
    return (word & (0x7F << 16)) >> 16;
  };
  word_t inline extractTsTe(word_t word) { // TS_TE
    return (word & (0xFF << 8)) >> 8;
  };
  word_t inline extractTsLe(word_t word) { return word & 0xFF; }; // TS_LE
  word_t inline extractPiggy(word_t word) {
    return (word & (1 << 23)) >> 23;
  }; // piggy or base
  word_t inline extractOverFlowCnt(word_t word) {
    return word & ~(0x1FF << 23);
  }

  bool inline isSOF(word_t word) {
    return ((word >> 24) & 0xAF) == 0xAF;
  } // is given word Start Of Frame?
  bool inline isEOF(word_t word) {
    return ((word >> 24) & 0xE0) == 0xE0;
  } // is given word End Of Frame?

  static inline PixelIndex dColIdx2Pix(int dcol, int pix) {
    /*
     * The pixel in a double column are enumerated in a snake-like pattern
     * eg dcol 0 for pixel 1 -> 5:
     * numbers represent the "user pixel name" row/col
     *
     * 2/0 -> ..
     *  ^
     *  |
     * 1/0 <- 1/1
     *         ^
     *         |
     * 0/0 -> 0/1
     *
     * 0/0 therefore is pixel 0 in dcol 0 and 2/0 is pixel 4 in dcol 0
     * 2/4 would be pixel 4 in dcol 2
     */
    PixelIndex retval;

    retval.row = pix / 2;
    retval.col = dcol * 2;
    /*
     * The snake like pattern forms blocks of 4
     * modulo division gets us index in the block
     */
    switch (pix % 4) {
    case 0:
    case 3:
      // alrdy at correct index, do nothing
      break;
    case 1:
    case 2:
      // corresponds to 0/1, 1/1 from example above
      // therefore column needs to be switched
      retval.col += 1;
      break;
    }

    return std::move(retval);
  }

} // namespace DefsMpw3

#endif
