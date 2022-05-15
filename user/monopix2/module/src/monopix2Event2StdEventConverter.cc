#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Monopix2RawEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Monopix2Raw");
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Monopix2RawEvent2StdEventConverter>(
      Monopix2RawEvent2StdEventConverter::m_id_factory);
}

bool Monopix2RawEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  //  std::cout << "converting MPW3FrameEvt\n" << std::flush;

  for (int i = 0; i < ev->NumBlocks(); i++) {
    std::vector<uint32_t> rawdata;

    const auto block = ev->GetBlock(i);
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
    rawdata.resize(block.size() / sizeWord - 4); // without head and tail
    memcpy(rawdata.data(),
           block.data() +
               sizeWord * 2, // TODO: *2 is double SOF (normal + piggy), find
                             // out how to handle it properly
           rawdata.size() * sizeWord); // starting 1 word after head

    eudaq::StandardPlane plane(0, "Event", "Monopix2");
    plane.SetSizeZS(DefsMonopix2::dimSensorCol, DefsMonopix2::dimSensorRow, 0);

    for (const auto &word : rawdata) {
      /*
       * each word represents a particle detection in the specified pixel for a
       * certain ToT. This converter does not aim to generate a proper global
       * Timestamp. We simply store the hit information of each word in a plane.
       * Therefore meant only for the monitor not for real analysis!!!
       */
      auto dcol = DefsMonopix2::extractDcol(word);
      auto pix = DefsMonopix2::extractPix(word);
      int tsTe = DefsMonopix2::extractTsTe(word);
      int tsLe = DefsMonopix2::extractTsLe(word);
      int tot = tsTe - tsLe;

      if (tot < 0) {
        tot += 256;
      }
      auto hitPixel = DefsMonopix2::dColIdx2Pix(dcol, pix);
      //      std::cout << "word = " << word << " dcol " << dcol << " pix " <<
      //      pix
      //                << " Te " << tsTe << " Le " << tsLe << " hitPixRow "
      //                << hitPixel.row << " col " << hitPixel.col << " tot " <<
      //                tot
      //                << "\n"
      //                << std::flush;

      plane.PushPixel(hitPixel.col, hitPixel.row,
                      tot); // store ToT as "raw pixel value"
    }
    d2->AddPlane(plane);
  }
  return true;
}
