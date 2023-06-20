#ifndef ELOGRUNCTRL_H
#define ELOGRUNCTRL_H

#include "Producer.hh"
#include "elog.h"

#include <QWidget>
#include <QMap>
#include <QDateTime>
#include <QSettings>
#include <QMainWindow>

namespace Ui {
class ElogProducer;
}

class ElogProducer : public QWidget, public eudaq::Producer
{
    Q_OBJECT

public:
    explicit ElogProducer(const std::string name, const std::string &runcontrol = "", QWidget *parent = nullptr);
    ~ElogProducer();

    void DoInitialise() override;
    void DoConfigure() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoReset() override;
    void DoTerminate() override;

    static const uint32_t m_id_factory = eudaq::cstr2hash("elog");

private slots:
    void submit(bool autoSubmit = false);

private:
      using SetAtt = QPair<QString, QString>;

    struct Attribute{
        QString name;
        QStringList options;
        bool required;
    };
    using AttList = QList<Attribute>;

    void populateUi();
    void elogSetup();
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
};

#endif // ELOGRUNCTRL_H
