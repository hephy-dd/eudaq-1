#ifndef MONITORVIEW_H
#define MONITORVIEW_H

#include "defaults.h"
#include "mpw3_monitor.h"

#include <QGraphicsScene>
#include <QWidget>

namespace Ui {
class MonitorView;
}

class MonitorView : public QWidget {
  Q_OBJECT

public:
  explicit MonitorView(int sensorSize, QWidget *parent = nullptr);
  ~MonitorView();

  void populateUiArray(const Hitmap &hitmap);
  void setSensorDimensions(int size);

public slots:

private slots:
  void on_pbTest_clicked();

  void on_pbConnect_clicked();

  void updateHitmap(const QList<QList<int>> &hitmap);
  void initDone();
  void configDone();
  void startRun();

  void on_pbClearLog_clicked();

private:
  Ui::MonitorView *ui;

  std::shared_ptr<Mpw3Monitor> mMpw3Monitor;
  Mpw3QtIf *mQtIf;

  int mSensorSize;
  bool mConnected = false;
  QString mPlotDataFilename;
  QGraphicsScene *mScene;

  QRgb pixelColor(int hits, int max);
  Hitmap randHitMap();
  void connectToRunCtrl(const QString &euMonName, const QString &confMonName,
                        const QString &server);
  void connectSigSlot();
  void makeGnuplotHitmap(const Hitmap &hitmap);
  void hitmapToPlotData(const Hitmap &hitmap);
};

#endif // MONITORVIEW_H
