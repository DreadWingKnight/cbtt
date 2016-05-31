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
#include "link.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseAnnounce( struct request_t *pRequest, struct response_t *pResponse, user_t user )
{
	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", "text/plain" ) );

	// retrieve info hash

	string strInfoHash = pRequest->mapParams["info_hash"];

	if( strInfoHash.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( "info hash missing" );
		pResponse->bCompressOK = false;

		return;
	}

	if( m_pAllowed )
	{
		if( !m_pAllowed->getItem( strInfoHash ) )
		{
			pResponse->strContent = UTIL_FailureReason( "requested download is not authorized for use with this tracker" );
			pResponse->bCompressOK = false;

			return;
		}
	}

	// retrieve ip

	string strIP = inet_ntoa( pRequest->sin.sin_addr );
	string strTempIP = pRequest->mapParams["ip"];
	string strIPConv = strIP.c_str( );

	// Verify that the IP is permitted to access the tracker
	if( ( isIPBanList( strIP.c_str( ) ) == 1 || isIPBanList( strTempIP ) == 1 ) && m_iIPBanMode == 1 )
	{
		pResponse->strContent = UTIL_FailureReason( "Your IP has been banned from this tracker." );
		pResponse->bCompressOK = false;
		return;
	}
	else if( ( isIPBanList( strIP.c_str( ) ) == 0 && isIPBanList( strTempIP ) == 0 ) && m_iIPBanMode == 2 )
	{
		pResponse->strContent = UTIL_FailureReason( "Your IP has not been cleared for use on this tracker." );
		pResponse->bCompressOK = false;
		return;
	}

	CAtomDicti *pData = new CAtomDicti( );

	if( m_iBlockNATedIP == 1 && ( !strTempIP.empty( ) && ( strTempIP.substr(0,8) == "192.168." || strTempIP.substr(0,8) == "169.254." || strTempIP.substr(0,3) == "10." || strTempIP.substr(0,7) == "172.16." || strTempIP.substr(0,7) == "172.17." || strTempIP.substr(0,7) == "172.18." || strTempIP.substr(0,7) == "172.19." || strTempIP.substr(0,7) == "172.20." || strTempIP.substr(0,7) == "172.21." || strTempIP.substr(0,7) == "172.22." || strTempIP.substr(0,7) == "172.23." || strTempIP.substr(0,7) == "172.24." || strTempIP.substr(0,7) == "172.25." || strTempIP.substr(0,7) == "172.26." || strTempIP.substr(0,7) == "172.27." || strTempIP.substr(0,7) == "172.28." || strTempIP.substr(0,7) == "172.29." || strTempIP.substr(0,7) == "172.30." || strTempIP.substr(0,7) == "172.31." || strTempIP.substr(0,7) == "172.32." || strTempIP == "127.0.0.1" ) ) )
	{
		strTempIP = "";
		pData->setItem("warning message", new CAtomString( "A122: This tracker does not permit you to specify your IP to it. Using the IP you are connecting from instead." ) );
	}

	if( m_iLocalOnly == 1 && ( !strIPConv.empty( ) && ( strIPConv.substr(0,8) == "192.168." || strIPConv.substr(0,8) == "169.254." || strIPConv.substr(0,3) == "10." || strIPConv.substr(0,7) == "172.16." || strIPConv.substr(0,7) == "172.17." || strIPConv.substr(0,7) == "172.18." || strIPConv.substr(0,7) == "172.19." || strIPConv.substr(0,7) == "172.20." || strIPConv.substr(0,7) == "172.21." || strIPConv.substr(0,7) == "172.22." || strIPConv.substr(0,7) == "172.23." || strIPConv.substr(0,7) == "172.24." || strIPConv.substr(0,7) == "172.25." || strIPConv.substr(0,7) == "172.26." || strIPConv.substr(0,7) == "172.27." || strIPConv.substr(0,7) == "172.28." || strIPConv.substr(0,7) == "172.29." || strIPConv.substr(0,7) == "172.30." || strIPConv.substr(0,7) == "172.31." || strIPConv.substr(0,7) == "172.32." || strIPConv == "127.0.0.1" ) ) )
	{
		if( !strTempIP.empty( ) && strTempIP.find_first_not_of( "1234567890." ) == string :: npos )
			strIP = strTempIP;
		else
		{
			struct hostent *he;
			struct in_addr **addr_list;
			if( ( he = gethostbyname( strTempIP.c_str() ) ) != NULL )
			{
				addr_list = (struct in_addr **)he->h_addr_list;
				strIP = inet_ntoa(*addr_list[0]);
			}
			//	string strParseIP = inet_ntoa( gethostbyname( strTempIP.c_str( ) ). );
            //    pData->setItem("warning message", new CAtomString( "A473: The IP you have specified is invalid. Using the IP you are connecting from instead." ) );
			//strTempIP = strParseIP;
			//strTempIP = "";
		}

	}

	else if( m_iLocalOnly == 1 && ( !strIPConv.empty( ) && !( strIPConv.substr(0,8) == "192.168." || strIPConv.substr(0,8) == "169.254." || strIPConv.substr(0,3) == "10." || strIPConv.substr(0,7) == "172.16." || strIPConv.substr(0,7) == "172.17." || strIPConv.substr(0,7) == "172.18." || strIPConv.substr(0,7) == "172.19." || strIPConv.substr(0,7) == "172.20." || strIPConv.substr(0,7) == "172.21." || strIPConv.substr(0,7) == "172.22." || strIPConv.substr(0,7) == "172.23." || strIPConv.substr(0,7) == "172.24." || strIPConv.substr(0,7) == "172.25." || strIPConv.substr(0,7) == "172.26." || strIPConv.substr(0,7) == "172.27." || strIPConv.substr(0,7) == "172.28." || strIPConv.substr(0,7) == "172.29." || strIPConv.substr(0,7) == "172.30." || strIPConv.substr(0,7) == "172.31." || strIPConv.substr(0,7) == "172.32." || strIPConv == "127.0.0.1" ) ) )
    {
		if( !strTempIP.empty( ) && strTempIP != strIPConv )
			pData->setItem("warning message", new CAtomString( "A824: This tracker does not permit you to specify your IP to it. Using the IP you are connecting from instead." ) );
		strTempIP = "";
	}

	else if( m_iLocalOnly == 0 )
	{
		if( !strTempIP.empty( ) && strTempIP.find_first_not_of( "1234567890." ) == string :: npos )
			strIP = strTempIP;
		else
		{
			struct hostent *he;
			struct in_addr **addr_list;
			if( ( he = gethostbyname( strTempIP.c_str() ) ) != NULL )
			{
				addr_list = (struct in_addr **)he->h_addr_list;
				strIP = inet_ntoa(*addr_list[0]);
			}
			//	string strParseIP = inet_ntoa( gethostbyname( strTempIP.c_str( ) ). );
            //    pData->setItem("warning message", new CAtomString( "A473: The IP you have specified is invalid. Using the IP you are connecting from instead." ) );
			//strTempIP = strParseIP;
			//strTempIP = "";
		}
	}

	// retrieve a ton of parameters

	string strEvent = pRequest->mapParams["event"];
	string strPort = pRequest->mapParams["port"];
	string strUploaded = pRequest->mapParams["uploaded"];
	string strDownloaded = pRequest->mapParams["downloaded"];
	string strLeft = pRequest->mapParams["left"];
	string strPeerID = pRequest->mapParams["peer_id"];
	string strNumWant = pRequest->mapParams["numwant"];
	string strNoPeerID = pRequest->mapParams["no_peer_id"];
	string strCompact = pRequest->mapParams["compact"];
	string strKey = pRequest->mapParams["key"];

	if( m_bAnnounceKeySupport && strKey.empty( ) )
	{
		if( m_bRequireKeySupport )
		{
			pResponse->strCode = "200 OK";
			pResponse->strContent = UTIL_FailureReason( "This tracker requires support for the \"key\" announce paramater. Please update or change your client." );
			return;
		}
		else
			pData->setItem("warning message", new CAtomString( "A290: This tracker supports key passing. Please enable your client." ) ); 
	}

	// Add support for MySQL DState Override checking here.
    if( m_pDFile )
	{
		if( !m_pDFile->getItem( strInfoHash ) )
			m_pDFile->setItem( strInfoHash, new CAtomDicti( ) );

		CAtom *pPeers = m_pDFile->getItem( strInfoHash );
		if( pPeers && pPeers->isDicti( ) )
		{
			CAtom *pPeer = ( (CAtomDicti *)pPeers )->getItem( strPeerID );

			if( pPeer && pPeer->isDicti( ) )
			{
					CAtom *pKey = ((CAtomDicti *)pPeer)->getItem( "key" , NULL );
					if( pKey && ((CAtomString *)pKey)->toString() != strKey.c_str( ) )
					{
						pResponse->strCode = "200 OK";
						pResponse->strContent = UTIL_FailureReason( "Your client's \"key\" paramater and the key we have for you in our database do not match." );
						return;
					}
			}
		}
	}


	// Check no_peer_id/compact requirement
	// Do things sanely by ignoring requests with numwant=0
	// This way we don't accidentally reject clients that don't want a peerlist
	// numwant=0 and compact=1 are mutually exclusive
	// numwant=0 and no_peer_id=1 are mutually exclusive
	if( strNumWant != "0" )
	{
		// Check compact Requirement
		if( m_bRequireCompact == true && strCompact != "1" )
		{
			pResponse->strCode = "200 OK";
			pResponse->strContent = UTIL_FailureReason( "This tracker requires support for compact tracker requests. Please update or change your client." );
			return;
		}

		// Check no_peer_id Requirement - If Compact is supported, no_peer_id isn't required.
		// both no_peer_id and compact need to be unsupported for a tracker requiring no_peer_id to reject a client.
		if( m_bRequireNoPeerID == true && strNoPeerID != "1" && strCompact != "1" )
		{
			pResponse->strCode = "200 OK";
			pResponse->strContent = UTIL_FailureReason( "This tracker requires support for no_peer_id or compact tracker requests. Please update or change your client." );
			return;
		}

	}

	string strUserAgent = pRequest->mapHeaders["User-Agent"];
    string struAgent = pRequest->mapHeaders["User-agent"];
	if( strUserAgent.empty() )
		strUserAgent = "Unknown";

	if( struAgent.empty() )
		struAgent = "Unknown";

	if( i_intPeerSpoofRestrict == 1 )
		if( ( "-AZ" != strPeerID.substr(0,3) && "Azureus" == strUserAgent.substr(0,7) ) || ( "BitTorrent/3." == strUserAgent.substr(0,13) && ( "S5" == strPeerID.substr(0,2) || UTIL_EscapedToString("S%05") == strPeerID.substr(0,2) ) ) )
		{
			pResponse->strCode = "200 OK";
			pResponse->strContent = UTIL_FailureReason( "This tracker does not allow client spoofing. Please disable client spoofing in your client configuration." );
			return;
		}

	int isbanned = isClientBanList( strUserAgent , true );

	if( isbanned == 0 )
		isbanned = isClientBanList( struAgent , true );
	if( isbanned == 0 )
		isbanned = isClientBanList( UTIL_EscapedToString( strPeerID ) );

	if (isbanned == 1 && i_intBanMode == 1)
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Client Version is banned. Check with the tracker administrator" );
//		UTIL_LogPrint("Banned client denied access\n");
		return;
	}

	if (isbanned == 0 && i_intBanMode == 2)
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Client Version is banned. Check with the tracker administrator" );
//		UTIL_LogPrint("Non-Friendly Client denied access\n");
		return;
	}


	if( !strEvent.empty( ) && strEvent != "started" && strEvent != "completed" && strEvent != "stopped" )
	{
		pResponse->strContent = UTIL_FailureReason( "invalid event" );
		pResponse->bCompressOK = false;

		return;
	}

	if( strPort.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( "port missing" );
		pResponse->bCompressOK = false;

		return;
	}

	if( strUploaded.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( "uploaded missing" );
		pResponse->bCompressOK = false;

		return;
	}

	if( strDownloaded.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( "downloaded missing" );
		pResponse->bCompressOK = false;

		return;
	}

	if( strLeft.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( "left missing" );
		pResponse->bCompressOK = false;

		return;
	}

	if( strPeerID.size( ) != 20 )
	{
		pResponse->strContent = UTIL_FailureReason( "peer id not of length 20" );
		pResponse->bCompressOK = false;

		return;
	}

	struct announce_t ann;

	ann.strInfoHash = strInfoHash;
	ann.strIP = strIP;
	ann.strEvent = strEvent;
	ann.iPort = (unsigned int)atoi( strPort.c_str( ) );
	ann.iUploaded = UTIL_StringTo64( strUploaded.c_str( ) );
	ann.iDownloaded = UTIL_StringTo64( strDownloaded.c_str( ) );
	ann.iLeft = UTIL_StringTo64( strLeft.c_str( ) );
	ann.strPeerID = strPeerID;
	ann.strKey = strKey;
	ann.bAbusive = false;


	if( ( ann.iPort <= 1024 ) && m_iBlackListServicePorts )
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Listen port " + strPort + " is blacklisted from this server, please adjust your settings to use a port above 1024." );
		return;
	}

	if( ( ( ann.iPort >= 6881 && ann.iPort <= 6999 ) || ( ann.iPort >= 411 && ann.iPort <= 413 ) || ann.iPort == 1214 || ann.iPort == 4662 || ( ann.iPort >= 6346 && ann.iPort <= 6347 ) ) && m_iBlacklistP2PPorts )
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Listen port " + strPort + " is blacklisted from this server, please adjust your settings to use a port outside of common p2p ranges." );
		return;
	}

	int64 iOverFlowLimit = UTIL_StringTo64( strOverFlowLimit.c_str() );
	int64 iOverFlowminimum = UTIL_StringTo64( "107374182400" );
	if( iOverFlowLimit < iOverFlowminimum )
		iOverFlowLimit = iOverFlowminimum;

	if( ann.iDownloaded < 0 || ( m_iRestrictOverflow == 1 && ann.iDownloaded >= iOverFlowLimit ) )
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Amount reported downloaded is invaild" );
		//UTIL_LogPrint("Invalid downloaded value reported, client rejected\n");
		return;
	}

	if( ann.iUploaded < 0 || ( m_iRestrictOverflow == 1 && ann.iUploaded >= iOverFlowLimit ) )
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Amount reported uploaded is invaild" );
		//UTIL_LogPrint("Invalid downloaded value reported, client rejected\n");
		return;
	}

	if( ann.iLeft < 0 )
	{
		pResponse->strCode = "200 OK";
		pResponse->strContent = UTIL_FailureReason( "Amount reported remaining is invalid" );
		return;
	}

	if( m_bEnableAbuseBlock == true && m_pAbuse )
	{
		if( m_pAbuse->getItem( strIP ) )
		{
			CAtom *pPeerIP = m_pAbuse->getItem( strIP );
			if( pPeerIP && pPeerIP->isDicti( ) )
			{
                CAtom *pPeerLastAnnounce = ((CAtomDicti *)pPeerIP)->getItem( "lastannounce", new CAtomLong ( 0 ) );
				CAtom *pPeerAbuses = ((CAtomDicti *)pPeerIP)->getItem( "totalabuses" , new CAtomLong ( 0 ) );
				//CAtom *pPeerAbuseByHash = ((CAtomDicti *)pPeerIP)->getItem( "abusesbyhash" );
				long m_lPeerAbuses = ((CAtomLong *)pPeerAbuses)->getValue( );
				if( gbDebug )
					UTIL_LogPrint( " Checking Abuse Rating \n" );
				bool m_bHammer;
				m_bHammer = false;
				if( gbDebug ){
					UTIL_LogPrint( "Current Time is %i and last announce from this peer was %i\n",GetRealTime(), ((CAtomLong *)pPeerLastAnnounce)->getValue( ) );
					UTIL_LogPrint( "The Difference is %i\n",GetRealTime() - ((CAtomLong *)pPeerLastAnnounce)->getValue( ) );
				}
				if( ann.strEvent == "completed" || ann.strEvent == "stopped" )
				{
					if( gbDebug )
						UTIL_LogPrint( "Resetting Due To Event sent \n" );

					ann.bIncrement = false;
					ann.bAbusive = false;
				}
				else {
				if( ( m_lPeerAbuses >= m_iGlobalAbuseHammer ) && ( ( (m_lPeerAbuses * m_iAnnounceInterval) - 5 ) > (long)( GetRealTime( ) - ((CAtomLong *)pPeerLastAnnounce)->getValue( ) ) ) )
				{
					if( gbDebug )
						UTIL_LogPrint( " Hammering Abuse - Abuse # %i\n", m_lPeerAbuses );
                    pResponse->strCode = "200 OK";
					pResponse->strContent = UTIL_FailureReason( "Your client has been banned for repeated abuse. " + ((CAtomLong *)pPeerAbuses)->toString( ) + " Abuses so far. Please check with Tracker Administration for details." );
					ann.bAbusive = true;
					ann.bIncrement = true;
				}
				else if( m_iMinAnnounceInterval - 5 > (long)( GetRealTime( ) - ((CAtomLong *)pPeerLastAnnounce)->getValue( ) ) )
				{
					if( gbDebug )
						UTIL_LogPrint( " Standard Abuse \n" );

					strNumWant = "0";
                    if( ((CAtomLong *)pPeerAbuses)->getValue( ) >= m_iGlobalAbuseLimit )
					{
						if( gbDebug )
							UTIL_LogPrint( "Banned for %i Abuses \n", m_lPeerAbuses );
                        pResponse->strCode = "200 OK";
						pResponse->strContent = UTIL_FailureReason( "Your client has been banned for abuse. " + ((CAtomLong *)pPeerAbuses)->toString( ) + " Abuses so far. Please check with Tracker Administration for details." );
						ann.bIncrement = true;
						ann.bAbusive = true;
					}
					else 
					{
						if( ann.strEvent != "completed" || ann.strEvent != "stopped" )
						{
							if( gbDebug )
								UTIL_LogPrint( " Warned for %i Abuses \n", m_lPeerAbuses );

							pData->setItem("warning message", new CAtomString( "A58A: This tracker does not permit abusive announcing to get more peers. Your announce will contain 0 peers. Continued abuse will result in an automated ban." ) );
							ann.bIncrement = true;
						}
					}
//					((CAtomDicti *)pPeerIP)->setItem( "abusesbyhash", new CAtomLong( ((CAtomLong *)pPeerAbuses)->getValue( ) +1 ) );
/*                  if( pPeerAbuseByHash && pPeerAbuseByHash->isDicti( ))
					{
						CAtom *pCurrentHash = ((CAtomDicti *)pPeerIP)->getItem( strInfoHash );
						if( pCurrentHash && pCurrentHash->isDicti( ) )
						{
	
						}
					}
					*/
				}
				}
			}
		}

	}

	Announce( ann );

	if( ann.bAbusive == true )
		return;

	unsigned int iRSize = m_iResponseSize;

	if( !strNumWant.empty( ) )
		iRSize = atoi( strNumWant.c_str( ) );

	if( iRSize > (unsigned int)m_iMaxGive )
		iRSize = (unsigned int)m_iMaxGive;

	// link

	if( gpLinkServer || gpLink )
	{
		CAtomDicti *pAnnounce = new CAtomDicti( );

		pAnnounce->setItem( "info_hash", new CAtomString( ann.strInfoHash ) );
		pAnnounce->setItem( "ip", new CAtomString( ann.strIP ) );

		if( !strEvent.empty( ) )
			pAnnounce->setItem( "event", new CAtomString( ann.strEvent ) );

		pAnnounce->setItem( "port", new CAtomLong( ann.iPort ) );
		pAnnounce->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
		pAnnounce->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
		pAnnounce->setItem( "left", new CAtomLong( ann.iLeft ) );
		pAnnounce->setItem( "peer_id", new CAtomString( ann.strPeerID ) );
		pAnnounce->setItem( "key", new CAtomString( ann.strKey ) );
		if( ann.bAbusive == true )
			pAnnounce->setItem( "abuse", new CAtomLong( 1 ) );
		if( ann.bIncrement == true )
			pAnnounce->setItem( "increment", new CAtomLong( 1 ) );

		struct linkmsg_t lm;

		lm.len = pAnnounce->EncodedLength( );
		lm.type = LINKMSG_ANNOUNCE;
		lm.msg = Encode( pAnnounce );

		if( gpLinkServer )
			gpLinkServer->Queue( lm );
		else if ( gpLink )
			gpLink->Queue( lm );

		delete pAnnounce;
	}

	// populate cache

	if( !m_pCached->getItem( ann.strInfoHash ) )
		m_pCached->setItem( ann.strInfoHash, new CAtomList( ) );

	CAtom *pCache = m_pCached->getItem( ann.strInfoHash );

	if( pCache && pCache->isList( ) )
	{
		CAtomList *pCacheList = (CAtomList *)pCache;

		if( pCacheList->getValuePtr( )->size( ) < iRSize )
		{
#ifdef BNBT_MYSQL
			if( m_bMySQLOverrideDState )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bip,bport FROM dstate WHERE bhash=\'" + UTIL_StringToMySQL( ann.strInfoHash ) + "\'" );

				vector<string> vecQuery;

				while( ( vecQuery = pQuery->nextRow( ) ).size( ) == 3 )
				{
					CAtomDicti *pAdd = new CAtomDicti( );

					pAdd->setItem( "peer id", new CAtomString( vecQuery[0] ) );
					pAdd->setItem( "ip", new CAtomString( vecQuery[1] ) );
					pAdd->setItem( "port", new CAtomLong( atoi( vecQuery[2].c_str( ) ) ) );

					pCacheList->addItem( pAdd );
				}

				delete pQuery;
			}
			else
			{
#endif
				if( m_pDFile )
				{
					if( !m_pDFile->getItem( ann.strInfoHash ) )
						m_pDFile->setItem( ann.strInfoHash, new CAtomDicti( ) );

					CAtom *pPeers = m_pDFile->getItem( ann.strInfoHash );

					if( pPeers && pPeers->isDicti( ) )
					{
						map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)pPeers )->getValuePtr( );

						for( map<string, CAtom *> :: iterator i = pmapPeersDicti->begin( ); i != pmapPeersDicti->end( ); i++ )
						{
							if( (*i).second->isDicti( ) )
							{
								CAtom *pIP = ( (CAtomDicti *)(*i).second )->getItem( "ip" );
								CAtom *pPort = ( (CAtomDicti *)(*i).second )->getItem( "port" );
								// check for oneself and remove from the list
								if( pIP && pIP->toString( ) == strIP && pPort && pPort->toString( ) == strPort )
								{
									continue;
								}
								
								// remove seeds if announce is from a seed
								if( ann.iLeft == 0 )
								{
									CAtom *pLeft = ( (CAtomDicti *)(*i).second )->getItem( "left" );

									if( pLeft && atol( pLeft->toString( ).c_str( ) ) == 0 )
									{
										continue;
									}
								}

								// Moved here to clean up from the memory leak
								CAtomDicti *pAdd = new CAtomDicti( );

								pAdd->setItem( "peer id", new CAtomString( (*i).first ) );

								if( pIP )
									pAdd->setItem( "ip", new CAtomString( pIP->toString( ) ) );

								if( pPort && pPort->isLong( ) )
									pAdd->setItem( "port", new CAtomLong( *(CAtomLong *)pPort ) );

								pCacheList->addItem( pAdd );
							}
						}
					}
				}

