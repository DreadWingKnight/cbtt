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
#include "client.h"
#include "config.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

#ifdef WIN32
 #include "util_ntservice.h"
#endif

CServer :: CServer( )
{
	m_bKill = false;

	m_iSocketTimeOut = CFG_GetInt( "socket_timeout", 15 );
	m_strBind = CFG_GetString( "bind", string( ) );
	m_iCompression = CFG_GetInt( "bnbt_compression_level", 6 );

	// clamp

	if( m_iCompression > 9 )
		m_iCompression = 9;


	// Binding socket to IP and port

	struct sockaddr_in sin;

	memset( &sin, 0, sizeof( sin ) );

	sin.sin_family = AF_INET;

	if( !m_strBind.empty( ) )
	{
		// bind to m_strBind

		if( gbDebug )
			UTIL_LogPrint( "server - binding to %s\n", m_strBind.c_str( ) );

		sin.sin_addr.s_addr = inet_addr( m_strBind.c_str( ) );

		if( sin.sin_addr.s_addr == INADDR_NONE || sin.sin_addr.s_addr == 0 )
		{
			UTIL_LogPrint( "server error - unable to bind to %s\n", m_strBind.c_str( ) );

			Kill( );
		}
	}
	else
	{
		// bind to all available addresses

		if( gbDebug )
			UTIL_LogPrint( "server - binding to all available addresses\n" );

		sin.sin_addr.s_addr = INADDR_ANY;
	}

	// Multiple Listen Ports
	// Checking for legacy configuration values (compatibility)

	if( ( sin.sin_port = htons( (u_short)CFG_GetInt("port",6969))) == 0)
		UTIL_LogPrint( "server warning - invalid port %d (\"port\"), ignoring\n",CFG_GetInt("port",6969));
	else if( !AddListener( sin ) )
		UTIL_LogPrint( "server warning - unable to add listener on port %d (\"port\")\n",CFG_GetInt("port",6969));
	else
		UTIL_LogPrint( "server - listening on port %d (\"port\")\n",CFG_GetInt("port",6969));

	
	// Multiple Listen Ports
	// Checking for new configuration values

	int iPort = 1;

	string strName = "port" + CAtomInt( iPort ).toString();
	string strPort = CFG_GetString( strName, string ());

	while( !strPort.empty() )
	{
		if( (sin.sin_port = htons( (u_short)atoi( strPort.c_str() ) ) ) == 0 )
			UTIL_LogPrint( "server warning - invalid port %d (\"%s\"), ignoring\n", atoi( strPort.c_str() ),strName.c_str() );
		else if(!AddListener( sin ) )
			UTIL_LogPrint( "server warning - unable to add listener on port %d (\"%s\")\n", atoi( strPort.c_str() ),strName.c_str() );
		else
			UTIL_LogPrint( "server - listening on port %d (\"%s\")\n", atoi( strPort.c_str() ),strName.c_str() );

		iPort++;
		strName = "port" + CAtomInt( iPort ).toString();
		strPort = CFG_GetString( strName, string ());
	}

	// verifying that we are listening on at least 1 port
	if( m_vecListeners.empty() )
	{
		UTIL_LogPrint("server error - not listening on any ports\n");
		exit(1);
	}


	/*sin.sin_port = htons( (u_short)CFG_GetInt( "port", 6969 ) );

	if( sin.sin_port < 1 || sin.sin_port > 65535 )
	{
		UTIL_LogPrint( "server error - invalid port %d\n", CFG_GetInt( "port", 6969 ) );

		Kill( );
	}*/

	m_pTracker = new CTracker( );

	UTIL_LogPrint( "server - start\n" );
}

CServer :: ~CServer( )
{
	unsigned long iStart = GetTime( );

	while( 1 )
	{
		for( vector<CClient *> :: iterator i = m_vecClients.begin( ); i != m_vecClients.end( ); )
		{
			if( (*i)->m_iState == CS_WAITING1 || (*i)->m_iState == CS_WAITING2 || (*i)->m_iState == CS_DEAD )
			{
				delete *i;

				i = m_vecClients.erase( i );
			}
			else
				i++;
		}

		if( !m_vecClients.empty( ) )
		{
			UTIL_LogPrint( "server - waiting for %d clients to disconnect\n", m_vecClients.size( ) );

			MILLISLEEP( 1000 );
		}
		else
			break;

		if( GetTime( ) - iStart > 60 )
		{
			UTIL_LogPrint( "server - waited 60 seconds, exiting anyway\n" );

			break;
		}
	}

	for( vector<SOCKET> :: iterator i = m_vecListeners.begin( ); i != m_vecListeners.end( ); i++ )
		closesocket( *i );

	for( vector<CClient *> :: iterator j = m_vecClients.begin( ); j != m_vecClients.end( ); j++ )
		delete *j;

	m_vecListeners.clear( );
	m_vecClients.clear( );

	if( m_pTracker )
		delete m_pTracker;

	m_pTracker = NULL;

	UTIL_LogPrint( "server - exit\n" );
}

