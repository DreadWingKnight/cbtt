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
#include "link.h"
#include "bencode.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseInfo( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	struct bnbttv btv = UTIL_CurrentTime( );

	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Info</title>\n";
	else
		pResponse->strContent += "<title>BNBT Tracker Info</title>\n";

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if( user.strLogin.empty( ) )
		pResponse->strContent += "<p class=\"login1_info\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
	else
		pResponse->strContent += "<p class=\"login2_info\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Info</title>\n";
	else
		pResponse->strContent += "<title>BNBT Tracker Info</title>\n";

	if( user.iAccess & ACCESS_VIEW )
	{
		time_t tNow = time( NULL );
		char *szTime = asctime( localtime( &tNow ) );
		szTime[strlen( szTime ) - 1] = '\0';

		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li><strong>Tracker Version:</strong> " + string( BNBT_VER ) + "</li>\n";
		pResponse->strContent += "<li><strong>Server Time:</strong> " + string( szTime ) + "</li>\n";
		pResponse->strContent += "<li><strong>Uptime:</strong> " + UTIL_SecondsToString( GetTime( ) ) + "</li>\n";

		if( m_pDFile )
		{
			pResponse->strContent += "<li><strong>Tracking " + CAtomLong( m_pDFile->getValuePtr( )->size( ) ).toString( ) + " Files, ";

			unsigned long iPeers = 0;

			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
			{
				if( (*i).second->isDicti( ) )
					iPeers += ( (CAtomDicti *)(*i).second )->getValuePtr( )->size( );
			}

			pResponse->strContent += CAtomLong( iPeers ).toString( ) + " Peers";

			if( m_bCountUniquePeers )
				pResponse->strContent += ", " + CAtomLong( m_pIPs->getValuePtr( )->size( ) ).toString( ) + " Unique";

			pResponse->strContent += "</strong></li>\n";
		}

		if( m_pUsers )
			pResponse->strContent += "<li><strong>" + CAtomLong( m_pUsers->getValuePtr( )->size( ) ).toString( ) + " Users</strong></li>\n";

		pResponse->strContent += "</ul>\n";
		// The Trinity Edition - Addition Begins
		// The following adds an RTT link to the Tracker Info page

		pResponse->strContent += "<p><a href=\"/index.html\">Return to Tracker</a></p>\n";

		// ------------------------------------------------- End of Addition
	}
	else
		pResponse->strContent += "<p class=\"denied\">You are not authorized to view this page.</p>\n";

	if( m_bGen )
		pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}

void CTracker :: serverResponseBencodeInfo( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
    pResponse->strCode = "200 OK";
	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/plain" ) ) );

	CAtomDicti *pData = new CAtomDicti( );
	pData->setItem("version", new CAtomString( string( BNBT_VER ) ) );
	CAtomDicti *pFeatures = new CAtomDicti( );
	pFeatures->setItem("linking", new CAtomString( "ka" ) );
	pFeatures->setItem("banning", new CAtomString( "cih" ) );
	CAtomList *pStatistics = new CAtomList();
	if( m_strDumpXMLFile != string() )
		pStatistics->addItem( new CAtomString( "XML Dump" ));
    pData->setItem("features", pFeatures);

#ifdef BNBT_MYSQL
	pStatistics->addItem( new CAtomString( "MySQL" ));
	if ( m_bMySQLOverrideDState == true )
		pData->setItem("database", new CAtomString( "MySQL Overriding DState" ) );
	else
	{
		pData->setItem("database", new CAtomString( "MySQL Statistics Dump with Flatfile dstate" ) );
		pStatistics->addItem( new CAtomString( "Dstate Dfile" ));
	}
#else
	pData->setItem("database", new CAtomString( "Flatfile Dstate" ) );
	pStatistics->addItem( new CAtomString( "Dstate Dfile" ));
#endif
	if( m_iSaveScrapeInterval > 0 )
		pStatistics->addItem( new CAtomString( "Timed scrape save" ) );

	pFeatures->setItem("statistics", pStatistics);

	if( gpLinkServer )
	{
		long m_lTrackers;
		long m_lInactiveTrackers;
		m_lTrackers = 0;
		m_lInactiveTrackers = 0;
		gpLinkServer->m_mtxLinks.Claim( );
		for( vector<CLinkClient *> :: iterator i = gpLinkServer->m_vecLinks.begin( ); i != gpLinkServer->m_vecLinks.end( ); i++ )
		{
			if( (*i)->m_bActive )
				m_lTrackers++;
			else
				m_lInactiveTrackers++;
		}
		gpLinkServer->m_mtxLinks.Release( );
		CAtomDicti *pTrackers = new CAtomDicti( );
		if( m_lTrackers > 0 )
			pTrackers->setItem("active", new CAtomLong(m_lTrackers) );
		if( m_lInactiveTrackers > 0 )
            pTrackers->setItem("inactive", new CAtomLong(m_lInactiveTrackers) );
		if( m_lTrackers + m_lInactiveTrackers > 0 )
            pTrackers->setItem("total", new CAtomLong(m_lTrackers + m_lInactiveTrackers) );
		pData->setItem("links", pTrackers);
	}
	if( gpLink )
	{
		CAtomDicti *pTrackers = new CAtomDicti( );
		pTrackers->setItem("active", new CAtomLong( 1 ) );
		pTrackers->setItem("total", new CAtomLong( 1 ) );
		pData->setItem("links", pTrackers);
	}

	if( m_pDFile )
		{
		pData->setItem("files", new CAtomLong( m_pDFile->getValuePtr( )->size( ) ) );
		unsigned long iPeers = 0;

		map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

		for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
		{
			if( (*i).second->isDicti( ) )
				iPeers += ( (CAtomDicti *)(*i).second )->getValuePtr( )->size( );
		}

		pData->setItem("peers", new CAtomLong( iPeers ) );

		if( m_bCountUniquePeers )
			pData->setItem("unique", new CAtomLong( m_pIPs->getValuePtr( )->size( ) ) );
	}

	pResponse->strContent = Encode( pData );
	pResponse->bCompressOK = false;

	delete pData;
}