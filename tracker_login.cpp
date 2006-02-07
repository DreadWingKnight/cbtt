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

void CTracker :: serverResponseLogin( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	struct bnbttv btv = UTIL_CurrentTime( );

	if( user.strLogin.empty( ) )
	{
		pResponse->strCode = "401 Unauthorized";

		pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", "text/plain" ) );
		pResponse->mapHeaders.insert( pair<string, string>( "WWW-Authenticate", string( "Basic realm=\"" ) + gstrRealm + "\"" ) );

		pResponse->bCompressOK = false;

		pResponse->strContent += "401 Unauthorized";

		return;
	}

	pResponse->strCode = "200 OK";

	// cookies

	time_t tNow = time( NULL );

	struct tm tmFuture = *gmtime( &tNow );
	tmFuture.tm_mon++;
	mktime( &tmFuture );
	struct tm tmPast = *gmtime( &tNow );
	tmPast.tm_mon--;
	mktime( &tmPast );

	char pTime[256];
	memset( pTime, 0, sizeof( char ) * 256 );

	string strLogout = pRequest->mapParams["logout"];

	if( strLogout == "1" )
		strftime( pTime, sizeof( char ) * 256, "%a, %d-%b-%Y %H:%M:%S GMT", &tmPast );
	else
		strftime( pTime, sizeof( char ) * 256, "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );
	pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );
	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + user.strLogin + "\"; expires=" + pTime + "; path=/" ) );
	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( user.strMD5 ) + "\"; expires=" + pTime + "; path=/" ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";
	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Login</title>\n";
	else
		pResponse->strContent += "<title>BNBT Tracker Login</title>\n";

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<h3>" + gstrTrackerTitle + " - Tracker Login - " + UTIL_RemoveHTML( user.strLogin ) + "</h3>\n";
	else
		pResponse->strContent += "<h3>BNBT Login - " + UTIL_RemoveHTML( user.strLogin ) + "</h3>\n";

	if( strLogout == "1" ){
		pResponse->strContent += "<p>Logging out... You may need to close your browser window to completely logout.</p>\n";
		pResponse->strContent += "<p><a href=\"/index.html\">Return to Tracker</a></p>\n";
	}
	else
	{
		// The Trinity Edition - Modification Begins
		// The following code reorganizes and creates additional navigation links
		// when viewing the LOGIN / MY TORRENTS page (login.html)

		// Logout and RTT Link

		pResponse->strContent += "<p>You signed up on " + user.strCreated + ".</p>\n";
		pResponse->strContent += "<p><a href=\"/login.html?logout=1\">Logout</a> | <a href=\"/index.html\">Return to Tracker</a></p>\n";

		// Upload Link - ONLY DISPLAYED WHEN USER HAS UPLOAD RIGHTS
		// Sets a CSS class "uploadlink" which can be used to HIDE this link FROM ALL UPLOADERS using the following CSS command:
		// .uploadlink{display:none}

		if( user.iAccess & ACCESS_UPLOAD ) {
			pResponse->strContent += "<p class=\"uploadlink\"><a href=\"/upload.html\">Upload a New Torrent</a></p>\n";
		}

		// Administrative Links - ONLY DISPLAYED WHEN USER HAS ADMIN RIGHTS
		// Sets a CSS class "adminlinks" which can be used to HIDE this link FROM ALL ADMINS using the following CSS command:
		// .adminlinks{display:none}

		if( user.iAccess & ACCESS_ADMIN ) {
			pResponse->strContent += "<p class=\"adminlinks\"><a href=\"/users.html\">User Configuration</a> | \n";
            pResponse->strContent += "<a href=\"/admin.html\">Admin Panel</a></p>\n";
		}

		/* Original Source Code:
		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li>You signed up on " + user.strCreated + ".</li>\n";
		pResponse->strContent += "<li>Click <a href=\"/login.html?logout=1\">here</a> to logout.</li>\n";
		pResponse->strContent += "<li>Click <a href=\"/index.html\">here</a> to return to the tracker.</li>\n";
		pResponse->strContent += "</ul>\n";
		*/

		// ------------------------------------------------- End of Modification
		if( m_bDeleteOwnTorrents )
		{
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				string strDelHashString = pRequest->mapParams["del"];
				string strDelHash = UTIL_StringToHash( strDelHashString );
				string strOK = pRequest->mapParams["ok"];

				if( strDelHash.empty( ) )
				{
					pResponse->strContent += "<p>Unable to delete torrent " + strDelHashString + ". The info hash is invalid. Click <a href=\"/login.html\">here</a> to return to the login page.</p>\n";

					if( m_bGen )
						pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

					pResponse->strContent += "</body>\n";
					pResponse->strContent += "</html>\n";

					return;
				}
				else
				{
					if( strOK == "1" )
					{
						if( m_pTags )
						{
							CAtom *pTagInfo = m_pTags->getItem( strDelHash );

							if( pTagInfo && pTagInfo->isDicti( ) )
							{
								CAtom *pUploader = ( (CAtomDicti *)pTagInfo )->getItem( "uploader" );

								string strUploader;

								if( pUploader )
									strUploader = pUploader->toString( );

								if( strUploader != user.strLogin )
								{
									pResponse->strContent += "<p>Unable to delete torrent " + strDelHashString + ". You didn't upload that torrent. Click <a href=\"/login.html\">here</a> to return to the login page.</p>\n";

									if( m_bGen )
										pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

									pResponse->strContent += "</body>\n";
									pResponse->strContent += "</html>\n";

									return;
								}
							}
						}

						if( m_pAllowed )
						{
							// delete from disk

							CAtom *pList = m_pAllowed->getItem( strDelHash );

							if( pList && pList->isList( ) )
							{
								vector<CAtom *> vecTorrent = ( (CAtomList *)pList )->getValue( );

								if( vecTorrent.size( ) == 6 )
								{
									CAtom *pFileName = vecTorrent[0];

									if( pFileName )
									{
										if( m_strArchiveDir.empty( ) )
											UTIL_DeleteFile( ( m_strAllowedDir + pFileName->toString( ) ).c_str( ) );
										else
											UTIL_MoveFile( ( m_strAllowedDir + pFileName->toString( ) ).c_str( ), ( m_strArchiveDir + pFileName->toString( ) ).c_str( ) );
									}
								}
							}

							m_pAllowed->delItem( strDelHash );
							m_pDFile->delItem( strDelHash );
							deleteTag( strDelHash );

							pResponse->strContent += "<p>Deleted torrent " + strDelHashString + ". Click <a href=\"/login.html\">here</a> to return to the login page.</p>\n";

							if( m_bGen )
								pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

							pResponse->strContent += "</body>\n";
							pResponse->strContent += "</html>\n";

							return;
						}
					}
					else
					{
						pResponse->strContent += "<p>Are you sure you want to delete the torrent " + strDelHashString + "?</p>\n";
						pResponse->strContent += "<p><a href=\"/login.html?del=" + strDelHashString + "&ok=1\">OK</a></p>\n";
						pResponse->strContent += "</body>\n";
						pResponse->strContent += "</html>\n";

						return;
					}
				}
			}
		}

		if( m_pTags )
		{
			bool bFound = false;

			map<string, CAtom *> *pmapDicti = m_pTags->getValuePtr( );

			for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
			{
				if( (*i).second->isDicti( ) )
				{
					CAtom *pUploader = ( (CAtomDicti *)(*i).second )->getItem( "uploader" );

					string strUploader;

					if( pUploader )
						strUploader = pUploader->toString( );

					if( strUploader != user.strLogin )
						continue;

					if( !bFound )
					{
						pResponse->strContent += "<p>Your Torrents</p>\n";
						pResponse->strContent += "<ul>\n";

						bFound = true;
					}

					pResponse->strContent += "<li><a href=\"/stats.html?info_hash=" + UTIL_HashToString( (*i).first ) + "\">";

					string strName = UTIL_HashToString( (*i).first );

					if( m_pAllowed )
					{
						CAtom *pTorrent = m_pAllowed->getItem( (*i).first );

						if( pTorrent && pTorrent->isList( ) )
						{
							vector<CAtom *> vecTorrent = ( (CAtomList *)pTorrent )->getValue( );

							if( vecTorrent.size( ) == 6 )
							{
								if( vecTorrent[1] )
									strName = vecTorrent[1]->toString( );
							}
						}
					}

					pResponse->strContent += strName + "</a>";

					if( m_bDeleteOwnTorrents )
						pResponse->strContent += " [<a href=\"/login.html?del=" + UTIL_HashToString( (*i).first ) + "\">DELETE</a>]";

					pResponse->strContent += "</li>\n";
				}
			}

			if( bFound )
				pResponse->strContent += "</ul>\n";
		}
	}

	if( m_bGen )
		pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}
