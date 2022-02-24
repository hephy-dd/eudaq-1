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
    eudaq::EventUP ev;
    uint32_t id;
    if (mDes->HasData()) {
      mDes->PreRead(id);
      ev = eudaq::Factory<eudaq::Event>::Create<eudaq::Deserializer &>(id,
                                                                       *mDes);
      if (ev != nullptr) { // building events from single hits here
        std::cout << "read mpw3FrameEvent; blocks: " << ev->GetNumBlock()
                  << " size [0] = " << ev->GetBlock(0).size() << "\n";
        processFrame(ev);

        /*
         * following code is quite tricky to understand, hopefully this comment
         * helps *fingers crossed* :)
         *
         * Time-Bin-Matching upcoming..
         * The idea is
         * 1) sort the buffer in double columns
         * 2) sort double columns by frames, done by using average
         *    timestamp-overflow counter = (OVFLW_EOF - OVFLW_SOF) / 2 (should
         *    be unique for all frames in the current data)
         * 3) sort by timestamp-chip TS-C (= TS_LE) in a frame
         *
         * the double columns in the buffer should now be internally sorted
         * chronologically so we generate global timestamp in the dcol blocks
         *
         * Note: the weird "[]" stuff is called a lambda and is a minimalistic,
         * very local, very useful function definition, see
         * https://en.cppreference.com/w/cpp/language/lambda
         */

        /*
        std::sort(mHitBuffer.begin(), mHitBuffer.end(),
                  [](const Hit &h1, const Hit &h2) {
                    return h1.dcol < h2.dcol;
                  }); // we sorted our buffer in double columns
        auto endCurrDCol = mHitBuffer.begin();
        auto endPrevDcol = mHitBuffer.begin();
*/

        std::vector<std::vector<Hit>::iterator>
            hitsSortedByDCol[DefsMpw3::dimSensorCol / 2];
        for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
          for (auto i = mHitBuffer.begin(); i < mHitBuffer.end(); ++i) {
            if (i->dcol == dcol) {
              hitsSortedByDCol[dcol].push_back(
                  i); // we need to keep the hit order of the original frame to
                      // detect overflows
            }
          }
        }

        for (int dcol = 0; dcol < DefsMpw3::dimSensorCol / 2; dcol++) {
          const auto &currDCol = hitsSortedByDCol[dcol];
          int ovflwCnt = 0;
          for (auto i = currDCol.begin(); i < currDCol.end();
               ++i) // i is an iterator pointing to an iterator, quite some
                    // dereferencing to do...
          {
            if ((*i)->tsLe > (*(i - 1))->tsLe) {
              ovflwCnt++; // we detected a higher TS-C followed by a lower one
                          // => overflow pretty likely
            }
            (*i)->globalTs =
                (ovflwCnt + (*i)->ovflwSOF) * DefsMpw3::dTPerOvflw +
                (*i)->tsLe * DefsMpw3::dTPerTsLsb;
            // this is the ominous time-bin-matching
            // FIXME: if a whole timebin is without any events in a certain dcol
            // and TS-C's seem to be growing continuously we'll get it wrong..
            // Don't know if it is even possible to detect this :| ..
          }
        }
        // Now that we assigned the global timestamps we can sort in ascending
        // order
        std::sort(mHitBuffer.begin(), mHitBuffer.end(),
                  [](const Hit &h1, const Hit &h2) {
                    return h1.globalTs < h2.globalTs;
                  });

        auto beginTimewindow = mHitBuffer.begin();
        auto endTimewindow = mHitBuffer.begin();
        while (true) { // event building based on global TS
          endTimewindow = std::find_if(
              beginTimewindow, mHitBuffer.end(), [&](const Hit &hit) {
                return hit.globalTs - beginTimewindow->globalTs >
                       DefsMpw3::dTSameEvt;
              });

          PrefabEvt prefab;
          for (auto i = beginTimewindow; i < (endTimewindow - 1);
               ++i) // we went 1 too far, found index no more belongs to
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
            break; // stop processing buffer at 3/4 length, upcoming hits may be
                   // merged with hits from next frame to an event
            // 3/4 is a pretty arbitrary choice
            // TODO: do better, think more
          }

          if (endTimewindow == mHitBuffer.end()) {
            break; // this should only happen when no frame is left in
                   // deserializer and we finished processing everyhing
          }
        }

        /*
        while (true) { // sort by frame in dcol
          endCurrDCol =
              std::find_if(endPrevDcol, mHitBuffer.end(), [&](const Hit &hit) {
                return hit.dcol <
                       endPrevDcol
                           ->dcol; // returns true if dcol increased => we
                                   // found the end of the current dcol
              });
          std::sort(endPrevDcol, endCurrDCol, [](const Hit &h1, const Hit &h2) {
            return h1.avgFrameOvflw < h2.avgFrameOvflw;
          }); // sorted the currently processed dcol by frames

          auto endCurrFrame =
              endCurrDCol; // value not needed, just a convenience assignment to
                           // use "auto"
          auto endPrevFrame = endPrevDcol;
          while (true) { // sort by TS-C in frame
            endCurrFrame =
                std::find_if(endPrevFrame, endCurrDCol, [&](const Hit &hit) {
                  return hit.avgFrameOvflw < endPrevFrame->avgFrameOvflw;
                });

            std::sort(endPrevFrame, endCurrFrame,
                      [](const Hit &h1, const Hit &h2) {
                        return h1.tsLe < h2.tsLe;
                      }); // sorted by TS-C

            if (endCurrFrame == endCurrDCol) {
              break; // finised this double column
            }
            endPrevFrame = endCurrFrame;
          }

          // at this point the dcol is sorted chronologically
          // FIXME: in principal we could miss a whole timebin / overflow when
          // there is no hit in between... Is it even possible to compensate for
          // that?
          int ovflwCnt = 0;
          for (auto i = endPrevDcol; i < endCurrDCol; ++i) {
            if (i->tsLe < (i - 1)->tsLe) {
              // this in an overflow in TS-C
              ovflwCnt++;
            }
            i->globalTs =
                DefsMpw3::ts_t((i->ovflwSOF + ovflwCnt)) * DefsMpw3::dTPerOvfl +
                i->tsLe * DefsMpw3::dTPerTsLsb; // this is the freakin
                                                // TIME-BIN-MATCHING we did the
                                                // whole mess above for :)
                                                // hopefully it succeeded
          }

          if (endCurrDCol == mHitBuffer.end()) {
            break; // we are finished with timestamp generation
          }
          endPrevDcol = endCurrDCol;
        }
        */

        /* now after all this buffer sorting, why not just throw everything away
         * and sort completely new :D No seriously this has to be done as we
         * want to group hits into events at this point, which is done by
         * processing the global timestamp
         */
      }
    }
  } else {
    // it seems we have alrdy prepared some events, return them
    eudaq::EventSPC retval = mTimeStampedEvents.front();
    mTimeStampedEvents.erase(mTimeStampedEvents.begin());
    return retval;
  }
  return nullptr;
}

