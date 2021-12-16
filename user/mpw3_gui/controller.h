#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QProcess>
#include <QWidget>

namespace Ui {
  class Controller;
}

class Controller : public QWidget {
  Q_OBJECT

public:
  explicit Controller(QWidget *parent = nullptr);
  ~Controller();

private slots:
  void on_pbInitCmds_clicked();

private slots:
  void on_pbTest_clicked();

private slots:
  void on_pbClrLog_clicked();

private slots:
  void on_pbConnect_clicked();

  void sshErrorOccured(QProcess::ProcessError error);
  void sshRead();
  void sshError();

private:
  void startPeary();
  QByteArray pearyCmd(const QString &cmd, int devId = 0);

  Ui::Controller *ui;

  QProcess mSshProc;
};

#endif // CONTROLLER_H
