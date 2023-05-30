#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3FrameEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("RD50_Mpw3Event");
  static bool foundT0;
};

bool Mpw3FrameEvent2StdEventConverter::foundT0 = false;

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
    Mpw3FrameEvent2StdEventConverter>(
    Mpw3FrameEvent2StdEventConverter::m_id_factory);
}

bool Mpw3FrameEvent2StdEventConverter::Converting(eudaq::EventSPC d1,
                                                  eudaq::StdEventSP d2,
                                                  eudaq::ConfigSPC conf) const {
  double t0 = -1.0;
  if (conf != nullptr) {
    t0 = conf->Get("t0_skip_time", -1.0);
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
    }

    /*
     * there is a head and a tail, 1 word each
     * we do not copy them
     */
    rawdata.resize(block.size() / sizeWord);
    memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

    eudaq::StandardPlane basePlane(0, "Base", "RD50_MPW3_base");
    eudaq::StandardPlane piggyPlane(1, "Piggy", "RD50_MPW3_piggy");
    basePlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
    piggyPlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
    DefsMpw3::word_t sofWord, eofWord;
    DefsMpw3::ts_t minSofOvflw = -1, maxEofOvflw = 0;
    int sofCnt = 0, eofCnt = 0, hitCnt = 0;
    double avgTsLe = 0.0, avgTsTe = 0.0;
    int errorCnt = 0;

    bool insideFrame = false;

    for (const auto &word : rawdata) {
      /*
       * each word represents a particle detection in the specified pixel for a
       * certain ToT.
       */

      DefsMpw3::HitInfo hi(word);
      if (hi.sof) {
        sofWord = hi.initialWord;
        sofCnt++;
        insideFrame = true;
        continue;
      }
      if (hi.eof) {
        eofWord = hi.initialWord;
        eofCnt++;
        if (!insideFrame) {
          EUDAQ_WARN("EOF before SOF");
          // we only except full frames
          // incomplete frames discarded
          errorCnt++;
          //          continue;
        }
        insideFrame = false;
        auto sofOvflw = DefsMpw3::frameOvflw(sofWord, eofWord, false);
        auto eofOvflw = DefsMpw3::frameOvflw(sofWord, eofWord, true);
        minSofOvflw = minSofOvflw > sofOvflw ? sofOvflw : minSofOvflw;
        maxEofOvflw = maxEofOvflw < eofOvflw ? eofOvflw : maxEofOvflw;
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
      uint64_t timeBegin =
          (minSofOvflw * DefsMpw3::dTPerOvflw) * DefsMpw3::lsbTime;
      uint64_t timeEnd =
          (maxEofOvflw * DefsMpw3::dTPerOvflw) * DefsMpw3::lsbTime;

      if (t0 < 0.0) {
        foundT0 = true;
      } else if (timeBegin < uint64_t(t0)) {
        foundT0 = true;
      }

      if (!foundT0) {
        return false;
      }

      d2->SetTimeBegin(timeBegin);
      d2->SetTimeEnd(timeEnd);
      d2->SetTriggerN(d1->GetTriggerN());
    } else {
      EUDAQ_WARN("Not possible to generate timestamp");
      return false;
    }

    d2->SetDescription("RD50_MPW3");
    d2->AddPlane(basePlane);
    d2->AddPlane(piggyPlane);
  }
  return true;
}
