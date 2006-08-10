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
#include "tracker.h"
#include "server.h"
#include "util.h"

void CTracker :: serverResponseAdmin( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Admin Panel</title>\n";
	else
		pResponse->strContent += "<title>BNBT Admin Panel</title>\n"; 

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if( !m_bDisableLogon )
	{
		if( user.strLogin.empty( ) )
			pResponse->strContent += "<p class=\"login1_index\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
		else
			pResponse->strContent += "<p class=\"login2_index\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";
	}

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Admin Panel</title>\n";
	else
		pResponse->strContent += "<title>BNBT Tracker Admin Panel</title>\n";

	if( user.iAccess & ACCESS_ADMIN )
	{
		//
		// kill tracker
		//

		if( pRequest->mapParams["ap_kill"] == "1" )
		{
			gpServer->Kill( );

			return;
		}

		if( pRequest->mapParams["ap_recount"] == "1" )
		{
			gpServer->getTracker( )->CountUniquePeers( );

			pResponse->strContent += "<p>Counting unique peers. Click <a href=\"/admin.html\">here</a> to return to the admin page.</p>\n";
			pResponse->strContent += "</body>\n";
			pResponse->strContent += "</html>\n";

			return;
		}
		//RSS Support - code by labarks
		if( pRequest->mapParams["ap_rss"] == "1" )
		{
			runSaveRSS( );

			pResponse->strContent += "<p>Updated RSS file(s). Click <a href=\"/admin.html\">here</a> to return to the admin page.</p>\n";
			pResponse->strContent += "</body>\n";
			pResponse->strContent += "</html>\n";

			return;
		}
		//end addition

		//
		// reset tracker link
		//

		if( pRequest->mapParams["ap_relink"] == "1" )
		{
			if( gpLink )
			{
				gpLink->Kill( );
				delete gpLink;
				gpLink = NULL;

				gpLink = new CLink;

				if( gbDebug )
					UTIL_LogPrint("Resetting Tracker Network Link At Admin Panel Request\n");
				
				pResponse->strContent += "<p>Resetting tracker link. Click <a href=\"/admin.html\">here</a> to return to the admin page.</p>\n";
			}
			else if( gpLinkServer )
			{
				//Kill server
				delete gpLinkServer;
				gpLinkServer = NULL;

				//Restart server

				gpLinkServer = new CLinkServer( );

				if( gbDebug )
					UTIL_LogPrint("Resetting Tracker Network Hub At Admin Panel Request\n");

				pResponse->strContent += "<p>Resetting link server. Click <a href=\"/admin.html\">here</a> to return to the admin page.</p>\n";
			}
			else
				pResponse->strContent += "<p>This tracker does not own a tracker link. Click <a href=\"/admin.html\">here</a> to return to the admin page.</p>\n";

			pResponse->strContent += "</body>\n";
			pResponse->strContent += "</html>\n";

			return;
		}

		//
		// clients
		//

		pResponse->strContent += "<p>Currently serving ";
		pResponse->strContent += CAtomInt( gpServer->m_vecClients.size( ) ).toString( );
		pResponse->strContent += " clients (including you)!</p>\n";

		//
		// tracker links
		//

		pResponse->strContent += "<table summary=\"tlink\">\n";
		pResponse->strContent += "<tr><th colspan=2>Tracker Links</th></tr>\n";
		pResponse->strContent += "<tr><td>Type</td>";

		if( gpLinkServer )
		{
			pResponse->strContent += "<td>Primary Tracker</td></tr>\n";
			pResponse->strContent += "<tr><td>Connections</td><td>";

			gpLinkServer->m_mtxLinks.Claim( );

			for( vector<CLinkClient *> :: iterator i = gpLinkServer->m_vecLinks.begin( ); i != gpLinkServer->m_vecLinks.end( ); i++ )
			{
				pResponse->strContent += (*i)->getName( );

				if( (*i)->m_bActive )
					pResponse->strContent += " (ACTIVE)";
				else
					pResponse->strContent += " (NOT ACTIVE)";

				if( i + 1 != gpLinkServer->m_vecLinks.end( ) )
					pResponse->strContent += "<br>";
			}

			gpLinkServer->m_mtxLinks.Release( );

			pResponse->strContent += "</td></tr>\n";
			pResponse->strContent += "<tr><td colspan=2><a href=\"/admin.html?ap_relink=1\">Reset Link Server</a>\n";
			pResponse->strContent += "</td></tr>\n";
		}
		else if( gpLink )
		{
			pResponse->strContent += "<td>Secondary Tracker</td></tr>\n";
			pResponse->strContent += "<tr><td>Connection</td><td>" + gpLink->getName( ) + "</td></tr>\n";
			pResponse->strContent += "<tr><td colspan=2><a href=\"/admin.html?ap_relink=1\">Reset Tracker Link</a>\n";
			pResponse->strContent += "</td></tr>\n";
		}
		else
			pResponse->strContent += "<td>No Link</td></tr>\n";

		pResponse->strContent += "</table>\n";

		//
		// kill tracker
		//

		pResponse->strContent += "<p><a href=\"/admin.html?ap_kill=1\">Kill Tracker</a></p>\n";
		pResponse->strContent += "<p>If you kill the tracker your connection will be dropped and no response will be sent to your browser.</p>\n";

		//
		// count unique peers
		//

		//RSS Support - Code by labarks - no need for this if it isn't enabled
		if( m_bCountUniquePeers )
			pResponse->strContent += "<p><a href=\"/admin.html?ap_recount=1\">Count Unique Peers</a></p>\n";
		/*Original Source Code:
		pResponse->strContent += "<p><a href=\"/admin.html?ap_recount=1\">Count Unique Peers</a></p>\n";
		*/
		if( m_strDumpRSSFile != "" )
			pResponse->strContent += "<p><a href=\"/admin.html?ap_rss=1\">Update RSS file(s)</a></p>\n";
		//end addition

		// The Trinity Edition - Addition Begins
		// The following adds an RTT link when viewing the ADMIN PAGE

		pResponse->strContent += "<p><a href=\"/index.html\">Return to Tracker</a></p>\n";

		// ------------------------------------------------- End of Addition
	}
	else
		pResponse->strContent += "<p class=\"denied\">You are not authorized to view this page.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}
