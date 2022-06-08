#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Monopix2RawEvent2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Monopix2RawEvent");

private:
  const int dimSensorCol = 512, dimSensorRow = 512;

  static uint32_t grayDecode(uint32_t gray);
  static inline auto isTjMonoTS(const uint32_t word) {
    return (word & 0xF8000000) == 0x48000000;
  }
  static inline auto isTjMonoData(const uint32_t word) {
    return (word & 0xF8000000) == 0x40000000;
  }
};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Monopix2RawEvent2StdEventConverter>(
      Monopix2RawEvent2StdEventConverter::m_id_factory);
}

bool Monopix2RawEvent2StdEventConverter::Converting(
    eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  std::vector<uint32_t> rawdata;

  const auto block = ev->GetBlock(0);
  constexpr auto sizeWord = sizeof(uint32_t);

  rawdata.resize(block.size() / sizeWord);
  memcpy(rawdata.data(), block.data(),
         rawdata.size() * sizeWord); // convert bytes to words (uint32's)

  eudaq::StandardPlane plane(0, "Event", "Monopix2");
  plane.SetSizeZS(dimSensorCol, dimSensorRow, 0);

  int le = -1, te = -1, row = -1, col = -1, tot, tjTS = -1, processingStep = 0,
      errorCnt = 0, actualHits = 0;
  bool insideFrame = false;

  for (const auto &word : rawdata) {
    if (isTjMonoTS(word)) { // we handle a timestamp word
      tjTS = word & 0x7FFFFFF;
    } else if (isTjMonoData(word)) { // this is a data word
      std::vector<uint32_t> bytes = {
          (word & 0x7FC0000) >> 18, (word & 0x003FE00) >> 9,
          word & 0x00001FF}; // each raw data word can be split in 3 distinct
                             // bytes which all carry different information
                             // about the hit

      //      std::cout << "data = 0x" << std::hex << bytes[0] << " 0x" <<
      //      bytes[1]
      //                << " 0x" << bytes[2] << " 0x" << bytes[3] << "\n";
      for (auto d : bytes) {
        if (d == 0x1bc) { // SOF
          if (insideFrame) {
            errorCnt++;
            //            EUDAQ_WARN("SOF before EOF");
          }
          insideFrame = true;
          processingStep = 0;

        } else if (d == 0x17c) { // EOF
          if (!insideFrame) {
            errorCnt++;
            //            EUDAQ_WARN("EOF before SOF");
          }
          insideFrame = false;

        } else if (d == 0x13c) { // Idle
          continue;
        } else {
          if (!insideFrame) {
            errorCnt++;
            //            EUDAQ_WARN("not in frame but got data words");
          }

          switch (processingStep) {
          case 0:
            // column data
            col = (d & 0xff) << 1;
            processingStep++;
            break;
          case 1:
            le = grayDecode((d & 0xfe) >> 1);
            te = (d & 0x01) << 6;
            processingStep++;
            break;
          case 2:
            // TS tailing edge split over 2 data words
            te = grayDecode(te | ((d & 0xfc) >> 2));
            row = (d & 0x01) << 8;
            col = col + ((d & 0x02) >> 1); // column split in 2 bytes
            processingStep++;
            break;
          case 3:
            row = row | (d & 0xff); // row split in 2 bytes
            processingStep = 0;
            // we should have finished, store hit pixel
            if (le < 0 && te < 0) {
              errorCnt++;
              EUDAQ_WARN("invalid TS: le = " + std::to_string(le) +
                         " te = " + std::to_string(te));
            }
            tot =
                te -
                le; // time over threshold is TS leading edge - TS tailing edge
            tot = tot < 0 ? tot + 128 : tot; // looks like an overflow, add 2^7
            if (errorCnt > 0) {
              plane.PushPixel(col, row,
                              tot); // store ToT as "raw pixel value"
            } else {
              EUDAQ_WARN("discarding frame as error occured");
              errorCnt = 0;
            }

            actualHits++;
            break;
          }
        }
      }
    } else if ((word & 0x80000000) > 0) {
      // this is a tlu trigger word, we do not care about them,
      // got alrdy processed
    } else {
      EUDAQ_WARN("weird data word = " + std::to_string(word));
    }
  }

  d2->AddPlane(plane);
  //  d2->SetTimestamp(tjTS, tjTS);
  d2->SetTimeBegin(tjTS);
  d2->SetTimeEnd(tjTS);
  d2->SetTriggerN(d1->GetTriggerN());

  if (errorCnt > 0) {
    EUDAQ_WARN(std::to_string(errorCnt) + " errors occured during conversion");
  }
  if (insideFrame) {
    EUDAQ_WARN("no data left but still inside a frame");
  }
  return actualHits > 0; // there could be events with just a trigger word
  // does not make sense to process them
}

uint32_t Monopix2RawEvent2StdEventConverter::grayDecode(uint32_t gray) {
  uint32_t bin;
  bin = gray;
  while (gray >>= 1) {
    bin ^= gray;
  }
  return bin;
}