void CServer :: Kill( )
{
	m_bKill = true;
}

bool CServer :: isDying( )
{
	return m_bKill;
}

bool CServer :: Update( bool bBlock )
{
	if( m_vecClients.size( ) < giMaxConns )
	{
		for( vector<SOCKET> :: iterator i = m_vecListeners.begin( ); i != m_vecListeners.end( ); i++ )
		{
		fd_set fdServer;

		FD_ZERO( &fdServer );
		FD_SET( *i, &fdServer );

		struct timeval tv;

		if( bBlock )
		{
			// block for 100 ms to keep from eating up all cpu time

			tv.tv_sec = 0;
			tv.tv_usec = 100000;
		}
		else
		{
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		}

#ifdef WIN32
		if( select( 1, &fdServer, NULL, NULL, &tv ) == SOCKET_ERROR )
#else
		if( select( *i + 1, &fdServer, NULL, NULL, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "server warning - select error (error %d)\n", GetLastError( ) );

			FD_ZERO( &fdServer );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( *i, &fdServer ) )
		{
			struct sockaddr_in adrFrom;

			int iAddrLen = sizeof( adrFrom );

			SOCKET sckClient;

#ifdef WIN32
			sckClient = accept( *i, (SOCKADDR*)&adrFrom, &iAddrLen );

			if( sckClient == INVALID_SOCKET )
#else
			sckClient = accept( *i, (struct sockaddr *)&adrFrom, (socklen_t *)&iAddrLen );

			if( sckClient == INVALID_SOCKET )
#endif
				UTIL_LogPrint( "server warning - accept error (error %d)\n", GetLastError( ) );
			else
			{
				// reuse the old timeval structure just because

				tv.tv_sec = m_iSocketTimeOut;
				tv.tv_usec = 0;

				CClient *pClient = new CClient( sckClient, adrFrom, tv, m_iCompression );

#ifdef WIN32
				if( _beginthread( ( void (*)(void *) )StartReceiving, 0, (void *)pClient ) == -1 )
				{
					UTIL_LogPrint( "server warning - unable to spawn receiver thread\n" );

					pClient->m_iState = CS_DEAD;
				}
#else
				pthread_t thread;

				// set detached state since we don't need to join with any threads

				pthread_attr_t attr;
				pthread_attr_init( &attr );
				pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

				int c = pthread_create( &thread, &attr, ( void * (*)(void *) )StartReceiving, (void *)pClient );

				if( c != 0 )
				{
					UTIL_LogPrint( "server warning - unable to spawn receiver thread (error %s)\n", strerror( c ) );

					pClient->m_iState = CS_DEAD;
				}
#endif

				m_vecClients.push_back( pClient );
			}
		}
		}
	}
	else
	{
		// maximum connections reached

		MILLISLEEP( 10 );
	}

	// process

	for( vector<CClient *> :: iterator i = m_vecClients.begin( ); i != m_vecClients.end( ); )
	{
		if( (*i)->m_iState == CS_WAITING1 )
		{
			(*i)->Process( );

			i++;
		}
		else if( (*i)->m_iState == CS_WAITING2 )
		{
			(*i)->m_iState = CS_SENDING;

#ifdef WIN32
			if( _beginthread( ( void (*)(void *) )StartSending, 0, (void *)(*i) ) == -1 )
			{
				UTIL_LogPrint( "server warning - unable to spawn sender thread\n" );

				(*i)->m_iState = CS_DEAD;
			}
#else
			pthread_t thread;

			// set detached state since we don't need to join with any threads

			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

			int c = pthread_create( &thread, &attr, ( void * (*)(void *) )StartSending, (void *)(*i) );

			if( c != 0 )
			{
				UTIL_LogPrint( "server warning - unable to spawn sender thread (error %s)\n", strerror( c ) );

				(*i)->m_iState = CS_DEAD;
			}
#endif

			i++;
		}
		else if( (*i)->m_iState == CS_DEAD )
		{
			delete *i;

			i = m_vecClients.erase( i );
		}
		else
			i++;
	}

	if( m_pTracker )
		m_pTracker->Update( );

	return m_bKill;
}

