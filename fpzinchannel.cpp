#include <QList>
#include <QString>

#include <scpi.h>
#include <scpicommand.h>
#include "scpiconnection.h"
#include "frqinputsettings.h"
#include "fpzinchannel.h"
#include "protonetcommand.h"


cFPZInChannel::cFPZInChannel(QString description, quint8 nr, FRQInputSystem::cChannelSettings *cSettings)
    :m_sDescription(description)
{
    m_sName = QString("fi%1").arg(nr);
    m_sAlias = cSettings->m_sAlias;
    m_bAvail = cSettings->avail;
}


void cFPZInChannel::initSCPIConnection(QString leadingNodes, cSCPI *scpiInterface)
{
    cSCPIDelegate* delegate;

    if (leadingNodes != "")
        leadingNodes += ":";

    delegate = new cSCPIDelegate(QString("%1%2").arg(leadingNodes).arg(m_sName),"ALIAS", SCPI::isQuery, scpiInterface, FPZINChannel::cmdAlias);
    m_DelegateList.append(delegate);
    connect(delegate, SIGNAL(execute(int, cProtonetCommand*)), this, SLOT(executeCommand(int, cProtonetCommand*)));
    delegate = new cSCPIDelegate(QString("%1%2").arg(leadingNodes).arg(m_sName),"STATUS", SCPI::isQuery, scpiInterface, FPZINChannel::cmdStatus);
    m_DelegateList.append(delegate);
    connect(delegate, SIGNAL(execute(int, cProtonetCommand*)), this, SLOT(executeCommand(int, cProtonetCommand*)));
}


void cFPZInChannel::executeCommand(int cmdCode, cProtonetCommand *protoCmd)
{
    switch (cmdCode)
    {
    case FPZINChannel::cmdAlias:
        protoCmd->m_sOutput = m_ReadAlias(protoCmd->m_sInput);
        break;
    case FPZINChannel::cmdStatus:
        protoCmd->m_sOutput = m_ReadChannelStatus(protoCmd->m_sInput);
        break;
    }

    if (protoCmd->m_bwithOutput)
        emit cmdExecutionDone(protoCmd);
}


QString &cFPZInChannel::getName()
{
    return m_sName;
}


QString &cFPZInChannel::getAlias()
{
    return m_sAlias;
}


QString &cFPZInChannel::getDescription()
{
    return m_sDescription;
}


bool cFPZInChannel::isAvail()
{
    return m_bAvail;
}


QString cFPZInChannel::m_ReadAlias(QString &sInput)
{
    cSCPICommand cmd = sInput;

    if (cmd.isQuery())
        return m_sAlias;
    else
        return SCPI::scpiAnswer[SCPI::nak];
}


QString cFPZInChannel::m_ReadChannelStatus(QString &sInput)
{
    cSCPICommand cmd = sInput;

    if (cmd.isQuery())
    {
        quint32 r;
        r = ((m_bAvail) ? 0 : 2 << 31);
            return QString("%1").arg(r);
    }
    else
        return SCPI::scpiAnswer[SCPI::nak];
}

