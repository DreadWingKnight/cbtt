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
#include "client.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseUploadGET( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	struct bnbttv btv = UTIL_CurrentTime( );

	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";
	// The Trintiy Tracker - Modification Begins
	// The following changes "BNBT File Uploader" to "BNBT Torrent Uplaoder"
	// in the Title Bar of the web browser

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Torrent Uploader</title>\n";
	else
		pResponse->strContent += "<title>BNBT Torrent Uploader</title>\n";

	/* Original Source Code:
	pResponse->strContent += "<title>BNBT File Uploader</title>\n";
	*/

	// ------------------------------------------------- End of Modification

	// The Trinity Tracker - Addition Begins
	// The following adds an RTT link
	// when viewing the UPLOAD TORRENT page (upload.html)

	pResponse->strContent += "<p><a href=\"/index.html\">Return to Tracker</a></p>\n";

	// ------------------------------------------------- End of Addition

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if( user.strLogin.empty( ) )
		pResponse->strContent += "<p class=\"login1_upload\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
	else
		pResponse->strContent += "<p class=\"login2_upload\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";

	// The Trinity Edition - Modification Begins
	// The following changes "BNBT File Uploader" to "BNBT Torrent Uploader"
	// in the <h3> tag

	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<h3>" + gstrTrackerTitle + " - Torrent Uploader</h3>\n";
	else
		pResponse->strContent += "<h3>BNBT Torrent Uploader</h3>\n";

	/* Original Source Code:
	pResponse->strContent += "<h3>BNBT File Uploader</h3>\n";
	*/

	if( user.iAccess & ACCESS_UPLOAD )
	{
		if( m_strUploadDir.empty( ) )
		{
			pResponse->strContent += "<p class=\"denied\">This tracker does not allow file uploads. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";

			if( m_bGen )
				pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

			pResponse->strContent += "</body>\n";
			pResponse->strContent += "</html>\n";

			return;
		}
		else if( m_iMaxTorrents > 0 )
		{
			if( m_pAllowed && m_pAllowed->getValuePtr( )->size( ) >= (unsigned int)m_iMaxTorrents )
				{
					pResponse->strContent += "<p class=\"denied\">This tracker has reached its torrent limit. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";

					if( m_bGen )
						pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

					pResponse->strContent += "</body>\n";
					pResponse->strContent += "</html>\n";

					return;
				}
		}

		pResponse->strContent += "<form method=\"post\" action=\"/upload.html\" enctype=\"multipart/form-data\">\n";
		// The Trinity Edition - Modification Begins
		// The following changes have been made:

		// 1. Field Descriptions now appear above the input fields
		// 2. Information regarding Optional input has been moved to a list below
		// 3. Modified the Tag field descriptor to read "Tag/Category"
		// 4. When FORCE_ANNOUNCE_URL is null/empty, the tracker's Announce URL will be displayed
		//    a CSS class "announce" is also set for this string, which can be used to HIDE the 
		//    "Announce URL" by using the following CSS command:
		//    .announce{display:none}
		// 5. Added a CANCEL button after the UPLOAD button.

		pResponse->strContent += "Torrent File to Upload<br>\n";
		pResponse->strContent += "<input name=\"torrent\" type=file size=50><br><br>\n";		
		pResponse->strContent += "Name (Optional - See Note Below)<br>\n";
		pResponse->strContent += "<input name=\"name\" type=text size=64 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + "> <br><br>\n";            

		if( m_bAllowInfoLink )
		{
            pResponse->strContent += "Informational Link (Optional - See Note Below)<br>\n";
			pResponse->strContent += "<input name=\"infolink\" type=text size=64 maxlength=" + CAtomInt( MAX_INFOLINK_LEN ).toString( ) + " value=\"\"> <br><br>\n";
		}

        if( m_vecTags.size( ) > 0 )
			pResponse->strContent += "Tag/Category <select name=\"tag\">\n";

		for( vector< pair<string, string> > :: iterator i = m_vecTags.begin( ); i != m_vecTags.end( ); i++ )
			pResponse->strContent += "<option>" + (*i).first + "\n";

		if( m_vecTags.size( ) > 0 )
			pResponse->strContent += "</select>\n";
		
		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li>If <b>Name</b> is left blank, the torrent filename will be used.</li>\n";
		pResponse->strContent += "<li>Names must be less than " + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " characters long</li>\n";
		pResponse->strContent += "<li>HTML cannot be used in these fields.</li>\n";
		pResponse->strContent += "<li>Informational Links must start with <b>http://</b></li>\n";
		pResponse->strContent += "<li><strong>Max. File Size:</strong> " + UTIL_BytesToString( giMaxRecvSize ) + "</li>\n";

		if( !m_strForceAnnounceURL.empty( ) )
			pResponse->strContent += "<li><strong>Auto Announce URL:</strong> " + UTIL_RemoveHTML( m_strForceAnnounceURL ) + "</li>\n";

		else {
            pResponse->strContent += "<script type=\"text/javascript\" language=\"javascript\">\n";
			pResponse->strContent += "document.write(\"<li class=announce><strong>Announce URL:</strong> http://\" + parent.location.host + \"/announce</li>\");";
			pResponse->strContent += "\n";
			pResponse->strContent += "</script>\n";
		}

		pResponse->strContent += "</ul>\n";
	
		pResponse->strContent += "<input type=submit value=\"Upload\">\n";
		pResponse->strContent += "<input type=button value=\"Cancel\" onClick=\"javascript:history.back()\">\n";

		/* Original Source Code:
		pResponse->strContent += "<input name=\"torrent\" type=file size=24> Torrent<br><br>\n";
		pResponse->strContent += "<input name=\"name\" type=text size=64 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + "> Name (optional - if blank, the name will be taken from the filename)<br><br>\n";

		if( m_bAllowInfoLink )
			pResponse->strContent += "<input name=\"infolink\" type=text size=64 maxlength=" + CAtomInt( MAX_INFOLINK_LEN ).toString( ) + "> Info Link (optional)<br><br>\n";

		if( m_vecTags.size( ) > 0 )
			pResponse->strContent += "<select name=\"tag\">\n";

		for( unsigned long i = 0; i < m_vecTags.size( ); i++ )
			pResponse->strContent += "<option>" + m_vecTags[i].strTag + "\n";

		if( m_vecTags.size( ) > 0 )
			pResponse->strContent += "</select> Tag\n";

		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li>Names must be less than " + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " characters long</li>\n";
		pResponse->strContent += "<li>No HTML</li>\n";
		pResponse->strContent += "<li><strong>Max. File Size:</strong> " + UTIL_BytesToString( MAX_RECV_SIZE ) + "</li>\n";

		if( !m_strForceAnnounceURL.empty( ) )
			pResponse->strContent += "<li><strong>Auto Announce URL:</strong> " + UTIL_RemoveHTML( m_strForceAnnounceURL ) + "</li>\n";

		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "<input type=submit value=\"Upload\">\n";
		*/

		// ------------------------------------------------- End of Modification

		pResponse->strContent += "</form>\n";
	}
	else
		pResponse->strContent += "<p class=\"denied\">You are not authorized to view this page.</p>\n";

	if( m_bGen )
		pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}

