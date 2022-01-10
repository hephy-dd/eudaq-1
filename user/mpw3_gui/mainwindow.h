#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "configcreator.h"
#include "controller.h"
#ifdef BUILD_MONITOR
#include "monitorview.h"
#endif
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
  class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
private:
  Ui::MainWindow *ui;
#ifdef BUILD_MONITOR
  MonitorView *mMonitorView;
#endif
  ConfigCreatorView *mConfigCreatorView;
  Controller *mController;

  int mSensorSize;
};
#endif // MAINWINDOW_H
