#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QGraphicsScene>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  mSensorSize = 8;

  mMonitorView = new MonitorView(mSensorSize, this);
  mConfigCreatorView = new ConfigCreatorView(this);
  mController = new Controller(this);

  ui->tabWidget->clear();
  ui->tabWidget->addTab(mMonitorView, "Monitor");
  ui->tabWidget->addTab(mConfigCreatorView, "Config");
  ui->tabWidget->addTab(mController, "Control");

  ui->tabWidget->setCurrentIndex(2);
}

MainWindow::~MainWindow() { delete ui; }
