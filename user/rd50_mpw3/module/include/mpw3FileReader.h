#ifndef MPW3FILEREADER_H

#include <chrono>

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

    std::string inline toStr() const {
      std::stringstream ss;
      auto idx = DefsMpw3::dColIdx2Pix(dcol, pix);
      ss << std::setfill('0') << " " << std::setw(2) << idx.row << ":"
         << std::setw(2) << idx.col << ";   " << std::setw(3) << int(tsLe)
         << ";   " << std::setw(3) << int(tsTe) << ";   " << std::setw(6)
         << globalTs << ";   " << std::setw(5) << ovflwSOF << "\n";
      return std::move(ss.str());
    }
    static std::string inline strHeader() {
      return "#ROW:COL;TS_LE; TS_TE; TS-glob; OvflwSOF \n";
    }
  };
  using PrefabEvt = std::vector<Hit>;

  static constexpr int sizeEvtQueue = 3;

  bool processFrame(const eudaq::EventUP &frame);
  void finalizePrefab(const PrefabEvt &prefab);

  std::unique_ptr<eudaq::FileDeserializer> mDes;
  std::string mFilename;

  std::vector<eudaq::EventSP> mTimeStampedEvents;
  std::vector<Hit> mHitBuffer;

  uint64_t mEventCnt = 0, mFrameCnt = 0;
  std::chrono::high_resolution_clock::duration mTimeForEvent, mTimeForFrame;
  std::chrono::high_resolution_clock::time_point mStartTime;
};
#define MPW3FILEREADER_H

#endif // MPW3FILEREADER_H
