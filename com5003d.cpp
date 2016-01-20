#include <QStateMachine>
#include <QState>
#include <QFile>
#include <QFinalState>
#include <QStringList>
#include <QDebug>
#include <QHostAddress>
#include <xmlconfigreader.h>
#include <QCoreApplication>
#include <protonetserver.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include "com5003dglobal.h"
#include "com5003d.h"
#include "pcbserver.h"
#include "debugsettings.h"
#include "ethsettings.h"
#include "i2csettings.h"
#include "fpgasettings.h"
#include "sensesettings.h"
#include "samplingsettings.h"
#include "sourcesettings.h"
#include "frqinputsettings.h"
#include "scheadsettings.h"
#include "statusinterface.h"
#include "samplinginterface.h"
#include "systeminterface.h"
#include "systeminfo.h"
#include "senseinterface.h"
#include "sourceinterface.h"
#include "frqinputinterface.h"
#include "scheadinterface.h"
#include "atmel.h"
#include "atmelwatcher.h"
#include "adjustment.h"
#include "rmconnection.h"

#ifdef SYSTEMD_NOTIFICATION
#include <systemd/sd-daemon.h>
#endif

cATMEL* pAtmel; // we take a static object for atmel connection

cCOM5003dServer::cCOM5003dServer(QObject *parent)
    :cPCBServer(parent)
{

    m_pDebugSettings = 0;
    m_pETHSettings = 0;
    m_pI2CSettings = 0;
    m_pFPGAsettings = 0;
    m_pSenseSettings = 0;
    pAtmel = 0;
    m_pAtmelWatcher = 0;
    m_pStatusInterface = 0;
    m_pSystemInterface = 0;
    m_pSenseInterface = 0;
    m_pSystemInfo = 0;
    m_pAdjHandler = 0;
    m_pRMConnection = 0;

    m_pInitializationMachine = new QStateMachine(this);

    QState* stateCONF = new QState(); // we start from here
    QFinalState* stateFINISH = new QFinalState(); // and here we finish

    stateCONF->addTransition(this, SIGNAL(abortInit()),stateFINISH); // from anywhere we arrive here if some error

    QState* statexmlConfiguration = new QState(stateCONF); // we configure our server with xml file
    QState* stateprogAtmel = new QState(stateCONF); // maybe we have to update the atmel
    QState* statewait4Atmel = new QState(stateCONF); // we synchronize on atmel running
    QState* statesetupServer = new QState(stateCONF); // we setup our server now
    stateconnect2RM = new QState(stateCONF); // we connect to resource manager
    stateconnect2RMError = new QState(stateCONF);
    stateSendRMIdentandRegister = new QState(stateCONF); // we send ident. to rm and register our resources

    stateCONF->setInitialState(statexmlConfiguration);

    statexmlConfiguration->addTransition(myXMLConfigReader, SIGNAL(finishedParsingXML(bool)), stateprogAtmel);
    stateprogAtmel->addTransition(this, SIGNAL(atmelProgrammed()), statewait4Atmel);
    statewait4Atmel->addTransition(this, SIGNAL(atmelRunning()), statesetupServer);
    statesetupServer->addTransition(this, SIGNAL(serverSetup()), stateconnect2RM);

    m_pInitializationMachine->addState(stateCONF);
    m_pInitializationMachine->addState(stateFINISH);
    m_pInitializationMachine->setInitialState(stateCONF);

    QObject::connect(statexmlConfiguration, SIGNAL(entered()), this, SLOT(doConfiguration()));
    QObject::connect(stateprogAtmel, SIGNAL(entered()), this, SLOT(programAtmelFlash()));
    QObject::connect(statewait4Atmel, SIGNAL(entered()), this, SLOT(doWait4Atmel()));
    QObject::connect(statesetupServer, SIGNAL(entered()), this, SLOT(doSetupServer()));
    QObject::connect(stateconnect2RM, SIGNAL(entered()), this, SLOT(doConnect2RM()));
    QObject::connect(stateconnect2RMError, SIGNAL(entered()), this, SLOT(connect2RMError()));
    QObject::connect(stateSendRMIdentandRegister, SIGNAL(entered()), this, SLOT(doIdentAndRegister()));
    QObject::connect(stateFINISH, SIGNAL(entered()), this, SLOT(doCloseServer()));

    m_pInitializationMachine->start();
}


