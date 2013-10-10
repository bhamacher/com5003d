#ifndef SENSECHANNEL_H
#define SENSECHANNEL_H

#include <QObject>
#include <QList>

#include "senserange.h"
#include "sensesettings.h"

namespace SenseChannel
{
enum Commands
{
    cmdAlias,
    cmdType,
    cmdUnit,
    cmdDspChannel,
    cmdStatus,
    cmdRange,
    cmdRangeCat
};

enum MMode
{
    modeAC,
    modeREF,
};

}


class cSCPIConnection;


class cSenseChannel : public cSCPIConnection
{
    Q_OBJECT

public:
    cSenseChannel(QString description, QString unit, SenseSystem::cChannelSettings* cSettings, quint8 nr);
    ~cSenseChannel();
    virtual void initSCPIConnection(QString leadingNodes, cSCPI *scpiInterface);

    void setRangeList(QList<cSenseRange*>& list);
    QList<cSenseRange*>& getRangeList();
    cSenseRange* getRange(QString& name);

    quint8 getAdjustmentStatus();

    QString& getName();
    QString& getAlias();
    QString& getDescription();
    void setDescription(const QString& s);
    void setUnit(QString& s);
    void setMMode(int m);
    bool isAvail();

protected slots:
    virtual void executeCommand(int cmdCode, QString& sInput, QString& sOutput);

private:
    QString m_sName; // the channels name m0...
    QString m_sAlias[2]; // the channel's alias name for example UL1 alternativ REF1
    QString m_sDescription; // the channel's brief description
    QString m_sUnit; // the channel's measured unit
    quint8 m_nCtrlChannel; // where to control the channel
    quint8 m_nDspChannel; // where to find the channel's sampled data
    bool m_bAvail; // is this channel available ?
    QList<cSenseRange*> m_RangeList;
    quint8 m_nMMode;

    QString m_ReadAlias(QString& sInput);
    QString m_ReadType(QString& sInput);
    QString m_ReadUnit(QString& sInput);
    QString m_ReadDspChannel(QString& sInput);
    QString m_ReadChannelStatus(QString& sInput);
    QString m_ReadWriteRange(QString& sInput);
    QString m_ReadRangeCatalog(QString& sInput);
};

#endif // SENSECHANNEL_H
