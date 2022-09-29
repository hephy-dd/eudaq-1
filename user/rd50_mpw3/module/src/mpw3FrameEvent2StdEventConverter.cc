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

    eudaq::StandardPlane plane(0, "Frame", "RD50_MPW3");
    plane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);

    for (const auto &word : rawdata) {
      /*
       * each word represents a particle detection in the specified pixel for a
       * certain ToT. This converter does not aim to generate a proper global
       * Timestamp. We simply store the hit information of each word in a plane.
       * Therefore meant only for the monitor not for real analysis!!!
       */

      DefsMpw3::HitInfo hi(word);
      if (hi.sof || hi.eof) {
        // we do not care about SOF and EOF here
        continue;
      }
      plane.PushPixel(hi.pixIdx.col, hi.pixIdx.row,
                      hi.tot); // store ToT as "raw pixel value"
    }
    d2->AddPlane(plane);
  }
  return true;
}