cCOM5003dServer::~cCOM5003dServer()
{
    if (m_pDebugSettings) delete m_pDebugSettings;
    if (m_pETHSettings) delete m_pETHSettings;
    if (m_pI2CSettings) delete m_pI2CSettings;
    if (m_pFPGAsettings) delete m_pFPGAsettings;
    if (m_pSenseSettings) delete m_pSenseSettings;
    if (m_pSourceSettings) delete m_pSourceSettings;
    if (m_pFRQInputSettings) delete m_pFRQInputSettings;
    if (m_pSCHeadSettings) delete m_pSCHeadSettings;
    if (pAtmel) delete pAtmel;
    if (m_pAtmelWatcher) delete m_pAtmelWatcher;
    if (m_pStatusInterface) delete m_pStatusInterface;
    if (m_pSystemInterface) delete m_pSystemInterface;
    if (m_pSenseInterface) delete m_pSenseInterface;
    if (m_pSourceInterface) delete m_pSourceInterface;
    if (m_pFRQInputInterface) delete m_pFRQInputInterface;
    if (m_pSCHeadInterface) delete m_pSCHeadInterface;
    if (m_pSystemInfo) delete m_pSystemInfo;
    if (m_pAdjHandler) delete m_pAdjHandler;
    if (m_pRMConnection) delete m_pRMConnection;
}


void cCOM5003dServer::doConfiguration()
{
    QStringList args;

    quint32 sigStart;


    args = QCoreApplication::instance()->arguments();
    if (args.count() != 2) // we want exactly 1 parameter
    {
        m_nerror = parameterError;
        emit abortInit();
    }
    else
    {

        m_nFPGAfd = open("/dev/zFPGA1reg",O_RDWR);
        lseek(m_nFPGAfd,0x0,0);
        sigStart = 0;
        write(m_nFPGAfd,(char*) &sigStart, 4);
        sigStart = 1;
        write(m_nFPGAfd,(char*) &sigStart, 4);

        if (myXMLConfigReader->loadSchema(defaultXSDFile))
        {

            sigStart = 0;
            write(m_nFPGAfd,(char*) &sigStart, 4);

            // we want to initialize all settings first
            m_pDebugSettings = new cDebugSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pDebugSettings,SLOT(configXMLInfo(const QString&)));
            m_pETHSettings = new cETHSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pETHSettings,SLOT(configXMLInfo(const QString&)));
            m_pI2CSettings = new cI2CSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pI2CSettings,SLOT(configXMLInfo(const QString&)));
            m_pFPGAsettings = new cFPGASettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pFPGAsettings,SLOT(configXMLInfo(const QString&)));
            m_pSenseSettings = new cSenseSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pSenseSettings,SLOT(configXMLInfo(const QString&)));
            m_pSourceSettings = new cSourceSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pSourceSettings,SLOT(configXMLInfo(const QString&)));
            m_pSamplingSettings = new cSamplingSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pSamplingSettings,SLOT(configXMLInfo(const QString&)));
            m_pFRQInputSettings = new cFRQInputSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pFRQInputSettings,SLOT(configXMLInfo(const QString&)));
            m_pSCHeadSettings = new cSCHeadSettings(myXMLConfigReader);
            connect(myXMLConfigReader,SIGNAL(valueChanged(const QString&)),m_pSCHeadSettings,SLOT(configXMLInfo(const QString&)));

            QString s = args.at(1);
            qDebug() << s;

            sigStart = 1;
            write(m_nFPGAfd,(char*) &sigStart, 4);
            if (myXMLConfigReader->loadXML(s)) // the first parameter should be the filename
            {
                sigStart = 0;
                write(m_nFPGAfd,(char*) &sigStart, 4);
                // xmlfile ok -> nothing to do .. the configreader will emit all configuration
                // signals and after this the finishedparsingXML signal
            }
            else
            {
                m_nerror = xmlfileError;
                emit abortInit();
            }
        }
        else
        {
            m_nerror = xsdfileError;
            emit abortInit();
        }

        close(m_nFPGAfd);

    }
}


