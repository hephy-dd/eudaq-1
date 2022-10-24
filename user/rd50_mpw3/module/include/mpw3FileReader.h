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
    DefsMpw3::ts_t globalTs = 0;
    bool isPiggy;
    bool tsGenerated = false;
    int originFrame;
    int64_t udpTs;
    DefsMpw3::PixelIndex pixIdx;

    std::string inline toStr() const {
      std::stringstream ss;
      auto idx = DefsMpw3::dColIdx2Pix(dcol, pix);
      ss << std::setfill('0') << " " << std::setw(2) << int(dcol) << ";  "
         << std::setw(3) << int(pix) << ";  " << std::setw(2) << idx.row << ":"
         << std::setw(2) << idx.col << ";   " << std::setw(3) << int(tsLe)
         << ";   " << std::setw(3) << int(tsTe) << ";   " << std::setw(6)
         << globalTs << ";   " << std::setw(5) << ovflwSOF << "; " << udpTs
         << "\n";
      return std::move(ss.str());
    }
    const static std::string inline dbgFileHeader() {
      return "#DCol;Pix;ROW:COL;TS_LE; TS_TE; TS-glob; OvflwSOF; FW TS\n";
    }
  };
  using HitBuffer = std::vector<Hit>;
  using EventBuffer = std::vector<eudaq::EventSP>;

  static constexpr int sizeEvtQueue = 3;

  bool processFrame(const eudaq::EventUP &frame);
  void buildEvent(HitBuffer &in, EventBuffer &out);
  void finalizePrefab(const HitBuffer &prefab, EventBuffer &out);
  bool eventRdy(eudaq::EventSP &event);

  std::unique_ptr<eudaq::FileDeserializer> mDes;
  std::string mFilename;

  EventBuffer mEventBuffBase, mEventBuffPiggy;
  std::vector<Hit> mHBBase, mHBPiggy;

  uint64_t mEventCnt = 0, mFrameCnt = 0;
  std::chrono::high_resolution_clock::duration mTimeForEvent, mTimeForFrame;
  std::chrono::high_resolution_clock::time_point mStartTime;
  uint32_t mOldOvflwCnt = 0, mOvflwCntOfOvflwCnt = 0;
};
#define MPW3FILEREADER_H

#endif // MPW3FILEREADER_H
