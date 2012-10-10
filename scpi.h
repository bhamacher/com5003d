// header datei für scpi parser
// start 20.12.06 Peter Lohmer

#ifndef SCPI_H
#define SCPI_H

#include <qstring.h>
#include <qstringlist.h>
#include "com5003scpi.h"
#include "cmdinterpret.h"
#include "parse.h"


class cSCPIString: public QString {
public:    
    cSCPIString() : QString(){};
    cSCPIString (const QString s) : QString(s){};
    cSCPIString (const char* s) : QString(s){};
    
    bool operator == (const QString&);  // überladen des == operators um scpi node abkürzungen auch zu erkennen
    bool  operator != (const QString& s) { return ( !(*this == s)); }; // 
    cSCPIString& operator = (const QString& s);
    cSCPIString& operator= (const char* s);
private:
    const QString ScpiAbreviation(const QString& s); 
};

enum tNodeSpec {isUnknown=0, isKnown=1, isNode=2, isCommand=4, isQuery=8, isQuerywPar =16} ;


class cNode { // rein virtuelle basisklasse 
public:
    cNode (int,cNode*,cNode*,SCPICmdType,SCPICmdType); // konstruktor, nNodedef, pNextNode, pNewLevelNode, Cmd, Query
    virtual ~cNode(){};
    virtual cNode* TestNode( cCmdInterpreter*,char**) = 0; // testet den knoten
    int m_nNodeStat; // ergebnis des parse durchlaufes
    SCPICmdType m_nCmd;
    SCPICmdType m_nQuery;
protected:    
    cNode* pNextNode; // nächster knoten im selben level
    cNode* pNewLevelNode; // nachfolge knoten im nächsten level;
    int nNodeDef; // definiert was der knoten sein darf
};


class cNodeSCPI: public cNode {
public:
    cNodeSCPI (const char*,int,cNode*,cNode*,SCPICmdType,SCPICmdType); 
    virtual  ~cNodeSCPI(){};
    // konstruktor, sNodeName, nNodedef, pNextNode, pNewLevelNode, Cmd, Query
    virtual cNode* TestNode(cCmdInterpreter*,char**); // zeiger, zeiger auf zeiger auf inputzeile, testet den knoten
private:
    cSCPIString sNodeName; // name des knoten
};
    
#endif

