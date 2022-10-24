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
    }

  } else {
    return std::move(retval);
  }
  return nullptr;
}

bool Mpw3FileReader::processFrame(const eudaq::EventUP &frame) {

  static long long prevUdpTs = -1;

  static auto lastT = std::chrono::high_resolution_clock::now();
  std::cout << "\rprocessing Frame #" << mFrameCnt << " Event #" << mEventCnt
            << std::flush;

  if (frame == nullptr) {
    EUDAQ_DEBUG("frame to process == nullptr");
    return false;
  }

  auto udpTs = std::stoll(frame->GetTag("recvTS_FW", "-1"));
  std::cout << "frame from UDP TS: " << udpTs << "\n";

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

  const auto &previousHit = in.front();

  int nOvflwOfOvflw = 0;
  int evtNmb = 0;

  for (const auto &hit : in) {
    if (hit.udpTs < previousHit.udpTs) {
      nOvflwOfOvflw++; // counting the number of overflows of the SOF ovflw
                       // counter inside one UDP package
    }
  }
  auto oldOvflw = in.end()->ovflwSOF;
  for (auto i = in.rbegin(); i < in.rend(); i++) {
    if (oldOvflw < i->ovflwSOF) {
      nOvflwOfOvflw--;
    }
    i->globalTs = i->udpTs >> 16;
    i->globalTs = i->globalTs << 16; // deleted lower 16 bit of UDP timestamp
    DefsMpw3::ts_t totalOvflws = i->ovflwSOF + nOvflwOfOvflw * 256;
    i->globalTs +=
        totalOvflws * DefsMpw3::dTPerOvflw + i->tsLe * DefsMpw3::dTPerTsLsb;
    std::cout << "set global ts to " << i->globalTs << "for pix "
              << i->pixIdx.row << ":" << i->pixIdx.col << "\n";
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
    std ::cout << "hits for event #" << evtNmb++ << ":\n"
               << Hit::dbgFileHeader();
    for (auto i = prefab.begin(); i < prefab.end(); i++) {
      std::cout << i->toStr() << "\n";
    }

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