void CTracker :: serverResponseUploadPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost, user_t user )
{
	struct bnbttv btv = UTIL_CurrentTime( );

	if( m_strUploadDir.empty( ) || !( user.iAccess & ACCESS_UPLOAD ) )
	{
		pResponse->strCode = "403 Forbidden";

		return;
	}

	if( m_iMaxTorrents > 0 )
	{
		if( m_pAllowed && m_pAllowed->getValuePtr( )->size( ) >= (unsigned int)m_iMaxTorrents )
			{
				pResponse->strCode = "403 Forbidden";

				return;
			}
		}

	string strFile;
	string strTorrent;
	string strTag;
	string strPostedName;
	string strPostedInfoLink;

	if( pPost )
	{
		vector<CAtom *> vecSegs = pPost->getValue( );

		for( unsigned long i = 0; i < vecSegs.size( ); i++ )
		{
			if( vecSegs[i]->isDicti( ) )
			{
				CAtomDicti *pSeg = (CAtomDicti *)vecSegs[i];

				CAtom *pDisp = ( (CAtomDicti *)vecSegs[i] )->getItem( "disposition" );
				CAtom *pDat = ( (CAtomDicti *)vecSegs[i] )->getItem( "data" );

				if( pDisp && pDisp->isDicti( ) && pDat )
				{
					CAtom *pName = ( (CAtomDicti *)pDisp )->getItem( "name" );

					if( pName )
					{
						string strName = pName->toString( );

						if( strName == "torrent" )
						{
							CAtom *pFile = ( (CAtomDicti *)pDisp )->getItem( "filename" );

							if( pFile )
							{
								// the path is local to the peer

								strFile = UTIL_StripPath( pFile->toString( ) );

								strTorrent = pDat->toString( );
							}
							else
							{
								pResponse->strCode = "400 Bad Request";

								return;
							}
						}
						else if( strName == "tag" )
							strTag = pDat->toString( );
						else if( strName == "name" )
							strPostedName = pDat->toString( ).substr( 0, MAX_FILENAME_LEN );
						else if( strName == "infolink" )
							strPostedInfoLink = pDat->toString( ).substr( 0, MAX_INFOLINK_LEN );
					}
					else
					{
						pResponse->strCode = "400 Bad Request";

						return;
					}
				}
			}
		}
	}
	else
	{
		pResponse->strCode = "400 Bad Request";

		return;
	}

	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( "text/html; charset=" ) + gstrCharSet ) );

	pResponse->strContent += "<html>\n";
	pResponse->strContent += "<head>\n";
	if ( !gstrTrackerTitle.empty( ) )
		pResponse->strContent += "<title>" + gstrTrackerTitle + " - Torrent Uploader</title>\n";
	else
		pResponse->strContent += "<title>BNBT Torrent Uploader</title>\n";

	if( !gstrStyle.empty( ) )
		pResponse->strContent += "<link rel=stylesheet type=\"text/css\" href=\"" + gstrStyle + "\">\n";

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if( user.strLogin.empty( ) )
		pResponse->strContent += "<p class=\"login1_upload\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
	else
		pResponse->strContent += "<p class=\"login2_upload\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";

	string strPath = m_strUploadDir + strFile;

	string :: size_type iExt = strFile.rfind( "." );

	string strExt;

	if( iExt != string :: npos )
		strExt = strFile.substr( iExt );

	if( strTorrent.empty( ) )
	{
		pResponse->strContent += "<h3>Upload Failed</h3>\n";
		pResponse->strContent += "<p>The uploaded file is corrupt or invalid. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
	}
	else if( strExt != ".torrent" )
	{
		pResponse->strContent += "<h3>Upload Failed</h3>\n";
		pResponse->strContent += "<p>The uploaded file is not a .torrent file. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
	}
	else if( !checkTag( strTag ) )
	{
		pResponse->strContent += "<h3>Upload Failed</h3>\n";
		pResponse->strContent += "<p>The file tag is invalid. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
	}
	else if( UTIL_CheckFile( strPath.c_str( ) ) )
	{
		pResponse->strContent += "<h3>Upload Failed</h3>\n";
		pResponse->strContent += "<p>The uploaded file already exists. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
	}
	else
	{
		CAtom *pTorrent = Decode( strTorrent );

		if( pTorrent && pTorrent->isDicti( ) )
		{
			string strInfoHash = UTIL_InfoHash( pTorrent );

			if( !strInfoHash.empty( ) )
			{
				if( m_pDFile->getItem( strInfoHash ) )
				{
					pResponse->strContent += "<h3>Upload Failed</h3>\n";
					pResponse->strContent += "<p>A file with the uploaded file's info hash already exists. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
				}
				else
				{
					if( !m_strForceAnnounceURL.empty( ) )
						( (CAtomDicti *)pTorrent )->setItem( "announce", new CAtomString( m_strForceAnnounceURL ) );

					UTIL_MakeFile( strPath.c_str( ), Encode( pTorrent ) );

					addTag( strInfoHash, strTag, strPostedName, user.strLogin, strPostedInfoLink );

					pResponse->strContent += "<h3>Upload Successful</h3>\n";

					// The Trinity Edition - Addition Begins
					// The following adds an RTT link and a link to Upload Another Torrent
					// when a user has just SUCCESSFULLY UPLOADED A TORRENT - regardless of parsing method used

					pResponse->strContent += "<p><a href=\"/upload.html\">Upload Another Torrent</a> | <a href=\"/index.html\">Return To Tracker</a></p>\n\n";

					// ------------------------------------------------- End of Addition

					// The Trinity Edition - Modification Begins
					// The following removes the multiple RTT links that appear based on parsing method used

					if( m_bParseOnUpload )
					{
						if( m_pAllowed )
							parseTorrent( strPath.c_str( ) );

						pResponse->strContent += "<p>The uploaded file is ready. You should start seeding it now.</p>\n";
					}
					else
					{
						pResponse->strContent += "<p>The uploaded file will be ready in " + CAtomInt( m_iParseAllowedInterval ).toString( );

						if( m_iParseAllowedInterval == 1 )
							pResponse->strContent += " minute. You should start seeding it as soon as possible.</p>\n";
						else
							pResponse->strContent += " minutes. You should start seeding it as soon as possible.</p>\n";
					}

					/* Original Source Code:
					if( m_bParseOnUpload )
					{
						if( m_pAllowed )
							parseTorrent( strPath.c_str( ) );

						pResponse->strContent += "<p>The uploaded file is ready. You should start seeding it now. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";
					}
					else
					{
						pResponse->strContent += "<p>The uploaded file will be ready in " + CAtomInt( m_iParseAllowedInterval ).toString( );

						if( m_iParseAllowedInterval == 1 )
							pResponse->strContent += " minute. You should start seeding it as soon as possible. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";
						else
							pResponse->strContent += " minutes. You should start seeding it as soon as possible. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";
					}
					*/

					// ------------------------------------------------- End of Modification

					// The Trinity Edition - Addition Begins
					// The following displays SEEDING INSTRUCTIONS after a user successfully uploads a torrent

                    pResponse->strContent += "<h3>Seeding Instructions</h3>\n";
                    pResponse->strContent += "To begin seeding your torrent...\n";
					pResponse->strContent += "<ol>\n";
					pResponse->strContent += "<li>Locate and double-click the torrent file on your hard drive.&dagger;</li>\n";
					pResponse->strContent += "<li>Choose the save location where the torrented files exist already.</li>\n";
					pResponse->strContent += "<li>Your client will check the existing data and should then say <b>Download Complete</b>.</li>\n";
					pResponse->strContent += "<li>Your client should then contact the tracker and begin seeding to others.</li>\n";
					pResponse->strContent += "</ol>\n";
					pResponse->strContent += "<p>To verify that you have correctly started to seed your torrent,<br>\n";
                    pResponse->strContent += "<a href=\"/index.html\">Return to the Tracker</a> and examine the <u>SDs</u> (Seeders) column.</p>\n";
					pResponse->strContent += "<table><tr><td style=\"font-weight:normal; border:1px solid black; padding:10px\">\n";
					pResponse->strContent += "<p>&dagger; Depending on this tracker's configuration, the torrent may not<br>\n";
					pResponse->strContent += "show up until it has been seeded, and therefore cannot be<br>\n";
					pResponse->strContent += "initially started using the [DL] link on the tracker's main page.</p>\n";
					pResponse->strContent += "</td></tr></table>\n";

					// ------------------------------------------------- End of Addition

					UTIL_LogPrint( "%s uploaded %s\n", inet_ntoa( pRequest->sin.sin_addr ), strFile.c_str( ) );
				}
			}
			else
			{
				pResponse->strContent += "<h3>Upload Failed</h3>\n";
				pResponse->strContent += "<p>The uploaded file is corrupt or invalid. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
			}
		}
		else
		{
			pResponse->strContent += "<h3>Upload Failed</h3>\n";
			pResponse->strContent += "<p>The uploaded file is corrupt or invalid. Click <a href=\"/upload.html\">here</a> to return to the upload page.</p>\n";
		}

		if( pTorrent )
			delete pTorrent;
	}

	if( m_bGen )
		pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}
