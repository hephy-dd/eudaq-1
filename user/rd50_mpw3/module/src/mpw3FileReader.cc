#include "mpw3FileReader.h"

#include "eudaq/Logger.hh"
#define DEBUG_OUTPUT

namespace {
auto dummy0 =
    eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader, std::string &>(
        eudaq::cstr2hash("mpw3raw"));
auto dummy1 =
    eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader, std::string &&>(
        eudaq::cstr2hash("mpw3raw"));
// has to be same name as mpw3FileWriter file extension to be recognized by
// euCliConverter

} // namespace

Mpw3FileReader::Mpw3FileReader(const std::string &filename)
    : mFilename(filename) {}

eudaq::EventSPC Mpw3FileReader::GetNextEvent() {
  eudaq::EventSP retval = nullptr;
  if (!mDes) {
    mDes.reset(new eudaq::FileDeserializer(mFilename));
    mStartTime = std::chrono::high_resolution_clock::now();
    if (!mDes)
      EUDAQ_THROW("unable to open file: " + mFilename);
  }

  if (!eventRdy(retval)) {
    // there are no timestamped events left, let's process the next frame
    eudaq::EventUP ev = nullptr;

    while (true) {

      if (mDes->HasData()) {
        uint32_t id;
        mDes->PreRead(id);
        ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(id,
                                                                         *mDes);
      } else {
        auto tConversion =
            std::chrono::high_resolution_clock::now() - mStartTime;
        std::cout << "\n finished " << mFrameCnt << " frames and " << mEventCnt
                  << " events in " << double(tConversion.count()) * 1e-9
                  << "[s]\n";
        std::cout << "This corresponds to a speed of "
                  << double(tConversion.count()) / double(mFrameCnt) * 1e-6
                  << "[ms] per frame and "
                  << double(tConversion.count()) / double(mEventCnt) * 1e-6
                  << "[ms] per event\n";
        return nullptr; // no more data left, we finished whole file
      }

      if (processFrame(ev)) {
        break; // processed frame successful, therefore we should continue
               // preparing and returning events before processing next one
      } else {
        EUDAQ_WARN("Error processing frame; skipping this one");
        continue;
      }
    }

    if (mHBBase.size() == 0 && mHBPiggy.size() == 0) {
      EUDAQ_ERROR("Buffer after frame processing empty, probably a bug");
      return nullptr; // we should never get here, just to be save
    }

    buildEvent(mHBBase, mEventBuffBase);
    buildEvent(mHBPiggy, mEventBuffPiggy);

    if (eventRdy(retval)) {
      return std::move(retval);
    }

  } else {
    return std::move(retval);
  }
  return nullptr;
}

