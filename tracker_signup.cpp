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

void CTracker :: serverResponseSignup( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";
	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Tracker Sign Up</title>\n";
	else
		pResponse->strContent += "<title>BNBT Tracker Sign Up</title>\n";

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	// The Trinity Edition - Addition Begins
	// The following adds an RTT link to the SIGNUP PAGE

	pResponse->strContent += "<p><a href=\"/index.html\">Return to Tracker</a></p>\n";

	// ------------------------------------------------- End of Addition

	if( !m_bDisableLogon )
	{
		if( user.strLogin.empty( ) )
			pResponse->strContent += "<p class=\"login1_index\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
		else
			pResponse->strContent += "<p class=\"login2_index\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";
	}

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<h3>" + gstrTrackerTitle + " - Tracker Sign Up</h3>\n";
	else
		pResponse->strContent += "<h3>BNBT Tracker Sign Up</h3>\n";

	if( user.iAccess & ACCESS_SIGNUP )
	{
		if( pRequest->mapParams.find( "us_login" ) != pRequest->mapParams.end( ) &&
			pRequest->mapParams.find( "us_password" ) != pRequest->mapParams.end( ) &&
			pRequest->mapParams.find( "us_password_verify" ) != pRequest->mapParams.end( ) &&
			pRequest->mapParams.find( "us_email" ) != pRequest->mapParams.end( ) )
		{
			string strLogin = pRequest->mapParams["us_login"];
			string strPass = pRequest->mapParams["us_password"];
			string strPass2 = pRequest->mapParams["us_password_verify"];
			string strMail = pRequest->mapParams["us_email"];

			if( strLogin.empty( ) || strPass.empty( ) || strPass2.empty( ) || strMail.empty( ) )
			{
				pResponse->strContent += "<p>Unable to signup. You must fill in all the fields. Click <a href=\"/signup.html\">here</a> to return to the signup page.</p>\n";
				pResponse->strContent += "</body>\n";
				pResponse->strContent += "</html>\n";

				return;
			}
			else
			{
				if( strLogin[0] == ' ' || strLogin[strLogin.size( ) - 1] == ' ' || strLogin.size( ) > (unsigned int)m_iNameLength )
				{
					pResponse->strContent += "<p>Unable to signup. Your name must be less than " + CAtomInt( m_iNameLength ).toString( ) + " characters long and it must not start or end with spaces. Click <a href=\"/signup.html\">here</a> to return to the signup page.</p>\n";
					pResponse->strContent += "</body>\n";
					pResponse->strContent += "</html>\n";

					return;
				}

				if( strMail.find( "@" ) == string :: npos || strMail.find( "." ) == string :: npos )
				{
					pResponse->strContent += "<p>Unable to signup. Your e-mail address is invalid. Click <a href=\"/signup.html\">here</a> to return to the signup page.</p>\n";
					pResponse->strContent += "</body>\n";
					pResponse->strContent += "</html>\n";

					return;
				}

				if( strPass == strPass2 )
				{
					if( m_pUsers->getItem( strLogin ) )
					{
						pResponse->strContent += "<p>Unable to signup. The user \"" + UTIL_RemoveHTML( strLogin ) + "\" already exists. Click <a href=\"/signup.html\">here</a> to return to the signup page.</p>\n";
						pResponse->strContent += "</body>\n";
						pResponse->strContent += "</html>\n";

						return;
					}
					else
					{
						addUser( strLogin, strPass, m_iMemberAccess, strMail );

						pResponse->strContent += "<p>Thanks! You've successfully signed up! Click <a href=\"/login.html\">here</a> to login.</p>\n";
						pResponse->strContent += "</body>\n";
						pResponse->strContent += "</html>\n";

						return;
					}
				}
				else
				{
					pResponse->strContent += "<p>Unable to signup. The passwords did not match. Click <a href=\"/signup.html\">here</a> to return to the signup page.</p>\n";
					pResponse->strContent += "</body>\n";
					pResponse->strContent += "</html>\n";

					return;
				}
			}
		}

		pResponse->strContent += "<form method=\"get\" action=\"/signup.html\">\n";
		pResponse->strContent += "<p><strong>Signup</strong></p>\n";
		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li>Names must be less than " + CAtomInt( m_iNameLength ).toString( ) + " characters long</li>\n";
		pResponse->strContent += "<li>Names are case sensitive</li>\n";
		pResponse->strContent += "<li>No HTML</li>\n";
		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "<input name=\"us_login\" type=text size=24 maxlength=" + CAtomInt( m_iNameLength ).toString( ) + "> Login<br><br>\n";
		pResponse->strContent += "<input name=\"us_password\" type=password size=20> Password<br>\n";
		pResponse->strContent += "<input name=\"us_password_verify\" type=password size=20> Verify Password<br><br>\n";
		pResponse->strContent += "<input name=\"us_email\" type=text size=40> E-Mail<br><br>\n";
		pResponse->strContent += "<input type=submit value=\"Signup\">\n";
		pResponse->strContent += "</form>\n";
	}
	else
		pResponse->strContent += "<p class=\"denied\">You are not authorized to view this page.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}
