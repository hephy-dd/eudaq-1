#include "mpw3_qt_if.h"

int Mpw3QtIf::xyPosToPlaneIdx(int x, int y) {
  /*
   * in a plane the index is equivalent to the row/col position like:
   * i | x y
   * --|-----
   * 0 | 0 0
   * 1 | 0 1
   * 2 | 0 2
   * and so on
   */
  return x * mSensorDim + y;
}

Mpw3QtIf::Mpw3QtIf(QObject *parent) : QObject(parent) {}

void Mpw3QtIf::triggerEvt(std::shared_ptr<eudaq::StandardEvent> evt) {
  Hitmap hitmap;
  auto plane = evt->GetPlane(0);

  //  plane.Print(std::cout);
  for (int i = 0; i < mSensorDim * mSensorDim; i++) {
    //    std::cout << "pixel @" << i << " =  " << plane.GetPixel(i) << "\n";
  }

  for (int x = 0; x < mSensorDim; x++) {
    QList<int> row;
    for (int y = 0; y < mSensorDim; y++) {
      int planeIdx = xyPosToPlaneIdx(x, y);

      if (planeIdx <= plane.TotalPixels()) {
        auto pixelVal = plane.GetPixel(planeIdx);
        row.append(pixelVal);
      }
    }
    hitmap.append(row);
  }

  // TODO: try to pass StdEvent, make hitmap later
  emit eventReceived(hitmap);
  //    std::cout << "emitted signal\n\n";
}

void Mpw3QtIf::annInit() { emit didInit(); }

void Mpw3QtIf::annConfig() { emit didConfig(); }

void Mpw3QtIf::annStartRun() { emit startRun(); }

void Mpw3QtIf::setSensorDim(int size) { mSensorDim = size; }