void cCOM5003dServer::programAtmelFlash()
{
    QFile atmelFile(atmelFlashfilePath);
    if (atmelFile.exists())
    {
        int fd;
        QString devNode;

        m_nerror = atmelProgError; // preset error

        devNode = m_pFPGAsettings->getDeviceNode();
        syslog(LOG_INFO,"Starting programming atmel flash\n");

        if ( (fd = open(devNode.toLatin1().data(),O_RDWR)) < 0 )
        {
            syslog(LOG_ERR,"error opening fpga device: %s\n",devNode.toLatin1().data());
            emit abortInit();
        }
        else
        {
            ulong pcbTestReg;
            int r;
            if ( (r = lseek(fd,0xffc,0)) < 0 )
            {
                syslog(LOG_ERR,"error positioning fpga device: %s\n", devNode.toLatin1().data());
                syslog(LOG_ERR,"Programming atmel failed\n");
                close(fd);
                emit abortInit();
            }

            r = read(fd,(char*) &pcbTestReg,4);
            syslog(LOG_ERR,"reading fpga adr 0xffc =  %x\n", pcbTestReg);
            if (r < 0 )
            {
                syslog(LOG_ERR,"error reading fpga device: %s\n", devNode.toLatin1().data());
                syslog(LOG_ERR,"Programming atmel failed\n");
                emit abortInit();
            }

            pcbTestReg |=  1 << (atmelResetBit-1); // set bit for atmel reset
            syslog(LOG_INFO,"writing fpga adr 0xffc =  %x\n", pcbTestReg);
            r = write(fd, (char*) &pcbTestReg,4);

            if (r < 0 )
            {
                syslog(LOG_ERR,"error writing fpga device: %s\n", devNode.toLatin1().data());
                syslog(LOG_ERR,"Programming atmel failed\n");
                emit abortInit();
            }

            usleep(100); // give atmel some time for reset

            pcbTestReg &=  ~(1 << (atmelResetBit-1)); // reset bit for atmel reset
            syslog(LOG_INFO,"writing fpga adr 0xffc =  %x\n", pcbTestReg);
            r = write(fd, (char*) &pcbTestReg,4);
            close(fd);

            if (r < 0 )
            {
                syslog(LOG_ERR,"error writing fpga device: %s\n", devNode.toLatin1().data());
                syslog(LOG_ERR,"Programming atmel failed\n");
                emit abortInit();
            }

            // atmel is reset
            usleep(100000); // now we wait for 100ms so bootloader is running definitely

            // and start writing flash
            cIntelHexFileIO IntelHexData;
            if (IntelHexData.ReadHexFile(atmelFlashfilePath))
            {
               if (pAtmel->loadFlash(IntelHexData) == cmddone)
               {
                   syslog(LOG_INFO,"Programming atmel passed\n");
               }
               else
               {
                   syslog(LOG_ERR,"error writing atmel flash\n");
                   syslog(LOG_ERR,"Programming atmel failed\n");
                   emit abortInit();
               }
            }
            else
            {
                syslog(LOG_ERR,"error reading hex file\n");
                syslog(LOG_ERR,"Programming atmel failed\n");
                emit abortInit();
            }

            // we must restart atmel now
            if (pAtmel->startProgram() == cmddone)
            {
                syslog(LOG_ERR,"Restart atmel after programming done\n");
                emit atmelProgrammed();
            }
            {
                syslog(LOG_ERR,"Restart atmel after programming failed\n");
                emit abortInit();
            }

        }

        // once the job is done, we remove the file
        atmelFile.remove();
    }
    else
        emit atmelProgrammed();
}


void cCOM5003dServer::doWait4Atmel()
{
    pAtmel = new cATMEL(m_pI2CSettings->getDeviceNode(), m_pI2CSettings->getI2CAdress(i2cSettings::atmel), m_pDebugSettings->getDebugLevel());
    m_pAtmelWatcher = new cAtmelWatcher(m_pDebugSettings->getDebugLevel(), m_pFPGAsettings->getDeviceNode(), 10000, 100);


    m_nerror = atmelError; // we preset error
    connect(m_pAtmelWatcher,SIGNAL(timeout()),this,SIGNAL(abortInit()));
    connect(m_pAtmelWatcher,SIGNAL(running()),this,SIGNAL(atmelRunning()));
    m_pAtmelWatcher->start();
}


