#ifndef FPGASETTINGS_H
#define FPGASETTINGS_H

#include <QObject>
#include <QString>

#include "xmlsettings.h"

namespace FPGASettings
{
enum configstate
{
    setFPGADevNode
};
}

namespace Zera
{
namespace XMLConfig
{
    class XMLConfigReader;
}
}


class cFPGASettings : public cXMLSettings
{
    Q_OBJECT

public:
    cFPGASettings(Zera::XMLConfig::cReader *xmlread);
    QString& getDeviceNode();

public slots:
    virtual void configXMLInfo(const QString& key);

private:
    QString m_sDeviceNode;
};


#endif // FPGASETTINGS_H