bool Mpw3FileReader::processFrame(const eudaq::EventUP &frame) {

  static auto lastT = std::chrono::high_resolution_clock::now();
  std::cout << "\rprocessing Frame #" << mFrameCnt << " Event #" << mEventCnt
            << std::flush;

  if (frame == nullptr) {
    EUDAQ_DEBUG("frame to process == nullptr");
    return false;
  }

  if (frame->IsBORE() || frame->IsEORE() || frame->NumBlocks() == 0) {

    EUDAQ_DEBUG("frame to process is BORE / EORE / NumBlocks == 0");
    return false;
  }

  auto block = frame->GetBlock(0);

  if (block.size() < 2 * sizeof(DefsMpw3::word_t)) {

    // does not even contain head and tail, must be bullshit
    EUDAQ_DEBUG("frame to process: too small blocksize");
    return false;
  }

  std::vector<DefsMpw3::word_t> rawData;
  const int sizeWord = sizeof(DefsMpw3::word_t);

  rawData.resize(block.size() / sizeWord);
  memcpy(rawData.data(), block.data(), block.size());

  uint32_t ovFlwSOF, ovFlwEOF;

  if (DefsMpw3::isSOF(rawData.front()) && DefsMpw3::isEOF(rawData.back())) {
    // first word is SOF and last word is EOF, so far seems to be valid
    ovFlwSOF = DefsMpw3::extractOverFlowCnt(rawData.front());
    ovFlwEOF = DefsMpw3::extractOverFlowCnt(rawData.back());
    // the ovflwCnt from FPGA is only 8 bit long = > ovflw after ~3ms, not
    // sufficient! count ovflws of the ovflwCnt ;)
    if (ovFlwSOF < mOldOvflwCnt) {
      mOvflwCntOfOvflwCnt++;
    }
    mOldOvflwCnt = ovFlwSOF;

  } else {
    EUDAQ_WARN("invalid frame (not starting with SOF / not ending with EOF)");
    return false;
  }
  auto lastSOF =
      std::find_if(rawData.rbegin(), rawData.rend(),
                   &DefsMpw3::isSOF); // using reverse iterators as we have to
                                      // search from end to begin
  auto firstEOF =
      std::find_if(rawData.begin(), rawData.end(), &DefsMpw3::isEOF);

  // there might be more than 1 SOF / EOF word (base and piggy come with their
  // own) at this stage we only want to process the payload (hit) data in
  // between both
  for (auto i = lastSOF.base(); i < firstEOF; i++) {
    const auto word = *i;
    DefsMpw3::HitInfo hi(word);
    Hit hit;
    hit.dcol = hi.dcol;
    hit.pix = hi.pix;
    hit.tsLe = hi.tsLe;
    hit.tsTe = hi.tsTe;
    hit.isPiggy = hi.piggy;
    hit.ovflwSOF = ovFlwSOF;
    hit.ovflwEOF = ovFlwEOF;
    hit.avgFrameOvflw = double(ovFlwEOF - ovFlwSOF) / 2.0;
    if (hit.isPiggy) {
      mHBPiggy.push_back(hit);
    } else {
      mHBBase.push_back(hit);
    }
  }

  auto tmp = std::chrono::high_resolution_clock::now();
  mTimeForFrame = tmp - lastT;
  lastT = tmp;
  mFrameCnt++;
  return true;
}

void Mpw3FileReader::buildEvent(HitBuffer &in, EventBuffer &out) {

#ifdef DEBUG_OUTPUT
  static int nEvt = 0;
#endif
  if (in.size() == 0) {
    return;
  }

#ifdef DEBUG_OUTPUT
  if (nEvt < 5) {
    std::ofstream outUnsorted("unsorted.csv", std::ios_base::app);
    outUnsorted << Hit::dbgFileHeader();
    for (const auto &hit : in) {
      outUnsorted << hit.toStr();
    }
    outUnsorted.flush();
  }
#endif

  /*
   * Event-Building + Time-Bin-Matching upcoming..
   *
   * 1) We extract hits sorted in double columns from current buffer
   * 2) We check for overflows in the specific double columns.
   *    This has to be done as we cannot rely on the ovflwCnt from the
   *    SOF for the whole frame
   * 3) Calculate global timestamps by considering detected ovflwCnt,
   *    SOF ovflwCnt and TS_LE
   * 4) Sort buffer in terms of global timestamps
   * 5) Merge hits with similar timestamps to events
   */

  auto tStartEvtProcess = std::chrono::high_resolution_clock::now();

  std::vector<std::vector<Hit>::iterator>
      hitsSortedByDCol[DefsMpw3::dimSensorCol / 2];
  for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
    for (auto i = in.begin(); i < in.end(); ++i) {
      if (i->dcol == dcol) {
        hitsSortedByDCol[dcol].push_back(
            i); // we need to keep the hit order of the original frame
                // to detect overflows
      }
    }
  }
#ifdef DEBUG_OUTPUT
  if (nEvt < 5) {
    std::ofstream outSorted("sorted.csv", std::ios_base::app);
    outSorted << Hit::dbgFileHeader();
    for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
      const auto &currDCol = hitsSortedByDCol[dcol];
      DefsMpw3::ts_t ovflwCnt = 0;
      bool first = true;
      for (auto i = currDCol.begin(); i < currDCol.end();
           i++) // i is an iterator pointing to an iterator, quite some
      // dereferencing to do...
      {
        outSorted << (*i)->toStr();
        outSorted.flush();
      }
    }
  }
