#include "../tools/Defs.h"
#include <iomanip>
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

static constexpr ts_t lsbTime =
    50000; // number of picoseconds 1 LSB timestamp accounts for
static constexpr ts_t dTPerTsLsb =
    1; // we want to stay in LSB at this stage, could be used to perform
       // calculation in actual time units
static constexpr ts_t dTPerOvflw =
    dTPerTsLsb *
    256; // number of LSB per timestamp-overflow (integer from FPGA)
static constexpr ts_t dTSameEvt =
    0; // the timewindow in LSB in which multiple hits are considered to
       // belong to the same event
static constexpr ts_t dtPerOvflwOfOvwfl = dTPerOvflw * 256;
// ovflCnt comes with 8 bit, 1 ovflw ~ 12.8us => 1 ovflw of the ovflw ~ 3.278
// ms
static constexpr ts_t lsbToleranceOvflwDcol =
    5; // there's an ovflw detection within a dcol, done by comparing TSLE of
       // two consecutive hits. Whenever difference between these is bigger than
       // the here defined tolerance, we claim we encountered an ovflw

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
  return ((word >> 24) & 0x1F);
}
word_t inline extractPix(word_t word) { // PIX_ADDR
  return ((word >> 16) & 0x7F);
}
word_t inline extractTsTe(word_t word) { // TS_TE
  return (word >> 8) & 0xFF;
}
word_t inline extractTsLe(word_t word) { return word & 0xFF; } // TS_LE
word_t inline extractPiggy(word_t word) {
  return (word >> 23) & 0x01;
} // piggy or base
word_t inline extractOverFlowCnt(word_t word) { return word & 0x7FFFFF; }
word_t inline extractTriggerNmb(word_t word) { return word & 0xFFFF; }
auto inline frameTimestamp(ts_t sof, ts_t eof) {

  /*
   * To allow an overflowCounter of 38bit width it is split into SOF and EOF
   * words in the following manner:
   * SOF:
   * 0xaf piggy/base -1-> [22..0] <-1-
   * EOF:
   * 0xE0 piggy/base -1->[37..23]<-1- -2->[7..0] <-2-
   *
   * The values marked in -1->x<-1- is the 38 bit ovflw counter
   * [22..0] means bit 0 -> 22 of the 38 bit ovflw,
   * these have to be combined with bit 37 - 23 in [37..23] to generate full 38
   * bit
   *
   * In -2->y<-2- the value corresponds to the value of the 8 LSBs of the
   * FPGA internal ovflwCnt at the time the EOF  arrived piggy/base is one bit
   * to distingiush between data from piggy and base, not needed here
   */
  sof &= 0x7FFFFF; // upper 9 bit distinguish only SOF / EOF
  eof &= 0x7FFFFF;
  DefsMpw3::ts_t frameTs = sof;
  frameTs |= (eof << 23);
  return frameTs;
}

bool inline isSOF(word_t word) {
  return word >> 24 == 0xAF;
} // is given word Start Of Frame?
bool inline isEOF(word_t word) {
  return word >> 24 == 0xE0;
} // is given word End Of Frame?
bool inline isTrigger(word_t word) {
  return (word >> 16 & 0xFF7F) == 0xFF7F;
} // works for both trigger from Piggy and Base FIFO

static inline PixelIndex dColIdx2Pix(size_t dcol, size_t pix) {
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

struct HitInfo {
  word_t dcol, pix, tsTe, tsLe, OvFlwCnt, initialWord;
  int tot, triggerNmb = -1;
  bool sof, eof, piggy, hitWord;
  PixelIndex pixIdx;
  HitInfo(word_t word) {
    initialWord = word;
    dcol = extractDcol(word);
    pix = extractPix(word);
    tsTe = extractTsTe(word);
    tsLe = extractTsLe(word);
    calcTot();

    OvFlwCnt = extractOverFlowCnt(word);
    if (isTrigger(word)) {
      triggerNmb = int(extractTriggerNmb(word));
      hitWord = false;
      eof = sof = false;
    }
    sof = isSOF(word);
    eof = isEOF(word);
    hitWord = !sof & !eof & !isTrigger(word);
    piggy = extractPiggy(word) > 0 ? true : false;
    pixIdx = dColIdx2Pix(dcol, pix);
  }
  HitInfo() {}

  inline std::string toStr() {
    std::stringstream ss;
    if (hitWord) {
      ss << "iniWord = " << std::hex << initialWord << std::dec << " piggy? "
         << piggy << " idx = " << std::setw(2) << std::setfill('0')
         << pixIdx.row << ":" << std::setw(2) << std::setfill('0') << pixIdx.col
         << " TSTE = " << std::setw(3) << std::setfill('0') << tsTe
         << " TSLE = " << std::setw(3) << std::setfill('0') << tsLe
         << " Tot = " << std::setw(3) << std::setfill('0') << tot;
    } else {
      if (sof) {
        ss << "iniWord = " << std::hex << initialWord << std::dec << " SOF ";
        ss << "ovflwCnt = " << OvFlwCnt;
        if (piggy) {
          ss << "   Piggy";
        } else {
          ss << "   Base";
        }
      } else if (eof) {
        ss << "iniWord = " << std::hex << initialWord << std::dec << " EOF ";
        ss << "ovflwCnt = " << OvFlwCnt;
        if (piggy) {
          ss << "   Piggy";
        } else {
          ss << "   Base";
        }
      } else {
        ss << "iniWord = " << std::hex << initialWord << std::dec
           << " triggerNmb = " << triggerNmb;
        if (piggy) {
          ss << "   Piggy";
        } else {
          ss << "   Base";
        }
      }
    }
    return ss.str();
  }

  inline void generateGlobalTs(uint32_t ovflwCnt) {
    tsTe += 256 * ovflwCnt;
    tsLe += 256 * ovflwCnt;
    calcTot();
  }
  inline void calcTot() {
    tot = tsLe <= tsTe
              ? int(tsTe) - int(tsLe)
              : 256 + int(tsTe) - int(tsLe); // account for possible overflows
  }
};

} // namespace DefsMpw3

#endif
