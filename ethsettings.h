#ifndef ETHSETTINGS_H
#define ETHSETTINGS_H

#include <QObject>

#include "xmlsettings.h"


enum ethmember
{
    server,
    resourcemanager
};


enum ethconfigstate
{
    setServerPort,
    setRMPort
};


namespace Zera
{
namespace XMLConfig
{
    class cReader;
}
}

class cETHSettings : public cXMLSettings
{
    Q_OBJECT

public:
    cETHSettings(Zera::XMLConfig::cReader *xmlread);
    quint16 getPort(ethmember member);

public slots:
    virtual void configXMLInfo(const QString& key);

private:
    quint16 m_nServerPort, m_nRMPort;
};



#endif // ETHSETTINGS_H