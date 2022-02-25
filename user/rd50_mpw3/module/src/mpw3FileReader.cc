#include "mpw3FileReader.h"

#include "eudaq/Logger.hh"

namespace {
  auto dummy0 = eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader,
                                                            std::string &>(
      eudaq::cstr2hash("mpw3raw"));
  auto dummy1 = eudaq::Factory<eudaq::FileReader>::Register<Mpw3FileReader,
                                                            std::string &&>(
      eudaq::cstr2hash("mpw3raw"));
  // has to be same name as mpw3FileWriter file extension to be recognized by
  // euCliConverter

} // namespace

Mpw3FileReader::Mpw3FileReader(const std::string &filename)
    : mFilename(filename) {}

eudaq::EventSPC Mpw3FileReader::GetNextEvent() {
  if (!mDes) {
    mDes.reset(new eudaq::FileDeserializer(mFilename));
    if (!mDes)
      EUDAQ_THROW("unable to open file: " + mFilename);
  }

  if (mTimeStampedEvents.size() == 0) {
    // there are no timestamped events left, let's process the next frame
    eudaq::EventUP ev = nullptr;

    while (true) {

      if (mDes->HasData()) {
        uint32_t id;
        mDes->PreRead(id);
        ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(id,
                                                                         *mDes);
      } else {
        return nullptr;
      }

      if (processFrame(ev)) {
        break;
      } else {
        EUDAQ_WARN("Error processing frame; skipping this one");
        continue;
      }
      if (!mDes->HasData()) {
        return nullptr; // we finished the whole file
      }
    }

    if (mHitBuffer.size() == 0) {
      EUDAQ_ERROR("Buffer after frame processing empty, probably a bug");
      return nullptr; // we should never get here, just to be save
    }

    /*
     * following code is quite tricky to understand, hopefully this
     * comment helps *fingers crossed* :)
     *
     * Time-Bin-Matching upcoming..
     *
     * 1) we extract hits sorted in double columns from current buffer
     * 2) we check for overflows in the specific double columns
     *    this has to be done as we cannot rely on the ovflwCnt from the
     * SOF for the whole frame 3) calculate global timestamps by
     * considering detected ovflwCnt, SOF ovflwCnt and TS_LE 4) sort
     * buffer in terms of global timestamps 5) merge hits with similar
     * timestamps to events
     */

    std::vector<std::vector<Hit>::iterator>
        hitsSortedByDCol[DefsMpw3::dimSensorCol / 2];
    for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
      for (auto i = mHitBuffer.begin(); i < mHitBuffer.end(); ++i) {
        if (i->dcol == dcol) {
          hitsSortedByDCol[dcol].push_back(
              i); // we need to keep the hit order of the original frame
                  // to detect overflows
        }
      }
    }
    for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
      const auto &currDCol = hitsSortedByDCol[dcol];
      DefsMpw3::ts_t ovflwCnt = 0;
      for (auto i = currDCol.begin() + 1; i < currDCol.end();
           ++i) // i is an iterator pointing to an iterator, quite some
                // dereferencing to do...
      {
        if ((*i)->tsLe < (*(i - 1))->tsLe) {
          ovflwCnt++; // we detected a higher TS_LE followed by a lower
                      // one
                      // => overflow pretty likely
        }

        // this is the ominous time-bin-matching, the whole reason for
        // this class btw...
        (*i)->globalTs = (ovflwCnt + (*i)->ovflwSOF) * DefsMpw3::dTPerOvflw +
                         (*i)->tsLe * DefsMpw3::dTPerTsLsb;

        //        std::cout << "timebin matching sof = " << (*i)->ovflwSOF
        //                  << " ovflwCnt = " << ovflwCnt
        //                  << " TS_LE = " << int((*i)->tsLe)
        //                  << " => global = " << (*i)->globalTs << "\n"
        //                  << std::flush;

        // FIXME: if a whole timebin is without any hits in a certain dcol
        // and TS-C's seem to be growing continuously we'll get it wrong..
        // Don't know if it is even possible to detect this :/ ..
      }
    }
    // Now that we assigned the global timestamps we can sort them in
    // ascending order
    std::sort(
        mHitBuffer.begin(), mHitBuffer.end(),
        [](const Hit &h1, const Hit &h2) { return h1.globalTs < h2.globalTs; });

    auto beginTimewindow = mHitBuffer.begin();
    auto endTimewindow = mHitBuffer.begin();
    do { // event building based on global TS
      endTimewindow =
          std::find_if(beginTimewindow, mHitBuffer.end(), [&](const Hit &hit) {
            return hit.globalTs - beginTimewindow->globalTs >
                   DefsMpw3::dTSameEvt;
          });

      PrefabEvt prefab;
      for (auto i = beginTimewindow; i < (endTimewindow - 1);
           i++) // we went 1 too far, found index no more belongs to
      // current timewindow => -1
      {
        prefab.push_back(*i);
        mHitBuffer.erase(i); // processed, delete it from buffer
      }
      finalizePrefab(prefab);

      beginTimewindow = endTimewindow;

      auto currPos = beginTimewindow - mHitBuffer.begin();

      if ((currPos > 3.0 / 4.0 * double(mHitBuffer.size())) &&
          mDes->HasData()) {
        break; // stop processing buffer at 3/4 length, upcoming hits may
               // be merged with hits from next frame to an event
        // 3/4 is a pretty arbitrary choice
        // TODO: do better, think more
      }
    } while (endTimewindow != mHitBuffer.end());

    if (mTimeStampedEvents.size() > 0) {
      auto retval = mTimeStampedEvents.front();
      mTimeStampedEvents.erase(mTimeStampedEvents.begin());
      return retval;
      // we immediately have to return an event, otherwise euCliConverter thinks
      // we are already finished
    }
  } else {
    // it seems we have alrdy prepared some events, return them
    auto retval = mTimeStampedEvents.front();
    mTimeStampedEvents.erase(mTimeStampedEvents.begin());
    return retval;
  }
  return nullptr;
}

