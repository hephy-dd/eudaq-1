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

  enum class TimestampMode { Base, Piggy };

  TimestampMode tsMode = TimestampMode::Base;
  auto tag = d1->GetTag("syncMode");
  if (tag == "1") {
    tsMode = TimestampMode::Base;

  } else if (tag == "2") {
    tsMode = TimestampMode::Piggy;
  }

  double t0 = -1.0;
  bool filterZeroWords = true;
  bool weArePiggy = false;
  int tShift = 0;
  if (conf != nullptr) {
    t0 = conf->Get("t0_skip_time", -1.0);
    filterZeroWords = conf->Get("filter_zeros", true);
    tShift = conf->Get("mpw3_tshift", 0);
    weArePiggy = conf->Get("is_piggy", false);
  }

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  for (int i = 0; i < ev->NumBlocks(); i++) {
    std::vector<uint32_t> rawdata;

    auto block = ev->GetBlock(i);
    constexpr auto sizeWord = sizeof(uint32_t);
    if (block.size() <= 2 * sizeWord) {
      // no hits in event just trigger word,
      // actually not usable, but don't fail for debug purposes
      EUDAQ_WARN("empty event");
      return false;
    }

    /*
     * there is a head and a tail, 1 word each
     * we do not copy them
     */
    rawdata.resize(block.size() / sizeWord);
    memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

    eudaq::StandardPlane basePlane(0, "Base", "RD50_MPW3_base");
    eudaq::StandardPlane piggyPlane(0, "Piggy", "RD50_MPW3_piggy");
    basePlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
    piggyPlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
    DefsMpw3::word_t sofWord, eofWord;
    int sofCnt = 0, eofCnt = 0, hitCnt = 0;
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
      if (hi.sof) {
        if ((tsMode == TimestampMode::Base && hi.piggy) ||
            (tsMode == TimestampMode::Piggy && !hi.piggy)) {
          continue;
        }
        sofWord = hi.initialWord;
        sofCnt++;
        insideFrame = true;
        continue;
      }
      if (hi.eof) {
        if ((tsMode == TimestampMode::Base && hi.piggy) ||
            (tsMode == TimestampMode::Piggy && !hi.piggy)) {
          continue;
        }

        eofWord = hi.initialWord;
        eofCnt++;
        if (!insideFrame) {
          //          EUDAQ_WARN("EOF before SOF");
          // we only except full frames
          // incomplete frames discarded
          errorCnt++;
          //          continue;
        }
        insideFrame = false;
        frameTs = DefsMpw3::frameTimestamp(sofWord, eofWord);
        eofCnt++;
        continue;
      }
      if (hi.triggerNmb > 0) {
        // trigger numbers are alrdy in raw eudaq event, don't deal with them
        // here anymore
        continue;
      }
      avgTsLe += hi.tsLe;
      avgTsTe += hi.tsTe;
      hitCnt++;

      if (hi.piggy) {
        piggyPlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, hi.tot);
      } else {
        basePlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, hi.tot);
      }
    }
    /*
     * timestamp for begin / end is being calculated by
     * begin: ovflw Cnt from SOF and average TS-LE from all hits in the current
     * frame end: ovflw Cnt from EOF  and average TS-TE from all hits in the
     * current frame
     */
    if (sofCnt > 0 && eofCnt > 0) {
      uint64_t timeBegin = frameTs * DefsMpw3::lsbTime + tShift * 1e6;
      uint64_t timeEnd = timeBegin;
      //(maxEofOvflw * DefsMpw3::dTPerOvflw) * DefsMpw3::lsbTime;

      
      if (t0 < 0.0) {
        if (!weArePiggy) {
        foundT0Base = true;
        } else {
          foundT0Piggy = true;
        }
      } else if (timeBegin < uint64_t(t0)) {
        if (!weArePiggy) {

        foundT0Base = true;
        } else {
          foundT0Piggy = true;
        }
      }

      if (!foundT0Base && !weArePiggy) {
        return false;
      }
      if (!foundT0Piggy && weArePiggy) {
        return false;
      }

      d2->SetTimeBegin(timeBegin);
      d2->SetTimeEnd(timeEnd);
      d2->SetTriggerN(d1->GetTriggerN());
    } else {
      EUDAQ_WARN("Not possible to generate timestamp");
      //std::cout << "sofs " << sofCnt << " eofs " << eofCnt << "\n";

      return false;
    }

    d2->SetDescription("RD50_MPW3");
    d2->AddPlane(basePlane);
    d2->AddPlane(piggyPlane);
  }
  return true;
}
