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
  static bool first = true;
  eudaq::EventSP retval = nullptr;
  if (!mDes) {
    mDes.reset(new eudaq::FileDeserializer(mFilename));
    mStartTime = std::chrono::high_resolution_clock::now();
    if (!mDes)
      EUDAQ_THROW("unable to open file: " + mFilename);
  }

  if (first) {
    first = false;
    long long prevTs = 0;
    int skipCnt = 0;
    while (true) {
      /* Our datacollector is starting immediately when "Start" is pressed
      in
       * the run-control The AIDA-TLU needs some time. At start a reset TS
       is
       * issued by the TLU and processed by the FW (referred to as T0).
       * This moment accounts for the actual start of the run, from which on
       * synchronization with telescope data should be possible.
       * The data before T0 can be discarded is there is no data from
       telescope
       * anyways and is just annoying.
       */
      if (mDes->HasData()) {
        uint32_t id;
        mDes->PreRead(id);
        eudaq::EventUP ev = nullptr;
        ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(id,
                                                                         *mDes);
        auto currUdpTs = std::stoll(ev->GetTag("recvTS_FW", "-1"));
        if (currUdpTs < double(30) / 50e-9) {
          // current TS must be beneath 30s and smaller than the TS of the UDP
          // pack before to qualify for T0. 30s is a bit of a random choice, but
          // should be sufficient to find T0 within CERN SPS spill-structure
          std::cout << "T0 = " << currUdpTs << " = " << currUdpTs * 50e-9
                    << "s; skipped " << skipCnt << " frames before\n";
          break;
        } else {
          //              std::cout << "skipping " << currUdpTs << "\n";
          skipCnt++;
        }
        prevTs = currUdpTs;
      }
    }
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
        auto currUdpTs = std::stoll(ev->GetTag("recvTS_FW", "-1"));

        while (true) {
          if (mDes->HasData()) {
            uint32_t id;
            mDes->PreRead(id);
            ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(
                id, *mDes);
            auto udpTs = std::stoll(ev->GetTag("recvTS_FW", "-1"));
            // FIXME first event of new UDP pack get's thrown away
            // We aim to parse all frames within an UDP pack at first
            if (!processFrame(ev)) {
              EUDAQ_WARN("Error processing frame; skipping this one");
              continue;
            }
            if (udpTs != currUdpTs) {
              // current UDP pack apperently is finished
              break;
            }
          } else {
            break;
          }
        }
      } else {
        return nullptr;
        // no events and no data left, we have finished
      }

      if (mHBBase.size() == 0 && mHBPiggy.size() == 0 && mDes->HasData()) {

        EUDAQ_ERROR("Buffer after frame processing empty, probably a bug");

        return nullptr; // we should never get here, just to be save
      }

      buildEvent(mHBBase, mEventBuffBase);
      //      buildEvent(mHBPiggy, mEventBuffPiggy);

      if (eventRdy(retval)) {
        return std::move(retval);
      }
    }

  } else {
    return std::move(retval);
  }
  return nullptr;
}

bool Mpw3FileReader::processFrame(const eudaq::EventUP &frame) {

  static auto lastT = std::chrono::high_resolution_clock::now();
  //  std::cout << "\rprocessing Frame #" << mFrameCnt << " Event #" <<
  //  mEventCnt
  //            << std::flush;

  if (frame == nullptr) {
    EUDAQ_DEBUG("frame to process == nullptr");
    return false;
  }

  auto udpTs = std::stoll(frame->GetTag("recvTS_FW", "-1"));

  if (udpTs < 0) {
    EUDAQ_WARN("no valid UDP TS in the frame");
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
    hit.udpTs = udpTs;
    hit.dcol = hi.dcol;
    hit.pix = hi.pix;
    hit.tsLe = hi.tsLe;
    hit.tsTe = hi.tsTe;
    hit.isPiggy = hi.piggy;
    hit.ovflwSOF = ovFlwSOF;
    hit.ovflwEOF = ovFlwEOF;
    hit.avgFrameOvflw = double(ovFlwEOF - ovFlwSOF) / 2.0;
    hit.originFrame = mFrameCnt;
    hit.pixIdx = hi.pixIdx;

    //    if (hi.pixIdx.col == 0 && hi.pixIdx.row == 0) {
    //      std::cout << "pix 0:0\n";
    //            continue;
    //      // should not be in data, is aweird bug in thedata collector...
    //    }

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

  std::cout << "\rprocessed frame " << mFrameCnt;
  return true;
}

void Mpw3FileReader::buildEvent(HitBuffer &in, EventBuffer &out) {

  const auto &previousHit = in.front();

  int nOvflwOfOvflw = 0;
  int evtNmb = 0;

  auto oldOvflw = in.end()->ovflwSOF;
  for (auto i = in.rbegin(); i < in.rend(); i++) {
    /* work ourselves from back to front of current UDP package, because
     * the latest hits / frames should be closest (in terms of global timestamp)
     * to the UDP-TS. Is the best way to account for overflows of the
     * overflow-counter in SOF
     */
    if (oldOvflw < i->ovflwSOF) {
      // we found an overflow, the number of ovflws of the SOF ovflw counter
      // for frames before the current frame  therefore has to be
      // decreased by one
      nOvflwOfOvflw++;
    }
    oldOvflw = i->ovflwSOF;

    i->globalTs = i->udpTs >> 16;
    i->globalTs = (i->globalTs << 16) *
                  DefsMpw3::dTPerTsLsb; // deleted lower 16 bit of UDP timestamp
    i->globalTs +=
        i->ovflwSOF * DefsMpw3::dTPerOvflw + i->tsLe * DefsMpw3::dTPerTsLsb;
    i->globalTs -= (nOvflwOfOvflw << 17);
  }

  // Now that we assigned the global timestamps we can sort them in
  // ascending order
  std::sort(in.begin(), in.end(), [](const Hit &h1, const Hit &h2) {
    return h1.globalTs < h2.globalTs;
  });

  auto endTimewindow = in.begin();

  while (in.size() > 0) { // event building based on global TS

    HitBuffer prefab;
    for (auto i = in.begin(); i < in.end(); i++) {

      if (i->globalTs - in.begin()->globalTs <=
          DefsMpw3::dTSameEvt) { // do 2 hits belong to the same event?

        prefab.push_back(*i);
        endTimewindow = i;
      } else {
        break; // we found all hits belonging to the current event
      }
    }
    //    std ::cout << "hits for event #" << evtNmb++ << ":\n"
    //               << Hit::dbgFileHeader();
    //    for (auto i = prefab.begin(); i < prefab.end(); i++) {
    //      std::cout << i->toStr() << "\n";
    //    }

    in.erase(
        in.begin(),
        endTimewindow +
            1); // processed, delete from buffer; +1 needed as we also want
                // to remove the last element, begin is inclusive, end is not

    finalizePrefab(prefab, out);
  }
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
