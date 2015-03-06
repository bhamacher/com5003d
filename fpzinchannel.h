#ifndef FPZINCHANNEL_H
#define FPZINCHANNEL_H

#include <QObject>

#include "scpiconnection.h"

namespace FPZINChannel
{
enum Commands
{
    cmdAlias,
    cmdMuxChannel,
    cmdStatus
};

}


namespace FRQInputSystem
{
    class cChannelSettings;
}


class cFPZInChannel : public cSCPIConnection
{
    Q_OBJECT

public:
    cFPZInChannel(QString description, quint8 nr, FRQInputSystem::cChannelSettings* cSettings);
    virtual void initSCPIConnection(QString leadingNodes, cSCPI *scpiInterface);

    QString& getName();
    QString& getAlias();
    QString& getDescription();
    bool isAvail();

protected slots:
    virtual void executeCommand(int cmdCode, cProtonetCommand* protoCmd);

private:
    QString m_sName; // the channel's name
    QString m_sAlias;
    QString m_sDescription; // the channel's brief description
    quint8 m_nMuxChannel;
    bool m_bAvail; // is this channel available ?

    QString m_ReadAlias(QString& sInput);
    QString m_ReadChannelStatus(QString& sInput);
    QString m_ReadMuxChannel(QString& sInput);
};

#endif // FPZINCHANNEL_H