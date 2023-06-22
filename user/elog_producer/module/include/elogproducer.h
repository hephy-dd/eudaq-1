#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include "Producer.hh"
#include "elog.h"
#include "producer2guiproxy.h"

#include <QWidget>
#include <QMap>
#include <QDateTime>
#include <QSettings>
#include <QMainWindow>

namespace Ui {
class ElogProducer;
}

class ElogProducer : public QWidget
{
    Q_OBJECT

public:
    explicit ElogProducer(const std::string name, const std::string &runcontrol = "", QWidget *parent = nullptr);
    ~ElogProducer();

    static const uint32_t m_id_factory = eudaq::cstr2hash("elog");
    std::string Connect();

private slots:
    void submit(bool autoSubmit = false);
    void DoInitialise(const eudaq::ConfigurationSPC &ini);
    void DoConfigure(const eudaq::ConfigurationSPC &conf);
    void DoStartRun(int runNmb);
    void DoStopRun();
    void DoReset();
    void DoTerminate();

private:
      using SetAtt = QPair<QString, QString>;

    struct Attribute{
        QString name;
        QStringList options;
        bool required;
    };
    using AttList = QList<Attribute>;

    void populateUi();
    void elogSetup(const eudaq::ConfigurationSPC &ini);
    QStringList parseElogCfgLine(const std::string &key);
    QList<SetAtt> attributesSet();
    bool saveCurrentElogSetup();

    Ui::ElogProducer *ui;
    AttList mAttributes;
    Elog mElog;
    QDateTime mStartTime, mStopTime;
    QString mConfigFile;
    QString mEventCntConn;
    QSettings mSettings;
    uint mCurrRunN;
    Producer2GUIProxy mProxy;
};

#endif // ELOGRUNCTRL_H