void Mpw3FileReader::processFrame(const eudaq::EventUP &frame) {

  auto block = frame->GetBlock(0);
  std::vector<uint32_t> rawData;
  const int sizeWord = sizeof(SVD::Defs::VMEData_t);

  rawData.resize(block.size() / sizeWord);
  memcpy(rawData.data(), block.data(), block.size());

  uint32_t ovFlwSOF, ovFlwEOF;

  if (DefsMpw3::isSOF(rawData.front()) && DefsMpw3::isEOF(rawData.back())) {
    // first word is SOF and last word is EOF, so far seems to be valid
    ovFlwSOF = DefsMpw3::extractOverFlowCnt(rawData.front());
    ovFlwEOF = DefsMpw3::extractOverFlowCnt(rawData.back());

  } else {
    EUDAQ_WARN("invalid frame (SOF / EOF)");
    return;
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
}

void Mpw3FileReader::finalizePrefab(const PrefabEvt &prefab) {

  auto euEvent = eudaq::Event::MakeShared("MPW3PreprocessedEvent");

  DefsMpw3::ts_t minTs = std::min_element(prefab.begin(), prefab.end(),
                                          [](const Hit &h1, const Hit &h2) {
                                            return h1.globalTs < h2.globalTs;
                                          })
                             ->globalTs;
  DefsMpw3::ts_t maxTs = std::max_element(prefab.begin(), prefab.end(),
                                          [](const Hit &h1, const Hit &h2) {
                                            return h1.globalTs < h2.globalTs;
                                          })
                             ->globalTs;

  euEvent->SetTimestamp(minTs, maxTs);
  std::vector<uint32_t> data;
  for (auto i = prefab.begin(); i < prefab.end(); ++i) {
    auto tot = i->tsTe - i->tsLe; // Time Over Threshold
    uint32_t tmp =
        (uint32_t(i->dcol) << 16) + (uint32_t(i->pix) << 8) + uint32_t(tot);
    data.push_back(tmp);
  }
  euEvent->AddBlock(0, data);
  mTimeStampedEvents.push_back(euEvent);
}
