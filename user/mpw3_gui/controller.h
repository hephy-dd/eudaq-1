#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QProcess>
#include <QTimer>
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
  void on_pbReset_clicked();

private slots:
  void on_pbPower_toggled(bool checked);

private slots:
  void on_pbConfigure_clicked();

private slots:
  void on_pbExecute_clicked();

private slots:
  void on_pbInitCmds_clicked();

private slots:
  void on_pbTest_clicked();

private slots:
  void on_pbClrLog_clicked();

private slots:
  void on_pbConnect_clicked();

  void sshErrorOccured(QProcess::ProcessError error);
  void receivedSshOutput();
  void readSshError();
  void sshTimeout();

private:
  enum class StateSsh { NotInitialized, Running, Error };
  enum class StatePeary {
    NotInitialized,
    Initialized,
    GetCommands,
    Operational
  };

  void startPeary();
  bool populateCmds(const QString &pearyOutput);
  void sshCmdExecute(const QString &command, bool cmdForPeary = false,
                     int devId = 0);

  const QString connectedFeedback = "[Controller CONNECTED]";
  const QString pearyDevName;
  const int timeOutMs = 2000;

  Ui::Controller *ui;
  QProcess mSshProc;
  QTimer mTimeOutTimer;
  StateSsh mStateSsh = StateSsh::NotInitialized;
  StatePeary mStatePeary = StatePeary::NotInitialized;
  bool mSshWaitingForResponse = false;
};

#endif // CONTROLLER_H
