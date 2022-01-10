#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->tabWidget->clear();

  mSensorSize = 8;

  mConfigCreatorView = new ConfigCreatorView(this);
  mController = new Controller(this);

  ui->tabWidget->addTab(mConfigCreatorView, "Config");
  ui->tabWidget->addTab(mController, "Control");

#ifdef BUILD_MONITOR
  mMonitorView = new MonitorView(mSensorSize, this);
  ui->tabWidget->addTab(mMonitorView, "Monitor");
#endif

  ui->tabWidget->setCurrentIndex(1);
}

MainWindow::~MainWindow() { delete ui; }
