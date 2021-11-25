#include "eudaq/Monitor.hh"

#include "monitorview.h"
#include "ui_monitorview.h"

#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <QTime>
#include <memory>

MonitorView::MonitorView(int sensorSize, QWidget *parent)
    : QWidget(parent), ui(new Ui::MonitorView) {
  ui->setupUi(this);
  setSensorDimensions(sensorSize);
  mScene = new QGraphicsScene(this);

  ui->gvSensor->setScene(mScene);

  // connectToRunCtrl("MPW3Monitor", "mpw3_mon", "127.0.0.1");
  mPlotDataFilename = "/tmp/plot_data.dat";
}

MonitorView::~MonitorView() { delete ui; }

void MonitorView::populateUiArray(const Hitmap &hitmap) {
  // Create and init the image:
  QImage image = QImage(mSensorSize, mSensorSize, QImage::Format_RGB32);

  for (int row = 0; row < mSensorSize; row++) {
    for (int col = 0; col < mSensorSize; col++) {
      auto hits = hitmap[row][col];
      auto color = pixelColor(hits, 100);
      image.setPixel(col, row, color);
    }
  }

  // Set the image:
  mScene->addPixmap(QPixmap::fromImage(image));

  QRect shape(QPoint(0, 0), QPoint(mSensorSize, mSensorSize));
  ui->gvSensor->fitInView(shape);
}

void MonitorView::setSensorDimensions(int size) { mSensorSize = size; }

void MonitorView::updateHitmap(const QList<QList<int>> &hitmap) {
  // we cannot use the Hitmap type here, otherwise connect for this slot won't
  // work properly
  QString logMsg = "event @ " + QTime::currentTime().toString("hh:mm:ss.zzz");
  ui->tbLog->append(logMsg);

  makeGnuplotHitmap(hitmap);
}

void MonitorView::initDone() { ui->tbLog->append("Initialized"); }

void MonitorView::configDone() { ui->tbLog->append("Configured"); }

void MonitorView::startRun() {
  ui->tbLog->append("Running");
  mScene->clear();
}

QRgb MonitorView::pixelColor(int hits, int max) {
  return qRgb(static_cast<double>(hits) / static_cast<double>(max) * 256e0, 0,
              0);
}

void MonitorView::on_pbTest_clicked() { makeGnuplotHitmap(randHitMap()); }

Hitmap MonitorView::randHitMap() {
  Hitmap hitmap;
  QList<int> col;
  for (int i = 0; i < mSensorSize; i++) {
    col.clear();
    for (int j = 0; j < mSensorSize; j++) {
      int rdm = rand() % 100; // random number in (0, 100)
      col.append(rdm);
    }
    hitmap.append(col);
  }
  return hitmap;
}

void MonitorView::connectToRunCtrl(const QString &euMonName,
                                   const QString &confMonName,
                                   const QString &server) {
  /*
   * It seems only to be working properly when a factory is used to create the
   * monitor Otherwise (like just setting up monitor and "pressing" Connect)
   * leads to an exception during initialization
   */
  auto tmp = eudaq::Monitor::Make(
      euMonName.toStdString(), confMonName.toStdString(), server.toStdString());
  if (!tmp) {
    std::cout << "unknown Monitor: ";
    return;
  }
  tmp->Connect();
  auto mon = std::static_pointer_cast<eudaq::Monitor>(tmp);
  mMpw3Monitor = std::static_pointer_cast<Mpw3Monitor>(mon);

  if (mMpw3Monitor->IsConnected()) {
    ui->tbLog->append("Connected");
  }

  mQtIf = mMpw3Monitor->getMpw3QtIf();
  mQtIf->setSensorDim(mSensorSize);

  connectSigSlot();
}

void MonitorView::connectSigSlot() {
  qRegisterMetaType<QList<QList<int>>>("QList<QList<int>>");

  // connecting signal to slot, updateHitmap is called whenever the MPW3Monitor
  // receives an event
  connect(mQtIf, SIGNAL(eventReceived(QList<QList<int>>)), this,
          SLOT(updateHitmap(QList<QList<int>>)));
  connect(mQtIf, SIGNAL(didInit()), this, SLOT(initDone()));
  connect(mQtIf, SIGNAL(didConfig()), this, SLOT(configDone()));
  connect(mQtIf, SIGNAL(startRun()), this, SLOT(startRun()));
}

void MonitorView::makeGnuplotHitmap(const Hitmap &hitmap) {

  hitmapToPlotData(hitmap);

  QString gnuplotCmd = "gnuplot -e \"set autoscale xfix;"
                       "set autoscale yfix;"
                       "set autoscale cbfix; "
                       "set term png size 500, 400;"
                       "unset xlabel;"
                       "unset ylabel;"
                       //                         "unset xtics;"
                       //                         "unset ytics;"
                       "set output;" // output to std::out, this way popen works
                       "plot '" +
                       mPlotDataFilename +
                       "' matrix nonuniform with image notitle;"
                       "\"";

  /*
   * call of external program like gnuplot is rather inefficient,
   * you might think about using something like QCustomPlot if timing becomes an
   * issue
   */

  auto output = popen(gnuplotCmd.toLocal8Bit().data(),
                      "r"); // gnuplot output is piped to virtual file pointer
  QFile file;
  //    QFile testOut("/tmp/hitmap.png");
  //    testOut.open(QIODevice::WriteOnly);
  if (file.open(output, QIODevice::ReadOnly)) {
    QImage img;
    auto plotImg = file.readAll(); // this is the png image as pure data
    img = QImage::fromData(plotImg);
    mScene->addPixmap(QPixmap::fromImage(img));

    //        testOut.write(plotImg);
  } else {
    ui->tbLog->append("error reading gnuplot output");
  }
}

void MonitorView::hitmapToPlotData(const Hitmap &hitmap) {
  QFile file(mPlotDataFilename);
  if (!file.open(QIODevice::WriteOnly)) {
    ui->tbLog->append("error opening plot data file");
    return;
  }
  QTextStream out(&file);
  out << QString::number((mSensorSize)); // 1st line hast to be array size
  // and needs to be followed by position of columns (latter are dummies for our
  // use)
  for (int i = 0; i < mSensorSize; i++) {
    out << " " << i; // columns equally spread in range 0 -> nCols
  }

  out << "\n";

  /*
   * each line of data (for gnuplot) has to start with the row number
   * and is followed by the actual data for each pixel
   */
  for (int i = 0; i < mSensorSize; i++) {
    out << i; // row number
    for (int j = 0; j < mSensorSize; j++) {
      out << " " << hitmap[i][j];
    }
    out << "\n";
  }
}

void MonitorView::on_pbConnect_clicked() {
  auto runCtrlIp = ui->leRunCtrlAddr->text();
  connectToRunCtrl("MPW3Monitor", "mpw3_mon", runCtrlIp);
}

void MonitorView::on_pbClearLog_clicked() { ui->tbLog->clear(); }
