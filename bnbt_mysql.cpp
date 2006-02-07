/***
*
* BNBT Beta 8.0 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2004 Trevor Hogan
*
* CBTT variations (C) 2003-2005 Harold Feit
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
***/

#ifdef BNBT_MYSQL

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "util.h"

MYSQL *gpMySQL;
string gstrMySQLHost;
string gstrMySQLDatabase;
string gstrMySQLUser;
string gstrMySQLPassword;
string gstrMySQLPrefix;
int giMySQLPort;

string UTIL_StringToMySQL( const string &strString )
{
	char *szMySQL = new char[strString.size( ) * 2 + 1];

	if( gpMySQL )
		mysql_real_escape_string( gpMySQL, szMySQL, strString.c_str( ), strString.size( ) );

	string strMySQL = szMySQL;

	delete [] szMySQL;

	return strMySQL;
}

CMySQLQuery :: CMySQLQuery( string strQuery )
{
	mysql_real_query( gpMySQL, strQuery.c_str( ), strQuery.size( ) );

	m_pRes = mysql_store_result( gpMySQL );

	if( mysql_errno( gpMySQL ) )
	{
		m_pRes = NULL;

		if( gbDebug )
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
	}
}

CMySQLQuery :: ~CMySQLQuery( )
{
	if( m_pRes )
		mysql_free_result( m_pRes );
}

vector<string> CMySQLQuery :: nextRow( )
{
	vector<string> vecReturn;

	if( m_pRes )
	{
		MYSQL_ROW row;

		unsigned int num_fields = mysql_num_fields( m_pRes );
		unsigned int i;

		vecReturn.reserve( num_fields );

		if( row = mysql_fetch_row( m_pRes ) )
		{
			unsigned long *lengths = mysql_fetch_lengths( m_pRes );

			for( i = 0; i < num_fields; i++ )
				vecReturn.push_back( string( row[i], lengths[i] ) );
		}
	}

	return vecReturn;
}

#endif