void cCOM5003dServer::doSetupServer()
{
    pAtmel->setPLLChannel(1); // default channel m0 for pll control
    m_pSystemInfo = new cSystemInfo();
    m_pAdjHandler = new cAdjustment(m_pSystemInfo, m_pI2CSettings->getDeviceNode(), m_pDebugSettings->getDebugLevel(), m_pI2CSettings->getI2CAdress(i2cSettings::flash) );

    cPCBServer::setupServer(); // here our scpi interface gets instanciated, we need this for further steps

    scpiConnectionList.append(this); // the server itself has some commands
    scpiConnectionList.append(m_pStatusInterface = new cStatusInterface(m_pAdjHandler));
    scpiConnectionList.append(m_pSystemInterface = new cSystemInterface(this, m_pAdjHandler, m_pSystemInfo));
    scpiConnectionList.append(m_pSenseInterface = new cSenseInterface(this, m_pSenseSettings));
    scpiConnectionList.append(m_pSamplingInterface = new cSamplingInterface(this, m_pSamplingSettings));
    scpiConnectionList.append(m_pSourceInterface = new cSourceInterface(this, m_pSourceSettings));
    scpiConnectionList.append(m_pFRQInputInterface = new cFRQInputInterface(this, m_pFRQInputSettings));
    scpiConnectionList.append(m_pSCHeadInterface = new cSCHeadInterface(this, m_pSCHeadSettings));

    resourceList.append(m_pSenseInterface); // all our resources
    resourceList.append(m_pSamplingInterface);
    resourceList.append(m_pSourceInterface);
    resourceList.append(m_pFRQInputInterface);
    resourceList.append(m_pSCHeadInterface);

    m_pAdjHandler->addAdjFlashObject(m_pSenseInterface); // we add the senseinterface to both
    m_pAdjHandler->addAdjXMLObject(m_pSenseInterface); // adjustment list (flash and xml)
    m_pAdjHandler->importJDataFlash(); // we read adjustmentdata at least once

    initSCPIConnections();

    myServer->startServer(m_pETHSettings->getPort(protobufserver)); // and can start the server now
    m_pSCPIServer->listen(QHostAddress::AnyIPv4, m_pETHSettings->getPort(scpiserver));

    // our resource mananager connection must be opened after configuration is done
    m_pRMConnection = new cRMConnection(m_pETHSettings->getRMIPadr(), m_pETHSettings->getPort(resourcemanager), m_pDebugSettings->getDebugLevel());
    //connect(m_pRMConnection, SIGNAL(connectionRMError()), this, SIGNAL(abortInit()));
    // so we must complete our state machine here
    m_nRetryRMConnect = 100;
    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, SIGNAL(timeout()), this, SIGNAL(serverSetup()));

    stateconnect2RM->addTransition(m_pRMConnection, SIGNAL(connected()), stateSendRMIdentandRegister);
    stateconnect2RM->addTransition(m_pRMConnection, SIGNAL(connectionRMError()), stateconnect2RMError);
    stateconnect2RMError->addTransition(this, SIGNAL(serverSetup()), stateconnect2RM);

    emit serverSetup(); // so we enter state machine's next state
}


void cCOM5003dServer::doCloseServer()
{
    QCoreApplication::instance()->exit(m_nerror);
}


void cCOM5003dServer::doConnect2RM()
{
    m_nerror = rmConnectionError; // preset error condition
    m_pRMConnection->connect2RM();
}


void cCOM5003dServer::connect2RMError()
{
    m_nRetryRMConnect--;
    if (m_nRetryRMConnect == 0)
        emit abortInit();
    else
        m_retryTimer.start(200);
}


void cCOM5003dServer::doIdentAndRegister()
{
    m_pRMConnection->SendIdent(getName());

    for (int i = 0; i < resourceList.count(); i++)
    {
        cResource *res = resourceList.at(i);
        connect(m_pRMConnection, SIGNAL(rmAck(quint32)), res, SLOT(resourceManagerAck(quint32)) );
        res->registerResource(m_pRMConnection, m_pETHSettings->getPort(protobufserver));
    }
#ifdef SYSTEMD_NOTIFICATION
    sd_notify(0, "READY=1");
#endif
}







