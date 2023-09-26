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
word_t inline extractTimestamp(word_t word) { return word & 0x7FFFFF; }

bool inline isSOF(word_t word) {
  return word >> 24 == 0xAF;
} // is given word Start Of Frame?
bool inline isEOF(word_t word) {
  return word >> 24 == 0xE0;
} // is given word End Of Frame?
bool inline isTLUTsLSB(word_t word) { return word >> 24 == 0xBF; }
bool inline isTLUTsMSB(word_t word) { return word >> 24 == 0xF0; }

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
  word_t dcol, pix, tsTe, tsLe, timestamp, initialWord;
  int tot;
  bool sof, eof, piggy, hitWord, isTimestamp, isTluTsLsb, isTluTsMsb;
  PixelIndex pixIdx;
  HitInfo(word_t word) {
    initialWord = word;
    dcol = extractDcol(word);
    pix = extractPix(word);
    tsTe = extractTsTe(word);
    tsLe = extractTsLe(word);
    calcTot();

    sof = isSOF(word);
    eof = isEOF(word);
    isTluTsLsb = isTLUTsLSB(word);
    isTluTsMsb = isTLUTsMSB(word);
    if (isTluTsLsb || isTluTsMsb || sof || eof) {
      isTimestamp = true;
      timestamp = extractTimestamp(word);
    } else {
        isTimestamp = false;
    }
    timestamp = extractTimestamp(word);
    hitWord = !sof & !eof & !isTimestamp;
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
        ss << "ovflwCnt = " << timestamp;
        if (piggy) {
          ss << "   Piggy";
        } else {
          ss << "   Base";
        }
      } else if (eof) {
        ss << "iniWord = " << std::hex << initialWord << std::dec << " EOF ";
        ss << "ovflwCnt = " << timestamp;
        if (piggy) {
          ss << "   Piggy";
        } else {
          ss << "   Base";
        }
      } else if (isTimestamp) {
        ss << "TLU timestamp ";
        if (isTluTsLsb) {
          ss << "LSB = ";
        } else {
          ss << "MSB = ";
        }
        ss << std::hex << initialWord;
      } else {
        ss << "weird word = " << std::hex << initialWord;
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
