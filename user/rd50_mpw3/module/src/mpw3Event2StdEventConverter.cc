#include "defs.h"
#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

class Mpw3Raw2StdEventConverter : public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
                  eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("RD50_MPWxEvent");
  static bool foundT0Base, foundT0Piggy, parsedCalibration;
  static double calibMap[DefsMpw3::dimSensorCol * DefsMpw3::dimSensorRow * 2];

private:
  int inline indexCalibMap(int row, int col) const;
};

bool Mpw3Raw2StdEventConverter::foundT0Base = false;
bool Mpw3Raw2StdEventConverter::foundT0Piggy = false;
bool Mpw3Raw2StdEventConverter::parsedCalibration = false;
double Mpw3Raw2StdEventConverter::calibMap[DefsMpw3::dimSensorCol *
                                           DefsMpw3::dimSensorRow * 2] = {0.0};

namespace {
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::Register<
      Mpw3Raw2StdEventConverter>(Mpw3Raw2StdEventConverter::m_id_factory);
}

bool Mpw3Raw2StdEventConverter::Converting(eudaq::EventSPC d1,
                                           eudaq::StdEventSP d2,
                                           eudaq::ConfigSPC conf) const {

  enum class TimestampMode { TLU, Ovflw };

  TimestampMode tsMode = TimestampMode::Ovflw;

  double t0 = -1.0;
  bool filterZeroWords = true;
  bool weArePiggy = false;
  int tShift = 0;
  uint32_t lsbTime = 50000;

  /*
   * We might be supposed to convert our charge, read out as ToT units,
   * to an actual charge in e-
   * For this we must have a calibration at hand
   * Such a calibration can be recorded via injections and the
   * "recordTotCalibration" function of our Peary
   * The output of this function must be processed by the python script
   * "create-tot-calib.py" The output file of this script can than be provided
   * to Corryvreckan via the "calibration_file" parameter in the geometry file
   * Corry provides this file as a config parameter to this very EUDAQ converter
   * The file consists of several lines, each line corresponds to the linear fit
   * parameters of one pixel We convert ToT to charge Q with a linear relation
   * according to ToT = Q * k + d k ... slope, d ... offset One line looks like
   * <row> <col> <k> <d>
   * Eg a calib entry for Pixel 0:13 could look like:
   * 0 13 0.0005488587662337663 1.0923863636363622
   * This function parses this file and puts it into a global array which is
   * later used during the conversion from ToT -> Q
   *
   */
  auto parseCalibration = [&](const std::string calibFile) {
    std::string line;
    std::ifstream in(calibFile);
    if (!in.is_open()) {
      return false;
    }
    while (std::getline(in, line)) {
      if (line.front() == '#') {
        continue;
      }
      std::istringstream iss(line);
      int row, col;
      double k, d;
      iss >> row >> col >> k >> d;
      if (row > DefsMpw3::dimSensorRow || col > DefsMpw3::dimSensorCol) {
        EUDAQ_ERROR("invalid pixel specifier in calib file");
        return false;
      }
      int index = indexCalibMap(row, col);
      calibMap[index] = k;
      calibMap[index + 1] = d;
    }
    return true;
  };

  if (conf != nullptr) {
    t0 = conf->Get("t0_skip_time", -1.0) * 1e6;
    filterZeroWords = conf->Get("filter_zeros", true);
    weArePiggy = conf->Get("is_piggy", false);
    tsMode = conf->Get("ts_mode", "Ofvlw") == "TLU" ? TimestampMode::TLU
                                                    : TimestampMode::Ovflw;
    lsbTime = conf->Get("lsb_time", 50000);

    if (!parsedCalibration) {

      if (parseCalibration(conf->Get("calibration_file", ""))) {
        parsedCalibration = true;
      }
    }
  }

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  uint64_t timeBegin, timeEnd;

  auto type = ev->GetTag("Type", "Base");
  auto blockIdx = type == "Base" ? 0 : 1;
  // block[0] contains base data
  // block[1] piggy data
  std::vector<uint32_t> rawdata;

  auto block = ev->GetBlock(blockIdx);
  constexpr auto sizeWord = sizeof(uint32_t);
  //    if (block.size() <= 4 * sizeWord) {
  //        //does not contain hit word, just timestamp words
  //      EUDAQ_WARN("empty event");
  //      return false;
  //    }

  rawdata.resize(block.size() / sizeWord);
  memcpy(rawdata.data(), block.data(), rawdata.size() * sizeWord);

  eudaq::StandardPlane basePlane(0, "Base", "RD50_MPWx");
  eudaq::StandardPlane piggyPlane(1, "Piggy", "RD50_MPWx");
  basePlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
  piggyPlane.SetSizeZS(DefsMpw3::dimSensorCol, DefsMpw3::dimSensorRow, 0);
  int64_t tluTsLsb = -1, tluTsMsb = -1, ovflwCntLsb = -1, ovflwCntMsb = -1;
  int sofCnt = 0, eofCnt = 0, hitCnt = 0;
  std::vector<uint32_t> tsLe;
  double avgTsLe = 0.0, avgTsTe = 0.0;
  int errorCnt = 0;

  bool insideFrame = false;
  DefsMpw3::ts_t frameTs;

  for (const auto &word : rawdata) {
    /*
     * each word represents a particle detection in the specified pixel for a
     * certain ToT.
     */

    if (filterZeroWords && (word == 0 || word == (1 << 23))) {
      /* during UDP pack construction in FW the FIFOs for piggy and base get
       * pulled alternatingly if there is only data left in one of the two,
       * the other one still gets pulled (which results in 0x0 or 0x800000 (
       * bit 23 set to indicate piggy))
       * we might want to skip these words as they would be interpreted as
       * pixel 00:00 with TS-LE = TS-TE = 0
       */
      continue;
    }

    DefsMpw3::HitInfo hi(word);

    // std::cout << std::hex << hi.initialWord << "\n";

    if (hi.sof) {
      ovflwCntLsb = hi.timestamp;
      continue;
    }
    if (hi.eof) {
      ovflwCntMsb = hi.timestamp;
      continue;
    }
    if (hi.isTluTsLsb) {
      tluTsLsb = hi.timestamp;
      continue;
    }
    if (hi.isTluTsMsb) {
      tluTsMsb = hi.timestamp;
      continue;
    }
    avgTsLe += hi.tsLe;
    tsLe.push_back(hi.tsLe);
    avgTsTe += hi.tsTe;
    hitCnt++;
    if (!hi.hitWord) {
      EUDAQ_DEBUG("weird word, should not occur, check code");
      std::cout << std::hex << hi.initialWord << " " << hi.isTluTsLsb << " "
                << hi.isTluTsMsb << " " << hi.isTimestamp << "\n";
      continue;
    }

    uint32_t charge = hi.tot;
    if (parsedCalibration) {
      // if we got a calibration and parsed it successfully convert ToT to
      // charge
      auto i = indexCalibMap(hi.pixIdx.row, hi.pixIdx.col);
      auto k = calibMap[i];
      auto d = calibMap[i + 1];
      // std::cout << "converting " << hi.tot << " to " << (double(hi.tot) - d)
      // / k << "\n";
      charge = (double(hi.tot) - d) / k;
    }

    if (hi.piggy) {
      piggyPlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, charge);
    } else {
      basePlane.PushPixel(hi.pixIdx.col, hi.pixIdx.row, charge);
    }
  }
  /*
   * timestamp for begin / end is being calculated by
   * begin: combined ovflw Cnt from SOF and EOF and minimum TS-LE from all
   * hits in the current frame end: combined ovflw Cnt from  and EOF and
   * maximum TS-TE from all hits in the current frame
   */

  if (tsMode == TimestampMode::Ovflw && ovflwCntLsb >= 0 && ovflwCntMsb >= 0) {
    // std::cout << "sof =  " << sofWord << " eof = " << eofWord << "\n";

    uint32_t minTsLe = 0, maxTsLe = 0;
    //    tsLe.clear();
    if (tsLe.size() > 0) {
      minTsLe = *std::min_element(tsLe.begin(), tsLe.end());
      maxTsLe = *std::max_element(tsLe.begin(), tsLe.end());
    } else {
    }

    timeBegin =
        ((ovflwCntLsb + (ovflwCntMsb << 23)) * DefsMpw3::dTPerOvflw + minTsLe) *
        lsbTime;
    timeEnd =
        ((ovflwCntLsb + (ovflwCntMsb << 23)) * DefsMpw3::dTPerOvflw + maxTsLe) *
        lsbTime;

  } else if (tsMode == TimestampMode::TLU && tluTsLsb >= 0 && tluTsMsb >= 0) {
    timeEnd = timeBegin = (tluTsLsb + (tluTsMsb << 23)) * lsbTime;

  } else {
    EUDAQ_WARN("Not possible to generate timestamp");
    // std::cout << "sofs " << sofCnt << " eofs " << eofCnt << "\n";
    //    std::cout << "failed\n";
    return false;
  }

  if (t0 < 0.0) {
    // don't skip any time, so just assume we "found" T0 for both
    foundT0Base = foundT0Piggy = true;
  } else if (timeBegin < uint64_t(t0)) {
    if (!weArePiggy && type == "Base") {
      foundT0Base = true;
    } else if (type == "Piggy") {
      foundT0Piggy = true;
    }
  }

  if (!foundT0Base && !weArePiggy) {
    return false;
  }
  if (!foundT0Piggy && weArePiggy) {
    return false;
  }

  d2->SetDescription("RD50_MPWx");
  if (basePlane.HitPixels(0) > 0) {
    d2->AddPlane(basePlane);
  }
  if (piggyPlane.HitPixels(0) > 0) {
    d2->AddPlane(piggyPlane);
  }

  d2->SetTimeBegin(timeBegin);
  d2->SetTimeEnd(timeEnd);
  return true;
}

/**
 * @brief map the pixel address to an array index of the calibration map
 * @param row pixel row
 * @param col pixel column
 * @return index in calibMap to use for accessing the calibration parameters of
 * the specified pixel. The 'k' parameter is found at the returned index, whilst
 * 'd' is located at the returned index + 1
 */
int Mpw3Raw2StdEventConverter::indexCalibMap(int row, int col) const {
  return (row * DefsMpw3::dimSensorCol + col) * 2;
}
