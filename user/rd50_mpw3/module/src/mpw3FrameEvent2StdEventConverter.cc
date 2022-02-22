#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3FrameEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Mpw3FrameEvent");

private:
  struct PixelIndex {
    int col;
    int row;
  };

  static PixelIndex dColIdx2Pix(int dcol, int pix);
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
    rawdata.resize(block.size() / sizeWord - 2); // without head and tail
    memcpy(rawdata.data(), block.data() + sizeWord,
           rawdata.size() * sizeWord); // starting 1 word after head

    eudaq::StandardPlane plane(0, "Frame", "RD50_MPW3");
    plane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);

    for (const auto &word : rawdata) {
      /*
       * each word represents a particle detection in the specified pixel for a
       * certain ToT. This converter does not aim to generate a proper global
       * Timestamp. We simply store the hit information of each word in a plane.
       * Therefore meant only for the monitor not for real analysis!!!
       */
      auto dcol = DefsMpw3::extractDcol(word);
      auto pix = DefsMpw3::extractPix(word);
      int tsTe = DefsMpw3::extractTsTe(word);
      int tsLe = DefsMpw3::extractTsLe(word);
      int tot = tsTe - tsLe;

      if (tot < 0) {
        tot += 256;
      }
      auto hitPixel = dColIdx2Pix(dcol, pix);
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

Mpw3FrameEvent2StdEventConverter::PixelIndex
Mpw3FrameEvent2StdEventConverter::dColIdx2Pix(int dcol, int pix) {
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
   * 0/0 therefore is pixel 0 in dcol 0 and 2/0 is pixel 5 in dcol 0
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

  return retval;
}
