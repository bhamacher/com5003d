#include "scpiconnection.h"
#include "scpidelegate.h"


cSCPIConnection::cSCPIConnection()
{
}


cSCPIConnection::~cSCPIConnection()
{
    cSCPIDelegate* ptr;
    for (int i = 0; i < m_DelegateList.count(); i++)
    {
        ptr = m_DelegateList.at(i);
        delete ptr;
    }
}