bool Mpw3FileReader::processFrame(const eudaq::EventUP &frame) {

  if (frame == nullptr) {
    EUDAQ_DEBUG("frame to process == nullptr");
    return false;
  }

  if (frame->IsBORE() || frame->IsEORE() || frame->NumBlocks() == 0) {

    EUDAQ_DEBUG("frame to process is BORE /EORE / NumBlocks == 0");
    return false;
  }

  auto block = frame->GetBlock(0);

  std::cout << "blocksize = " << block.size() << "\n" << std::flush;

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
    EUDAQ_WARN("invalid frame (SOF / EOF)");
    return false;
  }

  for (int i = 1; i < rawData.size() - 1; i++) {
    const auto word = rawData[i];
    Hit hit;
    hit.dcol = DefsMpw3::extractDcol(word);
    hit.pix = DefsMpw3::extractPix(word);
    hit.tsLe = DefsMpw3::extractTsLe(word);
    hit.tsTe = DefsMpw3::extractTsTe(word);
    hit.ovflwSOF = ovFlwSOF;
    hit.ovflwEOF = ovFlwEOF;
    hit.avgFrameOvflw = double(ovFlwEOF - ovFlwSOF) / 2.0;
    mHitBuffer.push_back(hit);
  }

  return true;
}

void Mpw3FileReader::finalizePrefab(const PrefabEvt &prefab) {

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
    auto tot = i->tsTe - i->tsLe; // Time Over Threshold
    uint32_t tmp = (uint32_t(i->dcol) << 16) + (uint32_t(i->pix) << 8) +
                   uint32_t(tot); // store whole pixel-hit information in first
                                  // 3 bytes of 32 bit word
    data.push_back(tmp);
  }
  euEvent->AddBlock(0, data);
  mTimeStampedEvents.push_back(euEvent);
  /* this preprocessed event now contains information of
   * pixel identifiers and ToT of all hit pixel in the processed timewindow,
   * as well as the global timestamp of this event (begin + end)
   *
   * this afterwards needs to be converted by a Raw->StdEventConverter for
   * actual analysis
   */
}
