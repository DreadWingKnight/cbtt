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

#include "bnbt.h"
#include "atom.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseFile( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	string strFile = UTIL_EscapedToString( pRequest->strURL.substr( 7 ) );

	if( m_strFileDir.empty( ) || strFile.find( "..\\" ) != string :: npos || strFile.find( "../" ) != string :: npos || strFile.find( ":\\" ) != string :: npos || strFile.find( ":/" ) != string :: npos )
	{
		pResponse->strCode = "403 Forbidden";

		return;
	}

	string :: size_type iExt = strFile.rfind( "." );

	string strExt;

	if( iExt != string :: npos )
		strExt = strFile.substr( iExt );

	string strPath = m_strFileDir + strFile;

	if( UTIL_CheckFile( strPath.c_str( ) ) )
	{
		pResponse->strCode = "200 OK";

		pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[strExt] ) );

		// cache for awhile

		time_t tNow = time( NULL ) + m_iFileExpires * 60;
		char *szTime = asctime( gmtime( &tNow ) );
		szTime[strlen( szTime ) - 1] = '\0';

		pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );

		pResponse->strContent = UTIL_ReadFile( strPath.c_str( ) );
	}
	else
		pResponse->strCode = "404 Not Found";
}
