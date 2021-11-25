#ifndef MPW3QTIF_H
#define MPW3QTIF_H

#include <QObject>

#include "defaults.h"
#include "eudaq/StandardEvent.hh"

class Mpw3QtIf : public QObject {
  Q_OBJECT

private:
  int xyPosToPlaneIdx(int x, int y);

  int mSensorDim;

public:
  explicit Mpw3QtIf(QObject *parent = nullptr);

  void triggerEvt(std::shared_ptr<eudaq::StandardEvent> evt);
  void annInit();
  void annConfig();
  void annStartRun();

  void setSensorDim(int size);

signals:
  void eventReceived(const QList<QList<int>> &hitmap);
  void didInit();
  void didConfig();
  void startRun();
};

#endif // MPW3QTIF_H