#ifdef BNBT_MYSQL
			}
#endif

			pCacheList->Randomize( );
		}
	}

	// clamp response

	if( pCache && pCache->isList( ) )
	{
		CAtomList *pCacheList = (CAtomList *)pCache;

		CAtom *pPeers = NULL;

		if( strCompact == "1" )
		{
			// compact announce

			string strPeers;

			vector<CAtom *> *pvecList = pCacheList->getValuePtr( );

			for( vector<CAtom *> :: iterator i = pvecList->begin( ); i != pvecList->end( ); )
			{
				if( (*i)->isDicti( ) )
				{
					if( strPeers.size( ) / 6 >= iRSize )
						break;

					CAtom *pIP = ( (CAtomDicti *)(*i) )->getItem( "ip" );
					CAtom *pPort = ( (CAtomDicti *)(*i) )->getItem( "port" );

					if( pIP && pPort && pPort->isLong( ) )
					{
						// tphogan - this is a much more efficient version of UTIL_Compact

						bool bOK = true;
						char pCompact[6];
						char *szIP = new char[pIP->Length( ) + 1];
						char *szCur = szIP;
						char *pSplit;
						strncpy( szIP, pIP->toString( ).c_str( ), pIP->Length( ) + 1 );

						// first three octets

						for( int i = 0; i < 3; i++ )
						{
							if( (pSplit = strstr( szCur, "." )) )
							{
								*pSplit = '\0';
								pCompact[i] = (char)atoi( szCur );
								szCur = pSplit + 1;
							}
							else
							{
								bOK = false;

								break;
							}
						}

						if( bOK )
						{
							// fourth octet

							pCompact[3] = (char)atoi( szCur );

							// port

							unsigned int iPort = (unsigned int)dynamic_cast<CAtomLong *>( pPort )->getValue( );

#ifdef BNBT_BIG_ENDIAN
							pCompact[5] = (char)( ( iPort & 0xFF00 ) >> 8 );
							pCompact[4] = (char)( iPort & 0xFF );
#else
							pCompact[4] = (char)( ( iPort & 0xFF00 ) >> 8 );
							pCompact[5] = (char)( iPort & 0xFF );
#endif

							strPeers += string( pCompact, 6 );
						}

						delete[] szIP;
					}

					delete *i;

					i = pvecList->erase( i );
				}
				else
					i++;
			}

			pPeers = new CAtomString( strPeers );

			// don't compress

			pResponse->bCompressOK = false;
		}
		else
		{
			// regular announce

			CAtomList *pPeersList = new CAtomList( );

			vector<CAtom *> *pvecList = pCacheList->getValuePtr( );

			for( vector<CAtom *> :: iterator i = pvecList->begin( ); i != pvecList->end( ); )
			{
				if( (*i)->isDicti( ) )
				{
					if( pPeersList->getValuePtr( )->size( ) == iRSize )
						break;

					if( strNoPeerID == "1" )
						( (CAtomDicti *)(*i) )->delItem( "peer id" );

					pPeersList->addItem( new CAtomDicti( *(CAtomDicti *)(*i) ) );

					delete *i;

					i = pvecList->erase( i );
				}
				else
					i++;
			}

			pPeers = pPeersList;
		}

		// addition to support private tracker definitions.
		pData->setItem( "private", new CAtomLong( m_iPrivateTracker ) );
		pData->setItem( "interval", new CAtomLong( m_iAnnounceInterval ) );
		pData->setItem( "min interval", new CAtomLong( m_iMinAnnounceInterval ) );
		pData->setItem( "peers", pPeers );

		CAtom *pFC = m_pFastCache->getItem( strInfoHash );

		if( pFC && dynamic_cast<CAtomList *>( pFC ) )
		{
			vector<CAtom *> vecList = dynamic_cast<CAtomList *>( pFC )->getValue( );

			pData->setItem( "complete", new CAtomInt( *dynamic_cast<CAtomInt *>( vecList[0] ) ) );
			pData->setItem( "incomplete", new CAtomInt( *dynamic_cast<CAtomInt *>( vecList[1] ) ) );
		}

		pResponse->strContent = Encode( pData );

		delete pData;
	}
}
