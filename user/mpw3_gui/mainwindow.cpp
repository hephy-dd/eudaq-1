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

  ui->tabWidget->clear();
  ui->tabWidget->addTab(mMonitorView, "Monitor");
  ui->tabWidget->addTab(mConfigCreatorView, "Config");

  //    ui->graphicsView->setContentsMargins(0,0,0,0);
}

MainWindow::~MainWindow() { delete ui; }
