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
#include "bencode.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseTorrent( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	if( !m_pAllowed || !m_bAllowTorrentDownloads || !( user.iAccess & ACCESS_DL ) )
	{
		pResponse->strCode = "403 Forbidden";

		return;
	}

	CAtom *pTorrent = m_pAllowed->getItem( UTIL_StringToHash( pRequest->mapParams["info_hash"] ) );

	if( pTorrent && pTorrent->isList( ) )
	{
		vector<CAtom *> vecTorrent = ( (CAtomList *)pTorrent )->getValue( );

		if( vecTorrent.size( ) == 6 )
		{
			CAtom *pFileName = vecTorrent[0];

			if( pFileName )
			{
				string strPath = m_strAllowedDir + pFileName->toString( );

				if( UTIL_CheckFile( strPath.c_str( ) ) )
				{
					pResponse->strCode = "200 OK";

					//addition by labarks
					string strFile = pRequest->strURL.substr(10);	
					if ( strFile == pFileName->toString( ) )
					{
						pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "application/x-bittorrent; name=\"" ) + UTIL_EscapedToString(strFile) + "\"" ) );
						pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", string( "attachment; filename=\"" ) + UTIL_EscapedToString(strFile) + "\"" ) );
					}
					else
					{
						pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "application/x-bittorrent; name=\"" ) + UTIL_EscapedToString(pFileName->toString( )) + "\"" ) );
						pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", string( "attachment; filename=\"" ) + UTIL_EscapedToString(pFileName->toString( )) + "\"" ) );
					}
					/* Original Source Code:
					pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "application/x-bittorrent; name=\"" ) + pFileName->toString( ) + "\"" ) );
					pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", string( "attachment; filename=\"" ) + pFileName->toString( ) + "\"" ) );
					*/
					//end addition

					// cache for awhile

					time_t tNow = time( NULL ) + m_iFileExpires * 60;
					char *szTime = asctime( gmtime( &tNow ) );
					szTime[strlen( szTime ) - 1] = '\0';

					pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );

					if( m_iDontCompressTorrent == 1 )
						pResponse->bCompressOK = false;


					string strData = UTIL_ReadFile( strPath.c_str( ) );

					if( !m_strForceAnnounceURL.empty( ) && m_bForceAnnounceOnDL )
					{
						CAtom *pData = Decode( strData );

						if( pData && pData->isDicti( ) )
						{
							( (CAtomDicti *)pData )->setItem( "announce", new CAtomString( m_strForceAnnounceURL ) );

							pResponse->strContent = Encode( pData );
						}

						delete pData;
					}
					else
						pResponse->strContent = strData;
				}
				else
					pResponse->strCode = "404 Not Found";
			}
			else
				pResponse->strCode = "404 Not Found";
		}
		else
			pResponse->strCode = "404 Not Found";
	}
	else
		pResponse->strCode = "404 Not Found";
}
