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
#include "config.h"
#include "link.h"
#include "md5.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

//
// CLink
//

CLink :: CLink( )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;

	m_strIP = CFG_GetString( "bnbt_tlink_connect", string( ) );
	m_strPass = CFG_GetString( "bnbt_tlink_password", string( ) );

	memset( &sin, 0, sizeof( sin ) );

	sin.sin_family = AF_INET;

	// map host name

	struct hostent *pHE;

	if( pHE = gethostbyname( m_strIP.c_str( ) ) )
		memcpy( &sin.sin_addr, pHE->h_addr, pHE->h_length );
	else if( ( sin.sin_addr.s_addr = inet_addr( m_strIP.c_str( ) ) ) == INADDR_NONE )
	{
		UTIL_LogPrint( "link error (%s) - unable to get host entry\n", getName( ).c_str( ) );

		Kill( );
	}

	if( ( sin.sin_port = htons( (u_short)CFG_GetInt( "bnbt_tlink_port", 5204 ) ) ) == 0 )
	{
		UTIL_LogPrint( "link error (%s) - invalid port %d\n", getName( ).c_str( ), CFG_GetInt( "bnbt_tlink_port", 5204 ) );

		Kill( );
	}

	m_sckLink = INVALID_SOCKET;
}

CLink :: ~CLink( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	closesocket( m_sckLink );

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "link (%s) - link broken\n", getName( ).c_str( ) );
}

void CLink :: Kill( )
{
	m_bKill = true;
}

