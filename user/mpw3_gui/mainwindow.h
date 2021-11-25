#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "monitorview.h"
#include "configcreator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
private:
    Ui::MainWindow *ui;
    MonitorView *mMonitorView;
    ConfigCreatorView *mConfigCreatorView;

    int mSensorSize;


};
#endif // MAINWINDOW_H