CTracker *CServer :: getTracker( )
{
	return m_pTracker;
}

bool CServer :: AddListener( struct sockaddr_in sin )
{
	SOCKET sckListener;

	// map protocol name to protocol number

	struct protoent *pPE;
	
	pPE = getprotobyname( "tcp" );

	if( pPE == 0 )
	{
		UTIL_LogPrint( "server error - unable to get tcp protocol entry (error %d)\n", GetLastError( ) );

		return false;
	}

	// allocate socket
	
	sckListener = socket( PF_INET, SOCK_STREAM, pPE->p_proto );

	if( sckListener == INVALID_SOCKET )
	{
		UTIL_LogPrint( "server error - unable to allocate socket (error %d)\n", GetLastError( ) );

		return false;
	}
	// bind socket

	int iOptVal = 1;
	const char *on = "1";

#ifdef WIN32
	// Windows
	// TCP window size 
	// Send
	if( setsockopt( sckListener, SOL_SOCKET, SO_SNDBUF, (const char *)&giSO_SNDBUF, sizeof(giSO_SNDBUF) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - setsockopt SO_SNDBUF (%d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}

	// Receive
	if( setsockopt( sckListener, SOL_SOCKET, SO_RCVBUF, (const char *)&giSO_RECBUF, sizeof(giSO_RECBUF) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - setsockopt SO_RCVBUF (%d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}

	// 	Allows the socket to be bound to an address that is already in use.
	if( setsockopt( sckListener, SOL_SOCKET, SO_REUSEADDR, (const char *)&iOptVal, sizeof( int ) ) == SOCKET_ERROR )
	{
		if(GetLastError() != 0)
		{
            UTIL_LogPrint( "server warning - setsockopt SO_REUSEADDR (%d)\n", GetLastError( ) );
            UTIL_LogPrint( "Binding to an in-use socket had issues. Will attempt to use the socket anyway.\n");
		}
	}

	// Forcibly Disable the Nagle Algorithm, Speeding up announce requests on Windows systems.
	if( setsockopt( sckListener, SOL_SOCKET, TCP_NODELAY, on, sizeof( on ) ) == SOCKET_ERROR )
		UTIL_LogPrint( "Server Warning: Disabling of Nagle Failed - %d \n", GetLastError( ) );

	if( bind( sckListener, (SOCKADDR*)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - unable to bind socket (error %d)\n", GetLastError( ) );
		
		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}
#else
	// Everyone Else
	// TCP window size 
	// Send
	if( setsockopt( sckListener, SOL_SOCKET, SO_SNDBUF, (const char *)&giSO_SNDBUF, (socklen_t)sizeof(giSO_SNDBUF) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - setsockopt SO_SNDBUF (%d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}

	// Receive
	if( setsockopt( sckListener, SOL_SOCKET, SO_RCVBUF, (const char *)&giSO_RECBUF, (socklen_t)sizeof(giSO_RECBUF) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - setsockopt SO_RCVBUF (%d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}

	//	Allows the socket to be bound to an address that is already in use.
	// SO_REUSEADDR is not supported on Linux and has some issues on *BSD platforms.
	// If setting the SO_REUSEADDR fails, binding will still attempt to happen.
	if( setsockopt( sckListener, SOL_SOCKET, SO_REUSEADDR, (const void *)&iOptVal, sizeof( int ) ) == SOCKET_ERROR );
	{
		if(GetLastError() != 0)
		{
			UTIL_LogPrint( "server warning - setsockopt SO_REUSEADDR (%d)\n", GetLastError( ) );
			UTIL_LogPrint( "Binding to an in-use socket had issues. Will attempt to use the socket anyway.\n");
		}
	}

	// Forcibly Disable the Nagle Algorithm, Speeding up announce requests on Windows systems.
	if( setsockopt( sckListener, SOL_SOCKET, TCP_NODELAY, on, sizeof( on ) ) == SOCKET_ERROR )
		UTIL_LogPrint( "Server Warning: Disabling of Nagle Failed - %d \n", GetLastError( ) );

	if( bind( sckListener, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - unable to bind socket (error %d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}
#endif

	// listen, queue length 8

	if( listen( sckListener, 8 ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "server error - unable to listen (error %d)\n", GetLastError( ) );

		int rc = closesocket( sckListener );

		if( rc == SOCKET_ERROR )
			UTIL_LogPrint( "server error - Closing Socket (%d)\n", GetLastError( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "server - Socket Closed\n" );
		}

		return false;
	}
	m_vecListeners.push_back( sckListener );
return true;
}