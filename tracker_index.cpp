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
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseIndex( struct request_t *pRequest, struct response_t *pResponse, user_t user )
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
	//addition by labarks
	if( !m_strDumpRSSFile.empty( ) )
	{
		if( m_iDumpRSSFileMode == 0 || m_iDumpRSSFileMode == 2 )
			pResponse->strContent += "<link rel=\"alternate\" type=\"application/rss+xml\" title=\"RSS\" href=\"" + m_strDumpRSSFileDir + m_strDumpRSSFile + "\">\n";
		// There's a bug here Harold, Remove this comment when fixed
	}
	//end addition

	pResponse->strContent += "</head>\n";
	pResponse->strContent += "<body>\n";

	if( user.strLogin.empty( ) )
		pResponse->strContent += "<p class=\"login1_index\">You are not logged in. Click <a href=\"/login.html\">here</a> to login.</p>\n";
	else
		pResponse->strContent += "<p class=\"login2_index\">You are logged in as <span class=\"username\">" + UTIL_RemoveHTML( user.strLogin ) + "</span>. Click <a href=\"/login.html?logout=1\">here</a> to logout.</p>\n";

	if( user.iAccess & ACCESS_VIEW )
	{
		//
		// delete torrent
		//

		if( user.iAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				string strDelHashString = pRequest->mapParams["del"];
				string strDelHash = UTIL_StringToHash( strDelHashString );
				string strOK = pRequest->mapParams["ok"];

				if( strDelHash.empty( ) )
				{
					pResponse->strContent += "<p>Unable to delete torrent " + strDelHashString + ". The info hash is invalid. Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";

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

							pResponse->strContent += "<p>Deleted torrent " + strDelHashString + ". Click <a href=\"/index.html\">here</a> to return to the tracker.</p>\n";

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
						// The Trinity Edition - Modification Begins
						// The following replaces the OK response with a YES | NO option
						// when DELETING A TORRENT

						pResponse->strContent += "<p><a href=\"/index.html?del=" + strDelHashString + "&ok=1\">YES</a>\n";
						pResponse->strContent += " | <a href=\"javascript:history.back()\">NO</a></p>\n";

						/* Original Source Code:
						pResponse->strContent += "<p><a href=\"/index.html?del=" + pDel->toString( ) + "&ok=1\">OK</a></p>\n";
						*/

						// ------------------------------------------------- End of Modification
						pResponse->strContent += "</body>\n";
						pResponse->strContent += "</html>\n";

						return;
					}
				}
			}
		}

		pResponse->strContent += m_strStaticHeader;

		if( m_pDFile )
		{
			if( m_pDFile->isEmpty( ) )
			{
				pResponse->strContent += "<p>Not tracking any files yet!</p>\n";

				pResponse->strContent += m_strStaticFooter;

				if( m_bGen )
					pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

				pResponse->strContent += "</body>\n";
				pResponse->strContent += "</html>\n";

				return;
			}

			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			unsigned long iKeySize = pmapDicti->size( );

			// add the torrents into this structure one by one and sort it afterwards

			struct torrent_t *pTorrents = new struct torrent_t[iKeySize];

			unsigned long i = 0;

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
			{
				pTorrents[i].strInfoHash = (*it).first;
				pTorrents[i].strName = "unknown";
				pTorrents[i].strLowerName = "unknown";
				pTorrents[i].iComplete = 0;
				pTorrents[i].iDL = 0;
				pTorrents[i].iSize = 0;
				pTorrents[i].iFiles = 0;
				pTorrents[i].iComments = 0;
				pTorrents[i].iAverageLeft = 0;
				pTorrents[i].iAverageLeftPercent = 0;
				pTorrents[i].iMinLeft = 0;
				pTorrents[i].iMaxiLeft = 0;
				pTorrents[i].iCompleted = 0;
				pTorrents[i].iTransferred = 0;

				if( (*it).second->isDicti( ) )
				{
					map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

					bool bFirst = true;

					for( map<string, CAtom *> :: iterator j = pmapPeersDicti->begin( ); j != pmapPeersDicti->end( ); j++ )
					{
						if( (*j).second->isDicti( ) )
						{
							CAtom *pLeft = ( (CAtomDicti *)(*j).second )->getItem( "left" );

							if( pLeft && pLeft->isLong( ) )
							{
								int64 iLeft = ( (CAtomLong *)pLeft )->getValue( );

								if( iLeft == 0 )
									pTorrents[i].iComplete++;
								else
								{
									pTorrents[i].iDL++;

									// only calculate average / min / max on leechers

									pTorrents[i].iAverageLeft += iLeft;

									if( bFirst || iLeft < pTorrents[i].iMinLeft )
										pTorrents[i].iMinLeft = iLeft;

									if( bFirst || iLeft > pTorrents[i].iMaxiLeft )
										pTorrents[i].iMaxiLeft = iLeft;

									bFirst = false;
								}
							}
						}
					}

					if( m_pAllowed )
					{
						CAtom *pList = m_pAllowed->getItem( pTorrents[i].strInfoHash );

						if( pList && pList->isList( ) )
						{
							vector<CAtom *> vecTorrent = ( (CAtomList *)pList )->getValue( );

							if( vecTorrent.size( ) == 6 )
							{
								CAtom *pFileName = vecTorrent[0];
								CAtom *pName = vecTorrent[1];
								CAtom *pAdded = vecTorrent[2];
								CAtom *pSize = vecTorrent[3];
								CAtom *pFiles = vecTorrent[4];

								if( pFileName )
									pTorrents[i].strFileName = pFileName->toString( );

								if( pName )
								{
									// stick a lower case version in strNameLower for non case sensitive searching and sorting

									pTorrents[i].strName = pName->toString( );
									pTorrents[i].strLowerName = UTIL_ToLower( pTorrents[i].strName );
								}

								if( pAdded )
									pTorrents[i].strAdded = pAdded->toString( );

								if( pSize && dynamic_cast<CAtomLong *>( pSize ) )
									pTorrents[i].iSize = dynamic_cast<CAtomLong *>( pSize )->getValue( );

								if( pFiles && dynamic_cast<CAtomInt *>( pFiles ) )
									pTorrents[i].iFiles = dynamic_cast<CAtomInt *>( pFiles )->getValue( );
							}
						}

						if( m_bAllowComments )
						{
							if( m_pComments )
							{
								CAtom *pCommentList = m_pComments->getItem( pTorrents[i].strInfoHash );

								if( pCommentList && pCommentList->isList( ) )
									pTorrents[i].iComments = ( (CAtomList *)pCommentList )->getValuePtr( )->size( );
							}
						}
					}

					if( m_pCompleted )
					{
						CAtom *pCompleted = m_pCompleted->getItem( pTorrents[i].strInfoHash );

						if( pCompleted && pCompleted->isLong( ) )
						{
							pTorrents[i].iCompleted = (int)( (CAtomLong *)pCompleted )->getValue( );

							// size has already been found, calculate transferred

							if( m_pAllowed && m_bShowTransferred )
								pTorrents[i].iTransferred = pTorrents[i].iCompleted * pTorrents[i].iSize;
						}
					}

					if( m_pTags )
					{
						CAtom *pDicti = m_pTags->getItem( pTorrents[i].strInfoHash );

						if( pDicti && pDicti->isDicti( ) )
						{
							CAtom *pTag = ( (CAtomDicti *)pDicti )->getItem( "tag" );
							CAtom *pName = ( (CAtomDicti *)pDicti )->getItem( "name" );
							CAtom *pUploader = ( (CAtomDicti *)pDicti )->getItem( "uploader" );
							CAtom *pInfoLink = ( (CAtomDicti *)pDicti )->getItem( "infolink" );

							if( pTag )
								pTorrents[i].strTag = pTag->toString( );

							if( pName )
							{
								// this will overwrite the previous name, i.e. the filename

								pTorrents[i].strName = pName->toString( );
								pTorrents[i].strLowerName = UTIL_ToLower( pTorrents[i].strName );
							}

							if( pUploader )
								pTorrents[i].strUploader = pUploader->toString( );

							if( pInfoLink )
								pTorrents[i].strInfoLink = pInfoLink->toString( );
						}
					}

					if( m_bShowAverageLeft )
					{
						// iAverageLeft actually contains the total left at this point, so find the average

						if( pTorrents[i].iDL > 0 )
							pTorrents[i].iAverageLeft /= pTorrents[i].iDL;

						if( pTorrents[i].iSize > 0 )
							pTorrents[i].iAverageLeftPercent = (int)( ( (float)pTorrents[i].iAverageLeft / pTorrents[i].iSize ) * 100 );
					}
				}

				i++;
			}

			string strSort = pRequest->mapParams["sort"];

			if( m_bSort )
			{
				if( !strSort.empty( ) )
				{
					int iSort = atoi( strSort.c_str( ) );

					if( iSort == SORT_ANAME )
					{
						if( m_pAllowed && m_bShowNames )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByName );
					}
					else if( iSort == SORT_ACOMPLETE )
						qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByComplete );
					else if( iSort == SORT_AINCOMPLETE )
						qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByDL );
					else if( iSort == SORT_AADDED )
					{
						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByAdded );
					}
					else if( iSort == SORT_ASIZE )
					{
						if( m_pAllowed && m_bShowSize )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortBySize );
					}
					else if( iSort == SORT_AFILES )
					{
						if( m_pAllowed && m_bShowNumFiles )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByFiles );
					}
					else if( iSort == SORT_ACOMMENTS )
					{
						if( m_pAllowed && m_bAllowComments )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByComments );
					}
					else if( iSort == SORT_AAVGLEFT )
					{
						if( m_bShowAverageLeft )
						{
							if( m_pAllowed )
							{
								if( m_bShowLeftAsProgress )
									qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
								else
									qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
							}
							else
								qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByAvgLeft );
						}
					}
					else if( iSort == SORT_ACOMPLETED )
					{
						if( m_bShowCompleted )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByCompleted );
					}
					else if( iSort == SORT_ATRANSFERRED )
					{
						if( m_pAllowed && m_bShowTransferred )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByTransferred );
					}
					else if( iSort == SORT_DNAME )
					{
						if( m_pAllowed && m_bShowNames )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByName );
					}
					else if( iSort == SORT_DCOMPLETE )
						qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByComplete );
					else if( iSort == SORT_DINCOMPLETE )
						qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByDL );
					else if( iSort == SORT_DADDED )
					{
						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAdded );
					}
					else if( iSort == SORT_DSIZE )
					{
						if( m_pAllowed && m_bShowSize )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortBySize );
					}
					else if( iSort == SORT_DFILES )
					{
						if( m_pAllowed && m_bShowNumFiles )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByFiles );
					}
					else if( iSort == SORT_DCOMMENTS )
					{
						if( m_pAllowed && m_bAllowComments )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByComments );
					}
					else if( iSort == SORT_DAVGLEFT )
					{
						if( m_bShowAverageLeft )
						{
							if( m_pAllowed )
							{
								if( m_bShowLeftAsProgress )
									qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
								else
									qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
							}
							else
								qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAvgLeft );
						}
					}
					else if( iSort == SORT_DCOMPLETED )
					{
						if( m_bShowCompleted )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByCompleted );
					}
					else if( iSort == SORT_DTRANSFERRED )
					{
						if( m_pAllowed && m_bShowTransferred )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByTransferred );
					}
					else
					{
						// default action is to sort by added if we can

						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAdded );
					}
				}
				else
				{
					// default action is to sort by added if we can

					if( m_pAllowed && m_bShowAdded )
						qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAdded );
				}
			}
			else
			{
				// sort is disabled, but default action is to sort by added if we can

				if( m_pAllowed && m_bShowAdded )
					qsort( pTorrents, iKeySize, sizeof( struct torrent_t ), dsortByAdded );
			}

			// some preliminary search crap

			string strSearch = pRequest->mapParams["search"];
			string strLowerSearch = UTIL_ToLower( strSearch );
			string strSearchResp = UTIL_StringToEscaped( strSearch );

			if( !strSearch.empty( ) && m_pAllowed && m_bShowNames && m_bSearch )
				pResponse->strContent += "<p class=\"search_results\">Search results for \"" + UTIL_RemoveHTML( strSearch ) + "\".</p>\n";

			// filters

			string strFilter = pRequest->mapParams["filter"];

			if( !m_vecTags.empty( ) )
			{
				pResponse->strContent += "<p align=\"center\">";

				bool bFound = false;

				for( vector< pair<string, string> > :: iterator i = m_vecTags.begin( ); i != m_vecTags.end( ); i++ )
				{
					if( !bFound )
						// The Trinity Edition - Modification Begins
						// The following changes the CLEAR FILTER link to read
						// CLEAR FILTER AND SEARCH RESULTS which appears before the
						// Table of Torrents

						// It also sets a CSS class "clearfilter" which can be used
						// to HIDE this link using the following CSS command 
						// .clearfilter{display:none}

						pResponse->strContent += "<p class=\"clearfilter\"><a href=\"/index.html\">Clear Filter and Search Results</a></p>\n";

						/* Original Source Code:
						pResponse->strContent += "<a href=\"/index.html\">Clear Filter</a><br><br>\n";
						*/

						// ------------------------------------------------- End of Modification

					//addition by labarks
					pResponse->strContent += "<a title=\"" + (*i).first + "\" href=\"/index.html?filter=" + UTIL_StringToEscaped( (*i).first );
					/* Original Source Code:
					pResponse->strContent += "<a href=\"/index.html?filter=" + UTIL_StringToEscaped( (*i).first );
					*/
					//end addition

					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					pResponse->strContent += "\">";

					if( !(*i).second.empty( ) )
						pResponse->strContent += "<img class=\"tag\" src=\"" + (*i).second + "\" border=\"0\">";
					else
						pResponse->strContent += UTIL_RemoveHTML( (*i).first );

					pResponse->strContent += "</a>";

					//RSS Support - Code by labarks
					if( ( m_iDumpRSSFileMode == 1 || m_iDumpRSSFileMode == 2 ) && m_strDumpRSSFileDir != "" )
					{
						string :: size_type iExt = m_strDumpRSSFile.rfind( "." );

						string strExt;

						if( iExt != string :: npos )
							strExt = m_strDumpRSSFile.substr( iExt );
						string strRSSFile = UTIL_StripPath( m_strDumpRSSFile );
						strRSSFile = m_strDumpRSSFileDir + strRSSFile.substr( 0, strRSSFile.length() - strExt.length() ) + "-" + (*i).first + strExt;

						pResponse->strContent += "<span class=\"dash\">&nbsp;-&nbsp;</span><a class=\"rss\" href=\"" + strRSSFile + "\">RSS</a>";
					}
					//end addition

					if( i + 1 != m_vecTags.end( ) )
						pResponse->strContent += " <span class=\"pipe\">|</span> ";

					bFound = true;
				}

				pResponse->strContent += "</p>\n";
			}

			// which page are we viewing

			unsigned int iStart = 0;
			unsigned int m_override_iPerPage;
			string strPerPage = pRequest->mapParams["per_page"];
			if ( strPerPage.empty() )
				m_override_iPerPage = m_iPerPage;
			else
				m_override_iPerPage = atoi(strPerPage.c_str() );


			if( m_override_iPerPage > 0 )
			{
				string strPage = pRequest->mapParams["page"];

				if( !strPage.empty( ) )
					iStart = atoi( strPage.c_str( ) ) * m_override_iPerPage;

				// The Trinity Edition - Modification Begins
				// The following code changes "Page" to "You are viewing page" which appears before the current page number

				// It also creates an internal document link, called "Jump to Page Navigation and Torrent Search" that will
				// bring the user to the bottom of the Table of Torrents

				// Sets a CSS class "pagenavjumplink" which can be used to HIDE this link
				// using the following CSS command: .pagenavjumplink{display:none}
				
                pResponse->strContent += "<p class=\"pagenum_top\">You are viewing page " + CAtomInt( ( iStart / m_override_iPerPage ) + 1 ).toString( ) + "\n";
                pResponse->strContent += "<span class=\"pagenavjumplink\">| <a href=\"#search\">Jump to Page Navigation and Torrent Search</a></span></p>\n";

				/* Original Source Code:
				pResponse->strContent += "<p class=\"pagenum_top\">Page " + CAtomInt( ( iStart / m_override_iPerPage ) + 1 ).toString( ) + "</p>\n";
				*/

				// ------------------------------------------------- End of Modification
			}

			bool bFound = false;

			int iAdded = 0;
			int iSkipped = 0;

			// for correct page numbers after searching

			int iFound = 0;

			for( unsigned long i = 0; i < iKeySize; i++ )
			{
				if( !strFilter.empty( ) )
				{
					// only display entries that match the filter

					if( pTorrents[i].strTag != strFilter )
						continue;
				}

				if( !strSearch.empty( ) )
				{
					// only display entries that match the search

					if( pTorrents[i].strLowerName.find( strLowerSearch ) == string :: npos )
						continue;
				}

				iFound++;

				if( m_override_iPerPage == 0 || iAdded < m_override_iPerPage )
				{
					if( !bFound )
					{
						// output table headers

						pResponse->strContent += "<table summary=\"files\">\n";
						pResponse->strContent += "<tr>";

						// <th> tag

						if( !m_vecTags.empty( ) )
							pResponse->strContent += "<th>Tag</th>";

						// <th> info hash

						if( m_bShowInfoHash )
							pResponse->strContent += "<th class=\"hash\">Info Hash</th>";

						// <th> name

						if( m_pAllowed && m_bShowNames )
						{
							pResponse->strContent += "<th class=\"name\">Name";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ANAME;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DNAME;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> torrent

						if( m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) && !m_bSwapTorrentLink )
							pResponse->strContent += "<th class=\"download\">Torrent</th>\n";

						// <th> Stats

						if( m_pAllowed && m_bShowStats && ( user.iAccess & ACCESS_DL ) && m_bSwapTorrentLink )
							pResponse->strContent += "<th class=\"download\">Stats</th>\n";


						// <th> comments

						if( m_pAllowed && m_bAllowComments )
						{
							if( m_pComments )
							{
								pResponse->strContent += "<th class=\"number\">Comments";

								if( m_bSort )
								{
									pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ACOMMENTS;

									if( !strSearch.empty( ) )
										pResponse->strContent += "&search=" + strSearchResp;

									if( !strFilter.empty( ) )
										pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

									if( !strPerPage.empty( ) )
										pResponse->strContent += "&per_page=" + strPerPage;


									pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DCOMMENTS;

									if( !strSearch.empty( ) )
										pResponse->strContent += "&search=" + strSearchResp;

									if( !strFilter.empty( ) )
										pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

									if( !strPerPage.empty( ) )
										pResponse->strContent += "&per_page=" + strPerPage;

									pResponse->strContent += "\">Z</a>";
								}

								pResponse->strContent += "</th>";
							}
						}

						// <th> added

						if( m_pAllowed && m_bShowAdded )
						{
							pResponse->strContent += "<th class=\"date\">Added";
							
							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_AADDED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DADDED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> size

						if( m_pAllowed && m_bShowSize )
						{
							pResponse->strContent += "<th class=\"bytes\">Size";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ASIZE;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DSIZE;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> files

						if( m_pAllowed && m_bShowNumFiles )
						{
							pResponse->strContent += "<th class=\"number\">Files";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_AFILES;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DFILES;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> seeders

						pResponse->strContent += "<th class=\"number\">SDs";
						
						if( m_bSort )
						{
							pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ACOMPLETE;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

							if( !strPerPage.empty( ) )
								pResponse->strContent += "&per_page=" + strPerPage;

							pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DCOMPLETE;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

							if( !strPerPage.empty( ) )
								pResponse->strContent += "&per_page=" + strPerPage;

							pResponse->strContent += "\">Z</a>";
						}

						pResponse->strContent += "</th>";

						// <th> leechers
						
						pResponse->strContent += "<th class=\"number\">DLs";
						
						if( m_bSort )
						{
							pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_AINCOMPLETE;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

							if( !strPerPage.empty( ) )
								pResponse->strContent += "&per_page=" + strPerPage;

							pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DINCOMPLETE;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

							if( !strPerPage.empty( ) )
								pResponse->strContent += "&per_page=" + strPerPage;

							pResponse->strContent += "\">Z</a>";
						}

						pResponse->strContent += "</th>";

						// <th> completed

						if( m_bShowCompleted )
						{
							pResponse->strContent += "<th class=\"number\">Completed";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ACOMPLETED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DCOMPLETED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> transferred

						if( m_pAllowed && m_bShowTransferred )
						{
							pResponse->strContent += "<th class=\"bytes\">Transferred";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_ATRANSFERRED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DTRANSFERRED;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> min left

						if( m_bShowMinLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\">Min Progress</th>";
							else
								pResponse->strContent += "<th class=\"percent\">Min Left</th>";
						}

						// <th> average left

						if( m_bShowAverageLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\">Average Progress";
							else
								pResponse->strContent += "<th class=\"percent\">Average Left";

							if( m_bSort )
							{
								pResponse->strContent += "<br><a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_AAVGLEFT;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">A</a> <a class=\"sort\" href=\"/index.html?sort=" + SORTSTR_DAVGLEFT;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

								if( !strPerPage.empty( ) )
									pResponse->strContent += "&per_page=" + strPerPage;

								pResponse->strContent += "\">Z</a>";
							}

							pResponse->strContent += "</th>";
						}

						// <th> maxi left

						if( m_bShowMaxiLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\">Max Progress</th>";
							else
								pResponse->strContent += "<th class=\"percent\">Max Left</th>";
						}

						// <th> uploader

						if( m_bShowUploader )
							pResponse->strContent += "<th class=\"name\">Uploader</th>";

						// <th> info link

						if( m_bAllowInfoLink )
							pResponse->strContent += "<th class=\"infolink\">Info Link</th>";

						// <th> admin

						if( user.iAccess & ACCESS_EDIT )
						{
							if( m_pAllowed )
								pResponse->strContent += "<th>Admin</th>";
						}

						pResponse->strContent += "</tr>\n";

						bFound = true;
					}

					if( iSkipped == (int)iStart )
					{
						// output table rows

						if( iAdded % 2 )
							pResponse->strContent += "<tr class=\"even\">";
						else
							pResponse->strContent += "<tr class=\"odd\">";

						// <td> tag

						if( !m_vecTags.empty( ) )
						{
							string strTemp = pTorrents[i].strTag;

							for( vector< pair<string, string> > :: iterator j = m_vecTags.begin( ); j != m_vecTags.end( ); j++ )
							{
								if( (*j).first == pTorrents[i].strTag && !(*j).second.empty( ) ){
									//addition by labarks
									pTorrents[i].strTag = "<img src=\"" + (*j).second + "\" border=\"0\"/>";
									/* Original source code:
									pTorrents[i].strTag = "<img src=\"" + (*j).second + "\">";
									*/
								}
							}

							if( pTorrents[i].strTag == strTemp )
								pTorrents[i].strTag = UTIL_RemoveHTML( pTorrents[i].strTag );

							pResponse->strContent += "<td class=\"tag\">";

							// Link code for tag Filters
							pResponse->strContent += "<a title=\"" + strTemp + "\" href=\"/index.html?filter=" + UTIL_StringToEscaped( strTemp );

							if( !strSort.empty( ) )
								pResponse->strContent += "&sort=" + strSort;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&search=" + strSearchResp;

							pResponse->strContent += "\">";
							// End link code for tag Filters
							pResponse->strContent += pTorrents[i].strTag;
							pResponse->strContent += "</a>";
							pResponse->strContent += "</td>";
						}

						// <td> info hash

						if( m_bShowInfoHash )
						{
							pResponse->strContent += "<td class=\"hash\">";

							if( m_bShowStats && !m_bSwapTorrentLink )
								pResponse->strContent += "<a class=\"stats\" href=\"/stats.html?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\">";
							if( m_bSwapTorrentLink && m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) )
							{
								pResponse->strContent += "<a class=\"download\" href=\"";
								if( m_strExternalTorrentDir.empty( ) )
									pResponse->strContent += "/torrents/" + UTIL_StringToEscapedStrict( pTorrents[i].strFileName ) + "?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash );
								else
									pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( pTorrents[i].strFileName );
                                pResponse->strContent += "\">";
							}

							pResponse->strContent += UTIL_HashToString( pTorrents[i].strInfoHash );

							if( m_bShowStats || ( m_bSwapTorrentLink && m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) ) )
								pResponse->strContent += "</a>";

							pResponse->strContent += "</td>";
						}

						// <td> name

						if( m_pAllowed && m_bShowNames )
						{
							pResponse->strContent += "<td class=\"name\">";

							if( m_bShowStats && !m_bSwapTorrentLink )
							{
								pResponse->strContent += "<a class=\"stats\" href=\"/stats.html?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\">";
								if( m_strStatsLinkImage != "" )
									pResponse->strContent += "<img src=\"" + m_strStatsLinkImage + "\" border=0 alt=\"Statistics\">";
							}

							if( m_bSwapTorrentLink && m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) )
							{
								pResponse->strContent += "<a class=\"download\" href=\"";
								if( m_strExternalTorrentDir.empty( ) )
									pResponse->strContent += "/torrents/" + UTIL_StringToEscapedStrict( pTorrents[i].strFileName ) + "?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash );
								else
									pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( pTorrents[i].strFileName );
								pResponse->strContent += "\">";

								if( m_strDownloadLinkImage != "" )
									pResponse->strContent += "<img src=\"" + m_strDownloadLinkImage + "\" border=0 alt=\"Download Torrent\">";
							}

							pResponse->strContent += UTIL_RemoveHTML( pTorrents[i].strName );

							if( m_bShowStats || ( m_bSwapTorrentLink && m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) ) )
								pResponse->strContent += "</a>";

							pResponse->strContent += "</td>";
						}

						// <td> torrent

						if( m_pAllowed && m_bAllowTorrentDownloads && ( user.iAccess & ACCESS_DL ) && !m_bSwapTorrentLink )
						{
							pResponse->strContent += "<td class=\"download\"><a class=\"download\" href=\"";

							// Removed previous code for clarity, /torrent.html support is present for legacy link support ONLY. all links are generated with /torrents/<torrentname>?info_hash=<hash> for clarity
							if( m_strExternalTorrentDir.empty( ) )
								pResponse->strContent += "/torrents/" + UTIL_StringToEscapedStrict( pTorrents[i].strFileName ) + "?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash );
							else
								pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( pTorrents[i].strFileName );

							if( m_strDownloadLinkImage == "" )
								pResponse->strContent += "\">DL</a></td>";
							else
								pResponse->strContent += "\"><img src=\"" + m_strDownloadLinkImage + "\" border=0 alt=\"Download Torrent\"></a></td>";
						}

						// <td> Torrent Info
						if( m_bShowStats && m_bSwapTorrentLink )
						{
                            pResponse->strContent += "<td class=\"download\">";
							if( m_strStatsLinkImage == "" )
								pResponse->strContent += "<a class=\"stats\" href=\"/stats.html?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\">Stats</a></td>";
							else
								pResponse->strContent += "<a class=\"stats\" href=\"/stats.html?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\"><img src=\"" + m_strStatsLinkImage + "\" border=0 alt=\"Stats\"></a></td>";
						}

						// <td> comments

						if( m_pAllowed && m_bAllowComments )
						{
							if( m_pComments )
								pResponse->strContent += "<td class=\"number\"><a href=\"/comments.html?info_hash=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\">" + CAtomInt( pTorrents[i].iComments ).toString( ) + "</a></td>";
						}

						// <td> added

						if( m_pAllowed && m_bShowAdded )
						{
							pResponse->strContent += "<td class=\"date\">";

							if( !pTorrents[i].strAdded.empty( ) )
							{
								// strip year and seconds from time

								pResponse->strContent += pTorrents[i].strAdded.substr( 5, pTorrents[i].strAdded.size( ) - 8 );
							}

							pResponse->strContent += "</td>";
						}

						// <td> size

						if( m_pAllowed && m_bShowSize )
							pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[i].iSize ) + "</td>";

						// <td> files

						if( m_pAllowed && m_bShowNumFiles )
							pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[i].iFiles ).toString( ) + "</td>";

						// <td> seeders

						pResponse->strContent += "<td class=\"number_";

						if( pTorrents[i].iComplete == 0 )
							pResponse->strContent += "red\">";
						else if( pTorrents[i].iComplete < 5 )
							pResponse->strContent += "yellow\">";
						else
							pResponse->strContent += "green\">";

						pResponse->strContent += CAtomInt( pTorrents[i].iComplete ).toString( ) + "</td>";

						// <td> leechers

						pResponse->strContent += "<td class=\"number_";

						if( pTorrents[i].iDL == 0 )
							pResponse->strContent += "red\">";
						else if( pTorrents[i].iDL < 5 )
							pResponse->strContent += "yellow\">";
						else
							pResponse->strContent += "green\">";

						pResponse->strContent += CAtomInt( pTorrents[i].iDL ).toString( ) + "</td>";

						// <td> completed

						if( m_bShowCompleted )
							pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[i].iCompleted ).toString( ) + "</td>";

						// <td> transferred

						if( m_pAllowed && m_bShowTransferred )
							pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[i].iTransferred ) + "</td>";

						// <td> min left

						if( m_bShowMinLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[i].iDL == 0 )
								pResponse->strContent += "N/A";
							else
							{
								if( m_pAllowed )
								{
									int iPercent = 0;

									if( pTorrents[i].iSize > 0 )
									{
										if( m_bShowLeftAsProgress )
											iPercent = 100 - (int)( ( (float)pTorrents[i].iMaxiLeft / pTorrents[i].iSize ) * 100 );
										else
											iPercent = (int)( ( (float)pTorrents[i].iMinLeft / pTorrents[i].iSize ) * 100 );
									}

									pResponse->strContent += CAtomInt( iPercent ).toString( ) + "%</td>";
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[i].iMinLeft );
							}
						}

						// <td> average left

						if( m_bShowAverageLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[i].iDL == 0 )
								pResponse->strContent += "N/A";
							else
							{
								if( m_pAllowed )
								{
									int iPercent;

									if( m_bShowLeftAsProgress )
										iPercent = 100 - pTorrents[i].iAverageLeftPercent;
									else
										iPercent = pTorrents[i].iAverageLeftPercent;

									pResponse->strContent += CAtomInt( iPercent ).toString( ) + "%";

									if( !m_strImageBarFill.empty( ) && !m_strImageBarTrans.empty( ) )
									{
										pResponse->strContent += "<br>";

										if( iPercent < 0 )
											iPercent = 0;
										if( iPercent > 100 )
											iPercent = 100;

										if( iPercent > 0 )
											pResponse->strContent += "<img src=\"" + m_strImageBarFill + "\" width=" + CAtomInt( iPercent ).toString( ) + " height=8>";

										if( iPercent < 100 )
											pResponse->strContent += "<img src=\"" + m_strImageBarTrans + "\" width=" + CAtomInt( 100 - iPercent ).toString( ) + " height=8>";
									}
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[i].iAverageLeft );

								pResponse->strContent += "</td>";
							}
						}

						// <td> maxi left

						if( m_bShowMaxiLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[i].iDL == 0 )
								pResponse->strContent += "N/A";
							else
							{
								if( m_pAllowed )
								{
									int iPercent = 0;

									if( pTorrents[i].iSize > 0 )
									{
										if( m_bShowLeftAsProgress )
											iPercent = 100 - (int)( ( (float)pTorrents[i].iMinLeft / pTorrents[i].iSize ) * 100 );
										else
											iPercent = (int)( ( (float)pTorrents[i].iMaxiLeft / pTorrents[i].iSize ) * 100 );
									}

									pResponse->strContent += CAtomInt( iPercent ).toString( ) + "%</td>";
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[i].iMaxiLeft );
							}
						}

						// <td> uploader

						if( m_bShowUploader )
							pResponse->strContent += "<td class=\"name\">" + UTIL_RemoveHTML( pTorrents[i].strUploader ) + "</td>";

						// <td> info link

						if( m_bAllowInfoLink )
						{
							pResponse->strContent += "<td class=\"infolink\">";

							if( !pTorrents[i].strInfoLink.empty( ) )
								pResponse->strContent += "<a href=\"" + UTIL_RemoveHTML( pTorrents[i].strInfoLink ) + "\">Link</a>";

							pResponse->strContent += "</td>";
						}

						// <td> admin

						if( user.iAccess & ACCESS_EDIT )
						{
							if( m_pAllowed )
								pResponse->strContent += "<td>[<a href=\"/index.html?del=" + UTIL_HashToString( pTorrents[i].strInfoHash ) + "\">Delete</a>]</td>";
						}

						pResponse->strContent += "</tr>\n";

						iAdded++;
					}
					else
						iSkipped++;
				}
			}

			delete [] pTorrents;

			// some finishing touches

			if( bFound )
				pResponse->strContent += "</table>\n";

			if( m_pAllowed && m_bShowNames && m_bSearch )
			{
				pResponse->strContent += "<span class=\"search_index\"><form method=\"get\" action=\"/index.html\">\n";

				if( !strSort.empty( ) )
					pResponse->strContent += "<input name=\"sort\" type=hidden value=\"" + strSort + "\">\n";

				if( !strFilter.empty( ) )
					pResponse->strContent += "<input name=\"filter\" type=hidden value=\"" + strFilter + "\">\n";

				pResponse->strContent += "Search <input name=\"search\" type=text size=40> <a href=\"/index.html\">Clear Search</a>\n";
				pResponse->strContent += "</form></span>\n";
			}

			// page numbers

			// Number of items to show left/right of the current page:
			unsigned int pageRange = 3;
			
			if( m_override_iPerPage > 0 )
			{
				pResponse->strContent += "<table class=\"pagenumbers\"><tr><td width=\"39%\"><p align=\"right\" class=\"pagenum_bottom\">";

				// iStart holds the start number of items.  Ie, 45 means hold from 45 to 45+m_override_iPerPage
				// m_override_iPerPage holds the number of items on a page.


/*				// Lets just temporarily store this stuff, so we're not constantly regenerating it...
				string tmpHTML;

				if( !strSort.empty( ) )
					tmpHTML += "&sort=" + strSort;

				if( !strSearch.empty( ) )
					tmpHTML += "&search=" + strSearchResp;

				if( !strFilter.empty( ) )
					tmpHTML += "&filter=" + UTIL_StringToEscaped( strFilter );
*/				
				
				// Add a "go to start" link if the current page is more than pageRange pages away...
				if( iStart > m_override_iPerPage * m_iPageRange) {
					pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=0";
					
					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

					if( !strPerPage.empty( ) )
						pResponse->strContent += "&per_page=" + strPerPage;

					pResponse->strContent += "\" title=\"Go to first page\">&laquo;</a></span>&nbsp;";
				}
				
				// Adds a previous link iff we are not at the beginning
				if( iStart != 0 )
				{
					pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=" + CAtomInt( (iStart / m_override_iPerPage) - 1 ).toString( );
					
					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

					if( !strPerPage.empty( ) )
						pResponse->strContent += "&per_page=" + strPerPage;

					pResponse->strContent += "\" title=\"Previous page\">&lt;</a></span>&nbsp;";
				}
				
				for( unsigned int i = ( (int) iStart - (int) (m_iPageRange*m_override_iPerPage) < 0 ? 0 : iStart - (m_iPageRange*m_override_iPerPage)); i < iStart; i += m_override_iPerPage )
				{
					pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=" + CAtomInt( i / m_override_iPerPage ).toString( );

					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

					if( !strPerPage.empty( ) )
						pResponse->strContent += "&per_page=" + strPerPage;

					pResponse->strContent += "\">";
					
					pResponse->strContent += CAtomInt( ( i / m_override_iPerPage ) + 1 ).toString( );

					pResponse->strContent += "</a></span>&nbsp;";
				}
				
				// Print out the current page, without linkage of any kind...
				pResponse->strContent += "<span class=\"pagecurrent\">";
				pResponse->strContent += CAtomInt( ( iStart / m_override_iPerPage ) + 1 ).toString( );
				pResponse->strContent += "</span>&nbsp;";

				if( iStart != iFound )
				{
					for( unsigned int i = iStart + m_override_iPerPage ; i < (iStart + ( m_override_iPerPage * (m_iPageRange+1) ) > iFound ? iFound : iStart + ( m_override_iPerPage * (m_iPageRange+1) ) ); i += m_override_iPerPage )
					{
						pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=" + CAtomInt( i / m_override_iPerPage ).toString( );
	
						if( !strSort.empty( ) )
							pResponse->strContent += "&sort=" + strSort;
	
						if( !strSearch.empty( ) )
							pResponse->strContent += "&search=" + strSearchResp;
	
						if( !strFilter.empty( ) )
							pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );
	
						if( !strPerPage.empty( ) )
							pResponse->strContent += "&per_page=" + strPerPage;
	
						pResponse->strContent += "\">";
						
						pResponse->strContent += CAtomInt( ( i / m_override_iPerPage ) + 1 ).toString( );
	
						pResponse->strContent += "</a></span>&nbsp;";
					}
				}
				
				// Adds a next link iff we are not at the beginning
				if( iStart + m_override_iPerPage < iFound )
				{
					pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=" + CAtomInt( (iStart / m_override_iPerPage) + 1 ).toString( );
					
					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

					if( !strPerPage.empty( ) )
						pResponse->strContent += "&per_page=" + strPerPage;


					pResponse->strContent += "\" title=\"Next page\">&gt;</a></span>&nbsp;";
				}
				
				// Add a "go to end" link if the current page is more than pageRange pages away...
				if( iStart + ( m_override_iPerPage * (m_iPageRange+1) ) < iFound ) {
					pResponse->strContent += "<span class=\"pages\"><a href=\"/index.html?page=" + CAtomInt( (iFound / m_override_iPerPage) ).toString( );
					
					if( !strSort.empty( ) )
						pResponse->strContent += "&sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&filter=" + UTIL_StringToEscaped( strFilter );

					if( !strPerPage.empty( ) )
						pResponse->strContent += "&per_page=" + strPerPage;

					pResponse->strContent += "\" title=\"Go to last page\">&raquo;</a></span>";
				}

				pResponse->strContent += "</p></td>\n";
				pResponse->strContent += "</tr></table>";
			}
		}

		pResponse->strContent += m_strStaticFooter;

		// don't even think about removing this :)

		pResponse->strContent += "<p align=\"center\">POWERED BY " + string( BNBT_VER ) + "</p>\n";
	}
	else
	{
		pResponse->strContent += "<p class=\"denied\">You are not authorized to view this page.";

		if( user.iAccess & ACCESS_SIGNUP )
			pResponse->strContent += " Click <a href=\"/signup.html\">here</a> to sign up for an account!";

		pResponse->strContent += "</p>\n";
	}

	if( m_bGen )
		pResponse->strContent += "<p class=\"gen\">Generated in " + UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ) + " seconds.</p>\n";

	pResponse->strContent += "</body>\n";
	pResponse->strContent += "</html>\n";
}
