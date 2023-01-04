#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3FrameEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Mpw3FrameEvent");
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Mpw3FrameEvent2StdEventConverter>(
      Mpw3FrameEvent2StdEventConverter::m_id_factory);
}

bool Mpw3FrameEvent2StdEventConverter::Converting(eudaq::EventSPC d1,
                                                  eudaq::StdEventSP d2,
                                                  eudaq::ConfigSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  for (int i = 0; i < ev->NumBlocks(); i++) {
    std::vector<uint32_t> rawdata;

    auto block = ev->GetBlock(i);
    constexpr auto sizeWord = sizeof(uint32_t);
    if (block.size() <= 2 * sizeWord) {
      // not even head and tail present, this definitely is bullshit
      EUDAQ_WARN("invalid block size, skipping event");
      return false;
    }

    /*
     * there is a head and a tail, 1 word each
     * we do not copy them
     */
    rawdata.resize(block.size() / sizeWord);
    memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

    eudaq::StandardPlane plane(0, "Frame", "RD50_MPW3_frame");
    plane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
    DefsMpw3::word_t sofWord, eofWord;
    int sofCnt = 0, eofCnt = 0, hitCnt = 0;
    double avgTsLe = 0.0, avgTsTe = 0.0;

    for (const auto &word : rawdata) {
      /*
       * each word represents a particle detection in the specified pixel for a
       * certain ToT. This converter does not aim to generate a proper global
       * Timestamp. We simply store the hit information of each word in a plane.
       * Therefore meant only for the monitor not for real analysis!!!
       */

      DefsMpw3::HitInfo hi(word);
      if (hi.sof) {
        sofWord = hi.initialWord;
        sofCnt++;
        continue;
      }
      if (hi.eof) {
        eofWord = hi.initialWord;
        eofCnt++;
        continue;
      }
      avgTsLe += hi.tsLe;
      avgTsTe += hi.tsTe;
      hitCnt++;
      plane.PushPixel(hi.pixIdx.col, hi.pixIdx.row,
                      hi.tot); // store ToT as "raw pixel value"
    }
    avgTsLe /= double(hitCnt);
    avgTsTe /= double(hitCnt);
    if (avgTsTe < avgTsLe) {
      // tailing edgz < leading edge => should be an overflow
      avgTsTe += DefsMpw3::dTPerOvflw;
    }
    /*
     * timestamp for begin / end is being calculated by
     * begin: ovflw Cnt from SOF and average TS-LE from all hits in the current
     * frame end: ovflw Cnt from EOF  and average TS-TE from all hits in the
     * current frame
     */
    if (sofCnt == 1 && eofCnt == 1) {
      auto sofOvflw = DefsMpw3::frameOvflw(sofWord, eofWord, false);
      auto eofOvflw = DefsMpw3::frameOvflw(sofWord, eofWord, true);
      d2->SetTimeBegin((sofOvflw * DefsMpw3::dTPerOvflw + avgTsLe) *
                       DefsMpw3::lsbTime);
      d2->SetTimeEnd((eofOvflw * DefsMpw3::dTPerOvflw + avgTsTe) *
                     DefsMpw3::lsbTime);
      //      std::cout << "set time begin = " << d2->GetTimeBegin()
      //                << " end = " << d2->GetTimeEnd() << "\n";
    } else {
      EUDAQ_WARN("Not exactly 1 SOF and EOF in frame");
      return false;
    }

    d2->SetDescription("RD50_MPW3_frame");
    d2->AddPlane(plane);

    // for these "rough" events based on frames we simply use the receive
    // timestamp, which comes in [us], Corry wants [ps]

    //    uint64_t ts = d1->GetTag("recvTS", 0);
    //    d2->SetTimeBegin(ts * 1e6);
    //    d2->SetTimeEnd(ts * 1e6);

    //    d2->SetTimeBegin(cpuTsInt);
    //    d2->SetTimeEnd(cpuTsInt);
  }
  //  std::cout << "valid event\n";
  return true;
}
