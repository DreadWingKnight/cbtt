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
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseScrape( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	if( !m_bAllowScrape )
	{
		pResponse->strCode = "403 Forbidden";

		return;
	}

	typedef multimap<string,string> stringmap;
	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", "text/plain" ) );

	// retrieve info hash (note that the client sends an actual hash so don't convert it)

	//	Commented - MultiMapping InfoHash paramaters instead
	//	string strInfoHash = pRequest->multiParams["info_hash"];
	//  Major rehaul in the scrape function to allow for multi-scrape

	CAtomDicti *pScrape = new CAtomDicti( );
	CAtomDicti *pFiles = new CAtomDicti( );
	CAtomDicti *pFlags = new CAtomDicti( );

	pScrape->setItem( "files", pFiles );
	pScrape->setItem( "flags", pFlags );

	pFlags->setItem( "min_request_interval", new CAtomLong( m_iMinRequestInterval ) );

	if ( !pRequest->hasQuery )
	{
		// disable global scraping
		if( !m_bAllowGlobalScrape )
		{
			pResponse->strCode = "403 Forbidden";
			return;
		}
#ifdef BNBT_MYSQL
   if( m_bMySQLOverrideDState && m_iMySQLRefreshStatsInterval > 0 )
   {
      // Modified by =Xotic=
      string strQuery;
      strQuery = "SELECT bseeders,bleechers,bcompleted,bhash FROM torrents";

      CMySQLQuery *pQuery = new CMySQLQuery( strQuery );

      vector<string> vecQuery;
         // full
      while( ( vecQuery = pQuery->nextRow( ) ).size( ) == 4 )
      {
            CAtomDicti *pHuh = new CAtomDicti( );

            pHuh->setItem( "complete", new CAtomInt( atoi( vecQuery[0].c_str( ) ) ) );
            pHuh->setItem( "incomplete", new CAtomInt( atoi( vecQuery[1].c_str( ) ) ) );
            pHuh->setItem( "downloaded", new CAtomInt( atoi( vecQuery[2].c_str( ) ) ) );

            if( m_pAllowed )
            {
               CAtom *pList = m_pAllowed->getItem( vecQuery[3] );

               if( pList && dynamic_cast<CAtomList *>( pList ) )
               {
                  vector<CAtom *> vecTorrent = dynamic_cast<CAtomList *>( pList )->getValue( );

                  if( vecTorrent.size( ) == 6 )
                  {
                     CAtom *pName = vecTorrent[1];

                     if( pName )
                        pHuh->setItem( "name", new CAtomString( pName->toString( ) ) );
                  }
               }
            }

            pFiles->setItem( vecQuery[3], pHuh );
         }
		delete pQuery;
		pResponse->strContent = Encode ( pScrape );
		delete pScrape;
		return;
   }
#endif
   	if( m_pDFile )
	{
		map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

		for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
		{
			if( (*i).second->isDicti( ) )
			{
				CAtomDicti *pHuh = new CAtomDicti( );

				map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)(*i).second )->getValuePtr( );

				unsigned long iSeeders = 0;
				unsigned long iLeechers = 0;

				for( map<string, CAtom *> :: iterator j = pmapPeersDicti->begin( ); j != pmapPeersDicti->end( ); j++ )
				{
					if( (*j).second->isDicti( ) )
					{
						CAtom *pLeft = ( (CAtomDicti *)(*j).second )->getItem( "left" );

						if( pLeft && pLeft->isLong( ) )
						{
							if( ( (CAtomLong *)pLeft )->getValue( ) == 0 )
								iSeeders++;
							else
								iLeechers++;
						}
					}
				}

				pHuh->setItem( "complete", new CAtomInt( iSeeders ) );
				pHuh->setItem( "incomplete", new CAtomInt( iLeechers ) );

				if( m_pAllowed )
				{
					CAtom *pList = m_pAllowed->getItem( (*i).first );

					if( pList && pList->isList( ) )
					{
						vector<CAtom *> vecTorrent = ( (CAtomList *)pList )->getValue( );

						if( vecTorrent.size( ) == 6 )
						{
							CAtom *pName = vecTorrent[1];

							if( pName )
								pHuh->setItem( "name", new CAtomString( pName->toString( ) ) );
						}
					}
				}

				if( m_pCompleted )
				{
					CAtom *pCompleted = m_pCompleted->getItem( (*i).first );

					if( pCompleted && pCompleted->isLong( ) )
						pHuh->setItem( "downloaded", new CAtomLong( ( (CAtomLong *)pCompleted )->getValue( ) ) );
				}

				pFiles->setItem( (*i).first, pHuh );
				}
			}
		}
		pResponse->strContent = Encode ( pScrape );
		delete pScrape;
		return;
	}
	else
	{
		for(stringmap::iterator pos = pRequest->multiParams.lower_bound("info_hash"); pos != pRequest->multiParams.upper_bound("info_hash"); pos++)
		{
#ifdef BNBT_MYSQL
	       if( m_bMySQLOverrideDState && m_iMySQLRefreshStatsInterval > 0 )
			{
				string strQuery;
				strQuery = "SELECT bseeders,bleechers,bcompleted FROM torrents WHERE bhash=\'" + UTIL_StringToMySQL( (*pos).second ) + "\'";

				CMySQLQuery *pQuery = new CMySQLQuery( strQuery );
				vector<string> vecQuery;
         // single
         while( ( vecQuery = pQuery->nextRow( ) ).size( ) == 3 )
         {
            CAtomDicti *pHuh = new CAtomDicti( );

            pHuh->setItem( "complete", new CAtomInt( atoi( vecQuery[0].c_str( ) ) ) );
            pHuh->setItem( "incomplete", new CAtomInt( atoi( vecQuery[1].c_str( ) ) ) );
            pHuh->setItem( "downloaded", new CAtomInt( atoi( vecQuery[2].c_str( ) ) ) );

            if( m_pAllowed )
            {
               CAtom *pList = m_pAllowed->getItem( (*pos).second );

               if( pList && dynamic_cast<CAtomList *>( pList ) )
               {
                  vector<CAtom *> vecTorrent = dynamic_cast<CAtomList *>( pList )->getValue( );

                  if( vecTorrent.size( ) == 6 )
                  {
                     CAtom *pName = vecTorrent[1];

                     if( pName )
                        pHuh->setItem( "name", new CAtomString( pName->toString( ) ) );
                  }
               }
            }

            pFiles->setItem( (*pos).second , pHuh );
         }
		      delete pQuery;
		 }
		   else
		   {
#endif
			CAtom *pPeers = m_pDFile->getItem( (*pos).second );

			if( pPeers && pPeers->isDicti( ) )
			{
				CAtomDicti *pHuh = new CAtomDicti( );

				map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)pPeers )->getValuePtr( );

				unsigned long iSeeders = 0;
				unsigned long iLeechers = 0;

				for( map<string, CAtom *> :: iterator j = pmapPeersDicti->begin( ); j != pmapPeersDicti->end( ); j++ )
				{
					if( (*j).second->isDicti( ) )
					{
						CAtom *pLeft = ( (CAtomDicti *)(*j).second )->getItem( "left" );

						if( pLeft && pLeft->isLong( ) )
						{
							if( ( (CAtomLong *)pLeft )->getValue( ) == 0 )
								iSeeders++;
							else
								iLeechers++;
						}
					}
				}

				pHuh->setItem( "complete", new CAtomInt( iSeeders ) );
				pHuh->setItem( "incomplete", new CAtomInt( iLeechers ) );

				if( m_pAllowed )
				{
					CAtom *pList = m_pAllowed->getItem( (*pos).second );

					if( pList && pList->isList( ) )
					{
						vector<CAtom *> vecTorrent = ( (CAtomList *)pList )->getValue( );

						if( vecTorrent.size( ) == 6 )
						{
							CAtom *pName = vecTorrent[1];

							if( pName )
								pHuh->setItem( "name", new CAtomString( pName->toString( ) ) );
						}
					}
				}

				if( m_pCompleted )
				{
					CAtom *pCompleted = m_pCompleted->getItem( (*pos).second );

					if( pCompleted && pCompleted->isLong( ) )
						pHuh->setItem( "downloaded", new CAtomLong( ( (CAtomLong *)pCompleted )->getValue( ) ) );
				}

				pFiles->setItem( (*pos).second, pHuh );
			}

		}

#ifdef BNBT_MYSQL
}
#endif

	pResponse->strContent = Encode( pScrape );
	delete pScrape;

	}

}