#endif

  for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
    const auto &currDCol = hitsSortedByDCol[dcol];
    DefsMpw3::ts_t ovflwCnt = 0;
    bool first = true;

    for (auto i = currDCol.begin(); i < currDCol.end();
         i++) // i is an iterator pointing to an iterator, quite some
    // dereferencing to do...
    {

      if (!first &&
          (std::abs(int((*i)->tsLe) - int((*(i - 1))->tsLe))) >
              DefsMpw3::lsbToleranceOvflwDcol) { // we may not check for
                                                 // overflow in the 1st
        // element, as there will be no *(i - 1) !
        ovflwCnt++; // we detected a higher TS_LE followed by a lower
                    // one
                    // => overflow pretty likely
      }
      first = false;

      if (!(*i)->tsGenerated) {
        // this is the ominous time-bin-matching, the whole reason for
        // this class btw...
        (*i)->globalTs = (ovflwCnt + (*i)->ovflwSOF) * DefsMpw3::dTPerOvflw +
                         (*i)->tsLe * DefsMpw3::dTPerTsLsb +
                         mOvflwCntOfOvflwCnt * DefsMpw3::dtPerOvflwOfOvwfl;
        (*i)->tsGenerated = true;
        /* we only want to generate the TS once. We have to check this as the
         * current hit, if it is close to the end of the frame, might be
         * merged with hits from the next frame into 1 event. The TS should be
         * generated within the frame this hit originally belongs to, so SOF
         * and EOF ovflwCnt are correct.
         */
      }

      auto pix = DefsMpw3::dColIdx2Pix((*i)->dcol, (*i)->pix);

      //      std::cout << "timebin matching for " << pix.row << ":" << pix.col
      //                << " sof = " << (*i)->ovflwSOF << " ovflwCnt = " <<
      //                ovflwCnt
      //                << " ovflOvflw = " << mOvflwCntOfOvflwCnt
      //                << " TS_LE = " << int((*i)->tsLe)
      //                << " => global = " << (*i)->globalTs << "\n"
      //                << std::flush;

      // FIXME: if a whole timebin is without any hits in a certain dcol
      // and TS-C's seem to be growing continuously we'll get it wrong..
      // Don't know if it is even possible to detect this :/ ..
    }
  }
#ifdef DEBUG_OUTPUT
  if (nEvt < 5) {
    std::ofstream outTs("sorted+ts.csv", std::ios_base::app);
    outTs << Hit::dbgFileHeader();

    for (int i = 0; i < DefsMpw3::dimSensorCol / 2; i++) {
      for (const auto &h : hitsSortedByDCol[i]) {
        outTs << (*h).toStr();
      }
      outTs.flush();
    }
  }
#endif

  // Now that we assigned the global timestamps we can sort them in
  // ascending order
  std::sort(in.begin(), in.end(), [](const Hit &h1, const Hit &h2) {
    return h1.globalTs < h2.globalTs;
  });

#ifdef DEBUG_OUTPUT
  int evtNmb = 0;
  if (nEvt < 5) {
    if (evtNmb == 0) {
      std::ofstream outTsOrdered("timestamp_ordererd.csv", std::ios_base::app);
      outTsOrdered << Hit::dbgFileHeader();
      for (const auto &hit : in) {
        outTsOrdered << hit.toStr();
      }
      outTsOrdered.flush();
    }
  }
