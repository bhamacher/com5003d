#include <QString>
#include <scpi.h>

#include "samplerange.h"


cSampleRange::cSampleRange(QString name, quint16 srate, quint8 selcode)
    :m_sName(name), m_nSRate(srate), m_nSelCode(selcode)
{
}


void cSampleRange::initSCPIConnection(QString leadingNodes, cSCPI *scpiInterface)
{
    cSCPIDelegate* delegate;

    if (leadingNodes != "")
        leadingNodes += ":";

    delegate = new cSCPIDelegate(QString("%1%2").arg(leadingNodes).arg(m_sName),"SAMPLES", SCPI::isQuery, scpiInterface, SampleRangeSamples);
    m_DelegateList.append(delegate);
    connect(delegate, SIGNAL(execute(int,QString&,QString&)), this, SLOT(executeCommand(int,QString&,QString&)));
}


QString &cSampleRange::getName()
{
    return m_sName;
}


quint8 cSampleRange::getSelCode()
{
    return m_nSelCode;
}


void cSampleRange::executeCommand(int cmdCode, QString &sInput, QString &sOutput)
{
    switch (cmdCode)
    {
    case SampleRangeSamples:
        sOutput = m_ReadSRate(sInput);
        break;
    }
}


QString cSampleRange::m_ReadSRate(QString &sInput)
{
    cSCPICommand cmd = sInput;

    if (cmd.isQuery())
    {
        return QString("%1").arg(m_nSRate);
    }
    else
        return SCPI::scpiAnswer[SCPI::nak];
}
