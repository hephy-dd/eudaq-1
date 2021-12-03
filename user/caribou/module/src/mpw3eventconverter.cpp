#include "CaribouEvent2StdEventConverter.hh"

using namespace eudaq;

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Mpw3RawEvent2StdEventConverter>(
      Mpw3RawEvent2StdEventConverter::m_id_factory);
}

bool Mpw3RawEvent2StdEventConverter::Converting(eudaq::EventSPC d1,
                                                eudaq::StdEventSP d2,
                                                eudaq::ConfigSPC conf) const {

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  const int dimSensor = 8; // dimension of sensor array (assumed quadratic form)
  // Data containers:
  std::vector<uint64_t> timestamps;
  std::vector<uint32_t> rawdata;

  // Retrieve data from event

  if (ev->NumBlocks() == 1) {
    auto datablock = ev->GetBlock(0);

    uint32_t nData, triggerN;
    auto sizeWord = sizeof(uint32_t);

    /*
     * data is stored in raw bytes, but we inserted (in getRawData @ caribou
     * device) uint32_t values => we have to restore whole words of data
     */

    const int headerLength = 2 * sizeWord;

    /*
     * the protocol specifies a header
     * 1st word: n word of data in block
     * 2nd word: dimension of the sensor
     */

    memcpy(&nData, &datablock[0], sizeWord);
    memcpy(&triggerN, &datablock[0] + sizeWord, sizeWord);
    //    memcpy(&dimSensor, &datablock[0] + sizeWord, sizeWord);

    //    std::cout << "          nData = " << nData;
    rawdata.resize(datablock.size() /
                   sizeWord); // resize to be able to contain the proper number
                              // of sent data
    //    std::cout << "rawData size = " << rawdata.size() << "\n";
    memcpy(&rawdata[0], &datablock[0] + headerLength,
           nData * sizeWord); // extracted reasonable data

    eudaq::StandardPlane plane(0, "Caribou", "RD50_MPW3");
    plane.SetSizeZS(dimSensor, dimSensor, 0);
    // we have a x * y grid, where each grid-point has 1  pixel (this is the 0)

    for (int i = 0; i < nData; i++) {
      int x = i / dimSensor;
      int y = i % dimSensor;
      /* this is the part of the protocol in which the hits per pixel
       * are stored. We store a whole row (x) -> next col index -> next full row
       *                 |00|01|02|
       * Eg sensor like  |10|11|12|  is stored as |00|01|02|10|11|12|20|21|22|
       *                 |20|21|22|
       */

      if (x <= dimSensor && y <= dimSensor) {
        plane.PushPixel(x, y, rawdata[i]);
      }
    }

    d2->AddPlane(plane);
    d2->SetTriggerN(triggerN);
    uint64_t timebegin = triggerN * 2 * 1e12;
    d2->SetTimeBegin(timebegin);
    d2->SetTimeEnd(timebegin);
    return true;
  }
  return false;
}