void CLink :: Go( )
{
	if( m_bKill )
		return;

	// map protocol name to protocol number

	struct protoent *pPE;

	if( ( pPE = getprotobyname( "tcp" ) ) == 0 )
	{
		UTIL_LogPrint( "link error (%s) - unable to get tcp protocol entry (error %d)\n", getName( ).c_str( ), GetLastError( ) );

		return;
	}

	// allocate socket

	if( ( m_sckLink = socket( PF_INET, SOCK_STREAM, pPE->p_proto ) ) == INVALID_SOCKET )
	{
		UTIL_LogPrint( "link error (%s) - unable to allocate socket (error %d)\n", getName( ).c_str( ), GetLastError( ) );

		return;
	}

	// connect socket

	if( connect( m_sckLink, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "link error (%s) - unable to connect (error %d)\n", getName( ).c_str( ), GetLastError( ) );

		return;
	}

	UTIL_LogPrint( "link (%s) - link established\n", getName( ).c_str( ) );

	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_INFO )
	{
		UTIL_LogPrint( "link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	CAtom *pInfo = Decode( lmReceive.msg );

	if( pInfo && pInfo->isDicti( ) )
	{
		CAtom *pNonce = ( (CAtomDicti *)pInfo )->getItem( "nonce" );

		string strHashMe;

		if( pNonce )
			strHashMe = m_strPass + ":" + pNonce->toString( );
		else
			strHashMe = m_strPass;

		unsigned char szMD5[16];

		MD5_CTX md5;

		MD5Init( &md5 );
		MD5Update( &md5, (unsigned char *)strHashMe.c_str( ), strHashMe.size( ) );
		MD5Final( szMD5, &md5 );

		lmSend.len = 16;
		lmSend.type = LINKMSG_PASSWORD;
		lmSend.msg = string( (char *)szMD5, 16 );

		Send( lmSend );

		delete pInfo;
	}
	else
	{
		UTIL_LogPrint( "link error (%s) - bad info message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmSend.len = 0;
	lmSend.type = LINKMSG_READY;
	lmSend.msg.erase( );

	Send( lmSend );

	UTIL_LogPrint( "link (%s) - ready\n", getName( ).c_str( ) );

	while( 1 )
	{
		if( m_bKill )
			return;

		// send

		m_mtxQueued.Claim( );
		vector<struct linkmsg_t> vecTemp = m_vecQueued;
		m_vecQueued.clear( );
		m_mtxQueued.Release( );

		for( vector<struct linkmsg_t> :: iterator i = vecTemp.begin( ); i != vecTemp.end( ); i++ )
			Send( *i );

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			CAtom *pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				CAtomDicti *pParamsDicti = (CAtomDicti *)pParams;

				CAtom *pInfoHash = pParamsDicti->getItem( "info_hash" );
				CAtom *pIP = pParamsDicti->getItem( "ip" );
				CAtom *pEvent = pParamsDicti->getItem( "event" );
				CAtom *pPort = pParamsDicti->getItem( "port" );
				CAtom *pUploaded = pParamsDicti->getItem( "uploaded" );
				CAtom *pDownloaded = pParamsDicti->getItem( "downloaded" );
				CAtom *pLeft = pParamsDicti->getItem( "left" );
				CAtom *pPeerID = pParamsDicti->getItem( "peer_id" );
				CAtom *pKey = pParamsDicti->getItem( "key" );
				CAtom *pAbusive = pParamsDicti->getItem( "abuse" );
				CAtom *pIncrement = pParamsDicti->getItem( "increment" );

				struct announce_t ann;

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.iPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );
					if( pKey )
						ann.strKey = pKey->toString( );
					if( pAbusive && ((CAtomLong *)pAbusive)->getValue () == 1 )
						ann.bAbusive = true;
					if( pIncrement && ((CAtomLong *)pIncrement)->getValue () == 1)
						ann.bIncrement = true;
						

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CLink :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	while( !m_strSendBuf.empty( ) )
	{
		int s = send( m_sckLink, m_strSendBuf.c_str( ), m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( s == SOCKET_ERROR )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

			Kill( );

			return;
		}

		if( s > 0 )
			m_strSendBuf = m_strSendBuf.substr( s );
	}
}

struct linkmsg_t CLink :: Receive( bool bBlock )
{
	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	if( bBlock )
	{
		while( 1 )
		{
			char pTemp[16384];

			memset( pTemp, 0, sizeof( char ) * 16384 );

			int c = recv( m_sckLink, pTemp, 16384, 0 );

			if( c == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

				Kill( );

				return lm;
			}

			if( c > 0 )
				m_strReceiveBuf += string( pTemp, c );

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, NULL, NULL, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, NULL, NULL, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "link warning (%s) - select error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			char pTemp[16384];

			memset( pTemp, 0, sizeof( char ) * 16384 );

			int c = recv( m_sckLink, pTemp, 16384, 0 );

			if( c == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

				Kill( );

				return lm;
			}

			if( c > 0 )
				m_strReceiveBuf += string( pTemp, c );
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CLink :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	string :: size_type iDelim1 = m_strReceiveBuf.find_first_not_of( "1234567890" );

	if( iDelim1 != string :: npos )
	{
		if( iDelim1 > 0 )
		{
			lm.len = atoi( m_strReceiveBuf.substr( 0, iDelim1 ).c_str( ) );

			string :: size_type iDelim2 = m_strReceiveBuf.find_first_not_of( "1234567890", iDelim1 + 1 );

			if( iDelim2 != string :: npos )
			{
				if( iDelim2 > iDelim1 )
				{
					lm.type = atoi( m_strReceiveBuf.substr( iDelim1 + 1, iDelim2 - iDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > iDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( iDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( iDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CLink :: getName( )
{
	return m_strIP + ":" + CAtomInt( ntohs( sin.sin_port ) ).toString( );
}

void CLink :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartLink( )
{
	while( gpServer )
	{
		gpLink = new CLink( );

		gpLink->Go( );

		delete gpLink;

		gpLink = NULL;

		MILLISLEEP( 10000 );
	}
}

//
// CLinkClient
//

CLinkClient :: CLinkClient( SOCKET sckLink, struct sockaddr_in sinAddress )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;
	m_bActive = false;

	m_sckLink = sckLink;

	sin = sinAddress;

	UTIL_LogPrint( "link (%s) - link established\n", getName( ).c_str( ) );
}

CLinkClient :: ~CLinkClient( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	closesocket( m_sckLink );

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "link (%s) - link broken\n", getName( ).c_str( ) );
}

void CLinkClient :: Kill( )
{
	m_bKill = true;
}

void CLinkClient :: Go( )
{
	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	// todo -> change nonce

	string strNonce = "hello";

	CAtomDicti *pInfo = new CAtomDicti( );

	pInfo->setItem( "nonce", new CAtomString( strNonce ) );

	lmSend.len = pInfo->EncodedLength( );
	lmSend.type = LINKMSG_INFO;
	lmSend.msg = Encode( pInfo );

    Send( lmSend );

	delete pInfo;

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_PASSWORD )
	{
		UTIL_LogPrint( "link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	string strHashMe = gpLinkServer->m_strPass + ":" + strNonce;

	unsigned char szMD5[16];

	MD5_CTX md5;

	MD5Init( &md5 );
	MD5Update( &md5, (unsigned char *)strHashMe.c_str( ), strHashMe.size( ) );
	MD5Final( szMD5, &md5 );

	string strMD5 = string( (char *)szMD5, 16 );

	if( strMD5 == lmReceive.msg )
		UTIL_LogPrint( "link (%s) - password accepted\n", getName( ).c_str( ) );
	else
	{
		UTIL_LogPrint( "link error (%s) - bad password, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	while( 1 )
	{
		if( m_bKill )
			return;

		// send

		if( m_bActive )
		{
			m_mtxQueued.Claim( );
			vector<struct linkmsg_t> vecTemp = m_vecQueued;
			m_vecQueued.clear( );
			m_mtxQueued.Release( );

			for( vector<struct linkmsg_t> :: iterator i = vecTemp.begin( ); i != vecTemp.end( ); i++ )
				Send( *i );
		}

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_READY )
		{
			if( gbDebug )
				UTIL_LogPrint( "link (%s) - ready\n", getName( ).c_str( ) );

			m_bActive = true;
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			CAtom *pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				CAtomDicti *pParamsDicti = (CAtomDicti *)pParams;

				CAtom *pInfoHash = pParamsDicti->getItem( "info_hash" );
				CAtom *pIP = pParamsDicti->getItem( "ip" );
				CAtom *pEvent = pParamsDicti->getItem( "event" );
				CAtom *pPort = pParamsDicti->getItem( "port" );
				CAtom *pUploaded = pParamsDicti->getItem( "uploaded" );
				CAtom *pDownloaded = pParamsDicti->getItem( "downloaded" );
				CAtom *pLeft = pParamsDicti->getItem( "left" );
				CAtom *pPeerID = pParamsDicti->getItem( "peer_id" );

				struct announce_t ann;

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.iPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}

			gpLinkServer->Queue( lmReceive, getName( ) );
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CLinkClient :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	while( !m_strSendBuf.empty( ) )
	{
		int s = send( m_sckLink, m_strSendBuf.c_str( ), m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( s == SOCKET_ERROR )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

			Kill( );

			return;
		}

		if( s > 0 )
			m_strSendBuf = m_strSendBuf.substr( s );
	}
}

struct linkmsg_t CLinkClient :: Receive( bool bBlock )
{
	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	if( bBlock )
	{
		while( 1 )
		{
			char pTemp[16384];

			memset( pTemp, 0, sizeof( char ) * 16384 );

			int c = recv( m_sckLink, pTemp, 16384, 0 );

			if( c == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

				Kill( );

				return lm;
			}

			if( c > 0 )
				m_strReceiveBuf += string( pTemp, c );

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, NULL, NULL, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, NULL, NULL, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "link warning (%s) - select error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			char pTemp[16384];

			memset( pTemp, 0, sizeof( char ) * 16384 );

			int c = recv( m_sckLink, pTemp, 16384, 0 );

			if( c == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %d)\n", getName( ).c_str( ), GetLastError( ) );

				Kill( );

				return lm;
			}

			if( c > 0 )
				m_strReceiveBuf += string( pTemp, c );
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CLinkClient :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	string :: size_type iDelim1 = m_strReceiveBuf.find_first_not_of( "1234567890" );

	if( iDelim1 != string :: npos )
	{
		if( iDelim1 > 0 )
		{
			lm.len = atoi( m_strReceiveBuf.substr( 0, iDelim1 ).c_str( ) );

			string :: size_type iDelim2 = m_strReceiveBuf.find_first_not_of( "1234567890", iDelim1 + 1 );

			if( iDelim2 != string :: npos )
			{
				if( iDelim2 > iDelim1 )
				{
					lm.type = atoi( m_strReceiveBuf.substr( iDelim1 + 1, iDelim2 - iDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > iDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( iDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( iDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CLinkClient :: getName( )
{
	return string( inet_ntoa( sin.sin_addr ) ) + ":" + CAtomInt( ntohs( sin.sin_port ) ).toString( );
}

void CLinkClient :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartLinkClient( CLinkClient *pLinkClient )
{
	if( pLinkClient )
	{
		pLinkClient->Go( );

		gpLinkServer->m_mtxLinks.Claim( );

		for( vector<CLinkClient *> :: iterator i = gpLinkServer->m_vecLinks.begin( ); i != gpLinkServer->m_vecLinks.end( ); i++ )
		{
			if( *i == pLinkClient )
			{
				delete *i;

				gpLinkServer->m_vecLinks.erase( i );

				break;
			}
		}

		gpLinkServer->m_mtxLinks.Release( );
	}
}

//
// CLinkServer
//

CLinkServer :: CLinkServer( )
{
	m_mtxLinks.Initialize( );

	m_strBind = CFG_GetString( "bnbt_tlink_bind", string( ) );
	m_strPass = CFG_GetString( "bnbt_tlink_password", string( ) );

	struct sockaddr_in sin;

	memset( &sin, 0, sizeof( sin ) );

	sin.sin_family = AF_INET;

	if( !m_strBind.empty( ) )
	{
		// bind to m_strBind

		if( gbDebug )
			UTIL_LogPrint( "link server - binding to %s\n", m_strBind.c_str( ) );

		if( ( sin.sin_addr.s_addr = inet_addr( m_strBind.c_str( ) ) ) == INADDR_NONE )
			UTIL_LogPrint( "link server error - unable to bind to %s\n", m_strBind.c_str( ) );
	}
	else
	{
		// bind to all available addresses

		if( gbDebug )
			UTIL_LogPrint( "link server - binding to all available addresses\n" );

		sin.sin_addr.s_addr = INADDR_ANY;
	}

	if( ( sin.sin_port = htons( (u_short)CFG_GetInt( "bnbt_tlink_port", 5204 ) ) ) == 0 )
		UTIL_LogPrint( "link server error - invalid port %d\n", CFG_GetInt( "bnbt_tlink_port", 5204 ) );

	// map protocol name to protocol number

	struct protoent *pPE;

	if( ( pPE = getprotobyname( "tcp" ) ) == 0 )
		UTIL_LogPrint( "link server error - unable to get tcp protocol entry (error %d)\n", GetLastError( ) );

	// allocate socket

	if( ( m_sckLinkServer = socket( PF_INET, SOCK_STREAM, pPE->p_proto ) ) == INVALID_SOCKET )
		UTIL_LogPrint( "link server error - unable to allocate socket (error %d)\n", GetLastError( ) );

	// bind socket

	int optval = 1;

#ifdef WIN32
	setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof( int ) );
#else
	setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof( int ) );
#endif

	if( bind( m_sckLinkServer, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link server error - unable to bind socket (error %d)\n", GetLastError( ) );

	// listen, queue length 1

	if( listen( m_sckLinkServer, 1 ) == SOCKET_ERROR )
		UTIL_LogPrint( "link server error - unable to listen (error %d)\n", GetLastError( ) );

	UTIL_LogPrint( "link server - start\n" );
}

CLinkServer :: ~CLinkServer( )
{
	closesocket( m_sckLinkServer );

	for( vector<CLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
		(*i)->Kill( );

	unsigned long iStart = GetTime( );

	while( 1 )
	{
		if( !m_vecLinks.empty( ) )
		{
			UTIL_LogPrint( "link server - waiting for %d links to disconnect\n", m_vecLinks.size( ) );

			MILLISLEEP( 1000 );
		}
		else
			break;

		if( GetTime( ) - iStart > 60 )
		{
			UTIL_LogPrint( "link server - waited 60 seconds, exiting anyway\n" );

			break;
		}
	}

	m_mtxLinks.Destroy( );

	UTIL_LogPrint( "link server - exit\n" );
}

void CLinkServer :: Update( )
{
	fd_set fdLinkServer;

	FD_ZERO( &fdLinkServer );
	FD_SET( m_sckLinkServer, &fdLinkServer );

	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

#ifdef WIN32
	if( select( 1, &fdLinkServer, NULL, NULL, &tv ) == SOCKET_ERROR )
#else
	if( select( m_sckLinkServer + 1, &fdLinkServer, NULL, NULL, &tv ) == SOCKET_ERROR )
#endif
	{
		UTIL_LogPrint( "link server error - select error (error %d)\n", GetLastError( ) );

		FD_ZERO( &fdLinkServer );

		MILLISLEEP( 100 );
	}

	if( FD_ISSET( m_sckLinkServer, &fdLinkServer ) )
	{
		struct sockaddr_in adrFrom;

		int iAddrLen = sizeof( adrFrom );

		SOCKET sckLinkClient;

#ifdef WIN32
		if( ( sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, &iAddrLen ) ) == INVALID_SOCKET )
#else
		if( ( sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, (socklen_t *)&iAddrLen ) ) == INVALID_SOCKET )
#endif
			UTIL_LogPrint( "link server error - accept error (error %d)\n", GetLastError( ) );
		else
		{
			CLinkClient *pLinkClient = new CLinkClient( sckLinkClient, adrFrom );

#ifdef WIN32
			if( _beginthread( ( void (*)(void *) )StartLinkClient, 0, (void *)pLinkClient ) == -1 )
				UTIL_LogPrint( "error - unable to spawn link client thread\n" );
#else
			pthread_t thread;

			// set detached state since we don't need to join with any threads

			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

			int c = pthread_create( &thread, &attr, ( void * (*)(void *) )StartLinkClient, (void *)pLinkClient );

			if( c != 0 )
				UTIL_LogPrint( "error - unable to spawn link thread (error %s)\n", strerror( c ) );
#endif

			m_mtxLinks.Claim( );
			m_vecLinks.push_back( pLinkClient );
			m_mtxLinks.Release( );
		}
	}
}

void CLinkServer :: Queue( struct linkmsg_t lm )
{
	m_mtxLinks.Claim( );

	for( vector<CLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
		(*i)->Queue( lm );

	m_mtxLinks.Release( );
}

void CLinkServer :: Queue( struct linkmsg_t lm, string strExclude )
{
	m_mtxLinks.Claim( );

	for( vector<CLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
	{
		if( (*i)->getName( ) != strExclude )
			(*i)->Queue( lm );
	}

	m_mtxLinks.Release( );
}
