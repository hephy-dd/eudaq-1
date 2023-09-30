#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3Raw2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("RD50_Mpw3Event");
  static bool foundT0Base, foundT0Piggy;
};

bool Mpw3Raw2StdEventConverter::foundT0Base = false;
bool Mpw3Raw2StdEventConverter::foundT0Piggy = false;

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
    Mpw3Raw2StdEventConverter>(Mpw3Raw2StdEventConverter::m_id_factory);
}

bool Mpw3Raw2StdEventConverter::Converting(eudaq::EventSPC d1,
                                           eudaq::StdEventSP d2,
                                           eudaq::ConfigSPC conf) const {

  enum class TimestampMode { TLU, Ovflw };

  TimestampMode tsMode = TimestampMode::Ovflw;

  double t0 = -1.0;
  bool filterZeroWords = true;
  bool weArePiggy = false;
  int tShift = 0;
  if (conf != nullptr) {
    t0 = conf->Get("t0_skip_time", -1.0) * 1e6;
    filterZeroWords = conf->Get("filter_zeros", true);
    tShift = conf->Get("mpw3_tshift", 0);
    weArePiggy = conf->Get("is_piggy", false);
    tsMode = conf->Get("ts_mode", "Ofvlw") == "TLU" ? TimestampMode::TLU
                                                    : TimestampMode::Ovflw;
  }

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  uint64_t timeBegin, timeEnd;

  auto type = ev->GetTag("Type", "Base");
  auto blockIdx = type == "Base" ? 0 : 1;
  // block[0] contains base data
  // block[1] piggy data
  std::vector<uint32_t> rawdata;

  auto block = ev->GetBlock(blockIdx);
  constexpr auto sizeWord = sizeof(uint32_t);
  //    if (block.size() <= 4 * sizeWord) {
  //        //does not contain hit word, just timestamp words
  //      EUDAQ_WARN("empty event");
  //      return false;
  //    }

  rawdata.resize(block.size() / sizeWord);
  memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

  eudaq::StandardPlane basePlane(0, "Base", "RD50_MPW3_base");
  eudaq::StandardPlane piggyPlane(0, "Piggy", "RD50_MPW3_piggy");
  basePlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
  piggyPlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
  int64_t tluTsLsb = -1, tluTsMsb = -1, ovflwCntLsb = -1, ovflwCntMsb = -1;
  int sofCnt = 0, eofCnt = 0, hitCnt = 0;
  std::vector<uint32_t> tsLe;
  double avgTsLe = 0.0, avgTsTe = 0.0;
  int errorCnt = 0;

  bool insideFrame = false;
  DefsMpw3::ts_t frameTs;

  for (const auto &word : rawdata) {
    /*
     * each word represents a particle detection in the specified pixel for a
     * certain ToT.
     */

    if (filterZeroWords && (word == 0 || word == (1 << 23))) {
      /* during UDP pack construction in FW the FIFOs for piggy and base get
       * pulled alternatingly if there is only data left in one of the two,
       * the other one still gets pulled (which results in 0x0 or 0x800000 (
       * bit 23 set to indicate piggy))
       * we might want to skip these words as they would be interpreted as
       * pixel 00:00 with TS-LE = TS-TE = 0
       */
      continue;
    }

    DefsMpw3::HitInfo hi(word);

    // std::cout << std::hex << hi.initialWord << "\n";

    if (hi.sof) {
      ovflwCntLsb = hi.timestamp;
      continue;
    }
    if (hi.eof) {
      ovflwCntMsb = hi.timestamp;
      continue;
    }
    if (hi.isTluTsLsb) {
      tluTsLsb = hi.timestamp;
      continue;
    }
    if (hi.isTluTsMsb) {
      tluTsMsb = hi.timestamp;
      continue;
    }
    avgTsLe += hi.tsLe;
    tsLe.push_back(hi.tsLe);
    avgTsTe += hi.tsTe;
    hitCnt++;
    if (!hi.hitWord) {
      EUDAQ_DEBUG("weird word, should not occur, check code");
      std::cout << std::hex << hi.initialWord << " " << hi.isTluTsLsb << " "
                << hi.isTluTsMsb << " " << hi.isTimestamp << "\n";
      continue;
    }

    if (hi.piggy) {
      piggyPlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, hi.tot);
    } else {
      basePlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, hi.tot);
    }
  }
  /*
   * timestamp for begin / end is being calculated by
   * begin: ovflw Cnt from SOF and minimum TS-LE from all hits in the current
   * frame end: ovflw Cnt from EOF  and maximum TS-TE from all hits in the
   * current frame
   */

  if (tsMode == TimestampMode::Ovflw && ovflwCntLsb >= 0 && ovflwCntMsb >= 0) {
    // std::cout << "sof =  " << sofWord << " eof = " << eofWord << "\n";

    uint32_t minTsLe = 0, maxTsLe = 0;
    if (tsLe.size() > 0) {
      minTsLe = *std::min_element(tsLe.begin(), tsLe.end());
      maxTsLe = *std::max_element(tsLe.begin(), tsLe.end());
    } else {
    }

    timeBegin =
        ((ovflwCntLsb + (ovflwCntMsb << 23)) * DefsMpw3::dTPerOvflw + minTsLe) *
        DefsMpw3::lsbTime;
    timeEnd =
        ((ovflwCntLsb + (ovflwCntMsb << 23)) * DefsMpw3::dTPerOvflw + maxTsLe) *
        DefsMpw3::lsbTime;

  } else if (tsMode == TimestampMode::TLU && tluTsLsb >= 0 && tluTsMsb >= 0) {
    timeEnd = timeBegin = (tluTsLsb + (tluTsMsb << 23)) * DefsMpw3::lsbTime;

  } else {
    EUDAQ_WARN("Not possible to generate timestamp");
    // std::cout << "sofs " << sofCnt << " eofs " << eofCnt << "\n";
    //    std::cout << "failed\n";
    return false;
  }

  if (t0 < 0.0) {
    if (!weArePiggy && type == "Base") {
      foundT0Base = true;
    } else if (type == "Piggy") {
      foundT0Piggy = true;
    }
  } else if (timeBegin < uint64_t(t0)) {
    if (!weArePiggy && type == "Base") {
      foundT0Base = true;
    } else if (type == "Piggy") {
      foundT0Piggy = true;
    }
  }

  if (!foundT0Base && !weArePiggy) {
    return false;
  }
  if (!foundT0Piggy && weArePiggy) {
    return false;
  }

  d2->SetDescription("RD50_MPW3");
  if (basePlane.HitPixels(0) > 0) {
    d2->AddPlane(basePlane);
  }
  if (piggyPlane.HitPixels(0) > 0) {
    d2->AddPlane(piggyPlane);
  }

  // tShift configured in microseconds
  timeBegin += tShift * 1e6;
  timeEnd += tShift * 1e6;

  //  std::cout << "generated event with t = " << timeBegin / 1e6 << "us with ";
  //  if (basePlane.HitPixels(0) > 0) {
  //    std::cout << basePlane.HitPixels(0) << " base ";
  //  }
  //  if (piggyPlane.HitPixels(0) > 0) {
  //    std::cout << piggyPlane.HitPixels(0) << " piggy ";
  //  }
  //  std::cout << "\n";

  d2->SetTimeBegin(timeBegin);
  d2->SetTimeEnd(timeEnd);
  return true;
}