#endif

  auto endTimewindow = in.begin();
  int initBuffSize = in.size();
  while (in.size() > 0) { // event building based on global TS
    HitBuffer prefab;
#ifdef DEBUG_OUTPUT
    evtNmb++;
#endif
    //    std::cout << "operating on buffer size = " << in.size() << "\n";
    for (auto i = in.begin(); i < in.end(); i++) {

      if (i->globalTs - in.begin()->globalTs <=
          DefsMpw3::dTSameEvt) { // do 2 hits belong to the same event?
        prefab.push_back(*i);
        endTimewindow = i;
      } else {
        break; // we found all hits belonging to the current event
      }
    }
    //    std ::cout << "hits for event #" << evtNmb << ":\n";
    //    for (auto i = prefab.begin(); i < prefab.end(); i++) {
    //      std::cout << i->toStr() << "\n";
    //    }

    in.erase(
        in.begin(),
        endTimewindow +
            1); // processed, delete from buffer; +1 needed as we also want
                // to remove the last element, begin is inclusive, end is not

    finalizePrefab(prefab, out);
#ifdef DEBUG_OUTPUT
    if (nEvt < 5) {
      std::ofstream outPrefab("prefab.csv",
                              std::fstream::out | std::fstream::app);
      if (evtNmb == 1) {
        outPrefab << Hit::dbgFileHeader();
      }
      outPrefab << "\n Event #" << evtNmb << "\n";
      for (const auto &hit : prefab) {
        outPrefab << hit.toStr();
      }
    }
#endif

    if (in.size() < 1.0 / 4.0 * double(initBuffSize) && mDes->HasData()) {
      break; // stop processing buffer at 1/4 initial length, upcoming hits
             // may be merged with hits from next frame to an event
      // 1/4 is a pretty random choice
      // TODO: do better, think more
    }
  }
#ifdef DEBUG_OUTPUT
  nEvt++;
#endif
}

void Mpw3FileReader::finalizePrefab(const HitBuffer &prefab, EventBuffer &out) {

  static auto lastT = std::chrono::high_resolution_clock::now();

  if (prefab.size() == 0) {
    return;
  }

  auto euEvent = eudaq::Event::MakeShared("MPW3PreprocessedEvent");

  auto minTs = std::min_element(prefab.begin(), prefab.end(),
                                [](const Hit &h1, const Hit &h2) {
                                  return h1.globalTs < h2.globalTs;
                                })
                   ->globalTs;
  auto maxTs = std::max_element(prefab.begin(), prefab.end(),
                                [](const Hit &h1, const Hit &h2) {
                                  return h1.globalTs < h2.globalTs;
                                })
                   ->globalTs;

  euEvent->SetTimestamp(minTs, maxTs);
  std::vector<uint32_t> data;
  for (auto i = prefab.begin(); i < prefab.end(); ++i) {
    auto tot = int(i->tsTe) - int(i->tsLe); // Time Over Threshold

    if (tot < 0) {
      tot += 256; // compensate overflow
    }

    uint32_t tmp =
        (uint32_t(i->dcol) << 24) + (uint32_t(i->pix) << 16) +
        uint32_t(tot); // store whole pixel-hit information in 32 bit word

    data.push_back(tmp);
  }
  euEvent->AddBlock(0, data);
  auto piggyTag = prefab[0].isPiggy
                      ? "true"
                      : "false"; // they all originate either from piggy or
                                 // base, looking at 1 is sufficient
  euEvent->SetTag("isPiggy", piggyTag);
  out.push_back(euEvent);
  /* this preprocessed event now contains information of
   * pixel identifiers and ToT of all hit pixel in the processed timewindow,
   * as well as the global timestamp of this event (begin + end)
   *
   * this afterwards needs to be converted by a Raw->StdEventConverter for
   * actual analysis
   */

  mEventCnt++;
  auto tmp = std::chrono::high_resolution_clock::now();
  mTimeForEvent = tmp - lastT;
  lastT = tmp;
}

bool Mpw3FileReader::eventRdy(eudaq::EventSP &event) {
  std::vector<EventBuffer *> buffs = {&mEventBuffBase, &mEventBuffPiggy};

  for (int i = 0; i < 2; i++) {
    if (buffs[i]->size() > 0) {
      event = buffs[i]->front();
      buffs[i]->erase(buffs[i]->begin());
      return true;
    }
  }
  return false;
}
