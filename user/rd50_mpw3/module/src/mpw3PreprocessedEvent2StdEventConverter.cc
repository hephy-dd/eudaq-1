#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3PreprocessedEvent2StdEventConverter
    : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory =
      eudaq::cstr2hash("MPW3PreprocessedEvent");
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Mpw3PreprocessedEvent2StdEventConverter>(
      Mpw3PreprocessedEvent2StdEventConverter::m_id_factory);
}

bool Mpw3PreprocessedEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  //  std::cout << "converting MPW3FrameEvt\n" << std::flush;

  int planes = 0;

  for (int i = 0; i < ev->NumBlocks(); i++) {
    std::vector<uint32_t> rawdata;

    const auto block = ev->GetBlock(i);
    constexpr auto sizeWord = sizeof(uint32_t);

    rawdata.resize(block.size() / sizeWord);
    memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

    std::string description;
    if (d1->GetTag("isPiggy") == "true") {
      description = "RD50_MPW3_piggy";
    } else {
      description = "RD50_MPW3_base";
    }
    /*
     * the description is needed in corryvreckan to identify the detector
     * In Corry-Geometry file: "type = RD50_MPW3_base"
     * In Corry-Config:
     * [EventLoaderEUDAQ2]
     * "name = RD50_MPW3_base_0" or whatever name you assigned to the detector
     * in the geometry "type = "RD50_MPW3_base" or piggy, if it's the piggy
     * detector
     */

    d2->SetDescription(description);
    eudaq::StandardPlane plane(0, "Frame", description);
    plane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);

    for (const auto &word : rawdata) {
      /*
       * Each word represents a preprocessed (by the Mpw3FileReader) hit.
       * It alrdy contains computed ToT values as well as the double column and
       * the pixel index of the current hit
       * 1 uint32_t word contains this data in the following order:
       * 1 byte dcol   1 byte pix    2 byte ToT
       */

      uint8_t dcol = (word & (0xFF << 24)) >> 24;
      uint8_t pix = (word & (0xFF << 16)) >> 16;
      uint16_t tot = word & 0xFFFF;

      auto hitPixel = DefsMpw3::dColIdx2Pix(dcol, pix);
      //      std::cout << "word = " << word << " dcol " << dcol << " pix " <<
      //      pix << " tot " << tot << "\n" << std::flush;

      plane.PushPixel(hitPixel.col, hitPixel.row,
                      tot); // store ToT as "raw pixel value"
    }
    d2->AddPlane(plane);
    planes++;
  }
  d2->SetTimeBegin(d1->GetTimestampBegin());
  d2->SetTimeEnd(d1->GetTimestampEnd());

  if (planes > 0) {
    return true;
  }

  return false;
}
