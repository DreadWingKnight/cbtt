//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifdef BNBT_MYSQL

#ifndef BNBT_MYSQL_H
 #define BNBT_MYSQL_H

#ifdef WIN32
 #include <mysql.h>
#else
 #include <mysql/mysql.h>
#endif

extern MYSQL *gpMySQL;
extern string gstrMySQLHost;
extern string gstrMySQLDatabase;
extern string gstrMySQLUser;
extern string gstrMySQLPassword;
extern string gstrMySQLPrefix;
extern int giMySQLPort;

string UTIL_StringToMySQL( const string &strString );

class CMySQLQuery
{
public:
	CMySQLQuery( string strQuery );
	virtual ~CMySQLQuery( );

	vector<string> nextRow( );
private:
	MYSQL_RES *m_pRes;
};

#endif

#endif
