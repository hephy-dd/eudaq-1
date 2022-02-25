#ifndef MPW3FILEREADER_H

#include "defs.h"
#include "eudaq/FileDeserializer.hh"
#include "eudaq/FileReader.hh"

class Mpw3FileReader : public eudaq::FileReader {
public:
  Mpw3FileReader(const std::string &filename);
  eudaq::EventSPC GetNextEvent() override;

private:
  struct Hit {
    DefsMpw3::ts_t ovflwSOF, ovflwEOF;
    double avgFrameOvflw;
    uint8_t dcol;
    uint8_t pix;
    uint8_t tsLe;
    uint8_t tsTe;
    DefsMpw3::ts_t globalTs;
  };
  using PrefabEvt = std::vector<Hit>;

  static constexpr int sizeEvtQueue = 3;

  bool processFrame(const eudaq::EventUP &frame);
  void finalizePrefab(const PrefabEvt &prefab);

  std::unique_ptr<eudaq::FileDeserializer> mDes;
  std::string mFilename;

  std::vector<eudaq::EventSP> mTimeStampedEvents;
  std::vector<Hit> mHitBuffer;
};
#define MPW3FILEREADER_H

#endif // MPW3FILEREADER_H
