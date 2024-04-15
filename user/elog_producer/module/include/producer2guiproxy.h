#ifndef PRODUCER2GUIPROXY_H
#define PRODUCER2GUIPROXY_H

#include "Producer.hh"

#include <QObject>

class Producer2GUIProxy : public QObject, public eudaq::Producer
{
    Q_OBJECT
public:
    explicit Producer2GUIProxy(const std::string name, const std::string &runcontrol = "",QObject *parent = nullptr);

signals:
    void initialize(const eudaq::ConfigurationSPC);
    void configure(const eudaq::ConfigurationSPC);
    void start(int runNmb);
    void stop();
    void reset();
    void terminate();


private:

    void DoInitialise() override;
    void DoConfigure() override;
    void DoStartRun() override;
    void DoStopRun() override;
    void DoReset() override;
    void DoTerminate() override;

};

#endif // PRODUCER2GUIPROXY_H
