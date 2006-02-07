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
#include "base64.h"
#include "client.h"
#include "md5.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

#include <zlib.h>

CClient :: CClient( SOCKET sckClient, struct sockaddr_in sinAddress, struct timeval tvTimeOut, int iCompression )
{
	m_sckClient = sckClient;

	m_tvTimeOut = tvTimeOut;
	m_iCompression = iCompression;

	rqst.sin = sinAddress;
	rsp.bCompressOK = true;

	m_iState = CS_RECEIVING;
}

CClient :: ~CClient( )
{
	int rc = closesocket( m_sckClient );
	
	if( rc == SOCKET_ERROR )
		UTIL_LogPrint( "client error - Closing Socket (%d)\n", GetLastError( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "client - Socket Closed\n" );
	}
}

void CClient :: StartReceiving( )
{
	fd_set fdClient;

	m_strReceiveBuf.erase( );
	m_strReceiveBuf.reserve( 1024 );

	rqst.strMethod.erase( );
	rqst.strURL.erase( );
	rqst.mapParams.clear( );
	rqst.mapHeaders.clear( );

	rsp.strCode.erase( );
	rsp.mapHeaders.clear( );
	rsp.strContent.erase( );
	rsp.strContent.reserve( 1024 );

	m_bKeepAlive = false;

	while( m_strReceiveBuf.find( "\r\n\r\n" ) == string :: npos )
	{
		if( m_strReceiveBuf.size( ) > giMaxRecvSize )
		{
			UTIL_LogPrint( "client error - exceeded max recv size\n" );

			m_iState = CS_DEAD;
			return;
		}

		FD_ZERO( &fdClient );
		FD_SET( m_sckClient, &fdClient );

#ifdef WIN32
		if( select( 1, &fdClient, NULL, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#else
		if( select( m_sckClient + 1, &fdClient, NULL, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "client error - select error (error %d)\n", GetLastError( ) );

			m_iState = CS_DEAD;
			return;
		}

		if( FD_ISSET( m_sckClient, &fdClient ) )
		{
			char pTemp[8192];

			memset( pTemp, 0, sizeof( char ) * 8192 );

			// don't receive more than the buffer size

			int c = recv( m_sckClient, pTemp, 8192, 0 );

			if( c == SOCKET_ERROR )
			{
				if( GetLastError( ) != ECONNRESET )
					UTIL_LogPrint( "client error - receive error (error %d)\n", GetLastError( ) );

				m_iState = CS_DEAD;
				return;
			}
			else if( c == 0 )
			{
				m_iState = CS_DEAD;
				return;
			}
			else if( c > 0 )
				m_strReceiveBuf += string( pTemp, c );
			else
			{
				UTIL_LogPrint( "client error - recv returned garbage\n" );

				m_iState = CS_DEAD;
				return;
			}
		}
		else
		{
			m_iState = CS_DEAD;
			return;
		}
	}

	// grab method

	string :: size_type iMethodEnd = m_strReceiveBuf.find( " " );

	if( iMethodEnd == string :: npos )
	{
		UTIL_LogPrint( "client error - malformed HTTP request (can't find method)\n" );

		m_iState = CS_DEAD;
		return;
	}

	rqst.strMethod = m_strReceiveBuf.substr( 0, iMethodEnd );

	// grab url

	string strTemp = m_strReceiveBuf.substr( iMethodEnd + 1 );

	string :: size_type iURLEnd = strTemp.find( " " );

	if( iURLEnd == string :: npos )
	{
		UTIL_LogPrint( "client error - malformed HTTP request (can't find URL)\n" );

		m_iState = CS_DEAD;
		return;
	}

	strTemp = strTemp.substr( 0, iURLEnd );

	string :: size_type iParamsStart = strTemp.find( "?" );

	if( iParamsStart == string :: npos ){
		rqst.strURL = strTemp;
		rqst.hasQuery = false;
	}
	else {
        rqst.strURL = strTemp.substr( 0, iParamsStart );
		rqst.hasQuery = true;
	}

	// grab params
	if( iParamsStart != string :: npos )
	{
		strTemp = strTemp.substr( iParamsStart + 1 );
		rqst.hasQuery = true;

		while( 1 )
		{
			string :: size_type iSplit = strTemp.find( "=" );
			string :: size_type iEnd = strTemp.find( "&" );

			if( iSplit == string :: npos )
			{
				UTIL_LogPrint( "client warning - malformed HTTP request (found param key without value)\n" );

				break;
			}

			string strKey = UTIL_EscapedToString( strTemp.substr( 0, iSplit ) );
			string strValue = UTIL_EscapedToString( strTemp.substr( iSplit + 1, iEnd - iSplit - 1 ) );

			// multimap for scrape, regular map for everything else
			if ( rqst.strURL == "/scrape" )
				rqst.multiParams.insert( pair<string,string>( strKey, strValue ) );
			else
				rqst.mapParams.insert( pair<string, string>( strKey, strValue ) );

			strTemp = strTemp.substr( iEnd + 1, strTemp.size( ) - iEnd - 1 );

			if( iEnd == string :: npos )
				break;
		}
	}

	// grab headers

	string :: size_type iNewLine = m_strReceiveBuf.find( "\r\n" );
	string :: size_type iDoubleNewLine = m_strReceiveBuf.find( "\r\n\r\n" );

	strTemp = m_strReceiveBuf.substr( iNewLine + strlen( "\r\n" ), iDoubleNewLine - iNewLine - strlen( "\r\n" ) );

	while( 1 )
	{
		string :: size_type iSplit = strTemp.find( ":" );
		string :: size_type iEnd = strTemp.find( "\r\n" );

		// http://www.addict3d.org/index.php?page=viewarticle&type=security&ID=4861
		if( iSplit == string :: npos || iSplit == 0 )
		{
			UTIL_LogPrint( "client warning - malformed HTTP request (bad header)\n" );

			break;
		}

		string strKey = strTemp.substr( 0, iSplit );
		string strValue = strTemp.substr( iSplit + strlen( ": " ), iEnd - iSplit - strlen( "\r\n" ) );

		rqst.mapHeaders.insert( pair<string, string>( strKey, strValue ) );

		strTemp = strTemp.substr( iEnd + strlen( "\r\n" ) );

		if( iEnd == string :: npos )
			break;
	}

	// grab cookies

	string strCookies = rqst.mapHeaders["Cookie"];

	if( !strCookies.empty( ) )
	{
		while( 1 )
		{
			string :: size_type iWhite = strCookies.find_first_not_of( " " );

			if( iWhite != string :: npos )
				strCookies = strCookies.substr( iWhite );

			string :: size_type iSplit = strCookies.find( "=" );
			string :: size_type iEnd = strCookies.find( ";" );

			if( iSplit == string :: npos )
			{
				UTIL_LogPrint( "client warning - malformed HTTP request (found cookie key without value)\n" );

				break;
			}

			string strKey = UTIL_EscapedToString( strCookies.substr( 0, iSplit ) );
			string strValue = UTIL_EscapedToString( strCookies.substr( iSplit + 1, iEnd - iSplit - 1 ) );

			// strip quotes

			if( strValue.size( ) > 1 && strValue[0] == '"' )
				strValue = strValue.substr( 1, strValue.size( ) - 2 );

			rqst.mapCookies.insert( pair<string, string>( strKey, strValue ) );

			strCookies = strCookies.substr( iEnd + 1, strCookies.size( ) - iEnd - 1 );

			if( iEnd == string :: npos )
				break;
		}
	}

	// don't grab authentication here

	if( rqst.strMethod == "POST" )
	{
		string strContentLength = rqst.mapHeaders["Content-Length"];

		if( strContentLength.empty( ) )
		{
			UTIL_LogPrint( "client error - malformed HTTP request (no Content-Length with POST)\n" );

			m_iState = CS_DEAD;
			return;
		}

		long iLength = atol( strContentLength.c_str( ) );

		string :: size_type iContent = iDoubleNewLine + strlen( "\r\n\r\n" );

		while( m_strReceiveBuf.size( ) < iContent + iLength )
		{
			if( m_strReceiveBuf.size( ) > giMaxRecvSize )
			{
				UTIL_LogPrint( "client error - exceeded max recv size\n" );

				m_iState = CS_DEAD;
				return;
			}

			FD_ZERO( &fdClient );
			FD_SET( m_sckClient, &fdClient );

#ifdef WIN32
			if( select( 1, &fdClient, NULL, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#else
			if( select( m_sckClient + 1, &fdClient, NULL, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#endif
			{
				UTIL_LogPrint( "client error - select error (error %d)\n", GetLastError( ) );

				m_iState = CS_DEAD;
				return;
			}

			if( FD_ISSET( m_sckClient, &fdClient ) )
			{
				char pTemp[8192];

				memset( pTemp, 0, sizeof( char ) * 8192 );

				// don't receive more than the buffer size

				int c = recv( m_sckClient, pTemp, 8192, 0 );

				if( c == SOCKET_ERROR )
				{
					if( GetLastError( ) != ECONNRESET )
						UTIL_LogPrint( "client error - receive error (error %d)\n", GetLastError( ) );

					m_iState = CS_DEAD;
					return;
				}
				else if( c == 0 )
				{
					m_iState = CS_DEAD;
					return;
				}
				else if( c > 0 )
					m_strReceiveBuf += string( pTemp, c );
				else
				{
					UTIL_LogPrint( "client error - recv returned garbage\n" );

					m_iState = CS_DEAD;
					return;
				}
			}
			else
			{
				m_iState = CS_DEAD;
				return;
			}
		}
	}

	m_iState = CS_WAITING1;
}

void CClient :: Process( )
{
	m_iState = CS_PROCESSING;

	// grab authentication

	string strLogin = rqst.mapCookies["login"];
	string strMD5 = rqst.mapCookies["md5"];

	string strAuthorization = rqst.mapHeaders["Authorization"];

	if( !strAuthorization.empty( ) )
	{
		string :: size_type iWhite = strAuthorization.find( " " );

		if( iWhite != string :: npos )
		{
			string strType = strAuthorization.substr( 0, iWhite );
			string strBase64 = strAuthorization.substr( iWhite + 1 );

			if( strType == "Basic" )
			{
				char *szAuth = b64decode( strBase64.c_str( ) );

				if( szAuth )
				{
					string strAuth = szAuth;

					free( szAuth );

					string :: size_type iSplit = strAuth.find( ":" );

					if( iSplit != string :: npos )
					{
						strLogin = strAuth.substr( 0, iSplit );
						string strPass = strAuth.substr( iSplit + 1 );

						// calculate md5 hash of A1

						string strA1 = strLogin + ":" + gstrRealm + ":" + strPass;

						unsigned char szMD5[16];

						MD5_CTX md5;

						MD5Init( &md5 );
						MD5Update( &md5, (unsigned char *)strA1.c_str( ), strA1.size( ) );
						MD5Final( szMD5, &md5 );

						strMD5 = string( (char *)szMD5, 16 );
					}
				}
			}
		}
	}

	user_t user = gpServer->getTracker( )->checkUser( strLogin, strMD5 );

	// process

	if( rqst.strMethod == "GET" )
		gpServer->getTracker( )->serverResponseGET( &rqst, &rsp, user );
	else if( rqst.strMethod == "POST" )
	{
		CAtomList *pPost = UTIL_DecodeHTTPPost( m_strReceiveBuf );

		gpServer->getTracker( )->serverResponsePOST( &rqst, &rsp, pPost, user );

		if( pPost )
			delete pPost;

		pPost = NULL;
	}
	else
		rsp.strCode = "400 Bad Request";

	// compress

	int iCompress = COMPRESS_NONE;

	if( rsp.bCompressOK && m_iCompression > 0 )
	{
		string strAcceptEncoding = UTIL_ToLower( rqst.mapHeaders["Accept-Encoding"] );

		if( strAcceptEncoding.find( "gzip" ) != string :: npos )
			iCompress = COMPRESS_GZIP;
		else if( strAcceptEncoding.find( "deflate" ) != string :: npos )
			iCompress = COMPRESS_DEFLATE;
	}

	if( !rsp.strContent.empty( ) && iCompress != COMPRESS_NONE )
	{
		// allocate avail_in * 1.001 + 18 bytes (12 + 6 for gzip)

		unsigned int iSize = (unsigned int)( rsp.strContent.size( ) * 1.001 + 18 );

		unsigned char *pBuf = new unsigned char[iSize];
		memset( pBuf, 0, sizeof( unsigned char ) * iSize );

		z_stream_s zCompress;

		zCompress.next_in = (unsigned char *)rsp.strContent.c_str( );
		zCompress.avail_in = rsp.strContent.size( );
		zCompress.next_out = pBuf;
		zCompress.avail_out = iSize;
		zCompress.zalloc = (alloc_func)0;
		zCompress.zfree = (free_func)0;
		zCompress.opaque = (voidpf)0;
		zCompress.total_in = 0;
		zCompress.total_out = 0;

		int windowBits;

		if( iCompress == COMPRESS_GZIP )
			windowBits = 31;
		else
			windowBits = 15;

		int iResult = deflateInit2( &zCompress, m_iCompression, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY );

		if( iResult == Z_OK )
		{
			iResult = deflate( &zCompress, Z_FINISH );

			if( iResult == Z_STREAM_END )
			{
				if( gbDebug )
					UTIL_LogPrint( "client - (zlib) compressed %u bytes to %u bytes\n", zCompress.total_in, zCompress.total_out );

				if( iCompress == COMPRESS_GZIP )
					rsp.mapHeaders.insert( pair<string, string>( "Content-Encoding", "gzip" ) );
				else
					rsp.mapHeaders.insert( pair<string, string>( "Content-Encoding", "deflate" ) );

				rsp.strContent = string( (char *)pBuf, zCompress.total_out );

				deflateEnd( &zCompress );

				delete [] pBuf;
			}
			else
			{
				if( iResult != Z_OK )
					UTIL_LogPrint( "client warning - (zlib) deflate error (%d) on \"%s\", in = %u, sending raw\n", iResult, rqst.strURL.c_str( ), rsp.strContent.size( ) );

				deflateEnd( &zCompress );

				delete [] pBuf;
			}
		}
		else
		{
			UTIL_LogPrint( "client warning - (zlib) deflateInit2 error (%d), sending raw\n", iResult );

			delete [] pBuf;
		}
	}

	// keep alive

	if( UTIL_ToLower( rqst.mapHeaders["Connection"] ) == "keep-alive" )
	{
		if( !gpServer->isDying( ) )
		{
			m_bKeepAlive = true;

			rsp.mapHeaders.insert( pair<string, string>( "Connection", "Keep-Alive" ) );
			rsp.mapHeaders.insert( pair<string, string>( "Keep-Alive", CAtomInt( m_tvTimeOut.tv_sec - 1 ).toString( ) ) );
		}
		else
		{
			m_bKeepAlive = false;

			rsp.mapHeaders.insert( pair<string, string>( "Connection", "Close" ) );
			rsp.mapHeaders.insert( pair<string, string>( "Keep-Alive", CAtomInt( m_tvTimeOut.tv_sec - 1 ).toString( ) ) );
		}
	}

	rsp.mapHeaders.insert( pair<string, string>( "Content-Length", CAtomLong( rsp.strContent.size( ) ).toString( ) ) );

	// access log

	string strRequest;

	int iNewLine = m_strReceiveBuf.find( "\r\n" );

	if( iNewLine != string :: npos )
		strRequest = m_strReceiveBuf.substr( 0, iNewLine );

	UTIL_AccessLogPrint( inet_ntoa( rqst.sin.sin_addr ), user.strLogin, strRequest, atoi( rsp.strCode.substr( 0, 3 ).c_str( ) ), rsp.strContent.size( ) );

	m_iState = CS_WAITING2;
}

void CClient :: StartSending( )
{
	fd_set fdClient;

	// compose send buffer

	string strSendBuf;
	strSendBuf.reserve( 1024 );

	strSendBuf += "HTTP/1.1 " + rsp.strCode + "\r\n";

	for( multimap<string, string> :: iterator i = rsp.mapHeaders.begin( ); i != rsp.mapHeaders.end( ); i++ )
		strSendBuf += (*i).first + ": " + (*i).second + "\r\n";

	strSendBuf += "\r\n";
	strSendBuf += rsp.strContent;

	// send

	while( !strSendBuf.empty( ) )
	{
		FD_ZERO( &fdClient );
		FD_SET( m_sckClient, &fdClient );

#ifdef WIN32
		if( select( 1, NULL, &fdClient, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#else
		if( select( m_sckClient + 1, NULL, &fdClient, NULL, &m_tvTimeOut ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "client error - select error (error %d)\n", GetLastError( ) );

			m_iState = CS_DEAD;
			return;
		}

		if( FD_ISSET( m_sckClient, &fdClient ) )
		{
			int s = send( m_sckClient, strSendBuf.c_str( ), strSendBuf.size( ), MSG_NOSIGNAL );

			if( s == SOCKET_ERROR )
			{
				UTIL_LogPrint( "client error - send error (error %d)\n", GetLastError( ) );

				m_iState = CS_DEAD;
				return;
			}
			else if( s == 0 )
			{
				UTIL_LogPrint( "client error - send returned 0\n" );

				m_iState = CS_DEAD;
				return;
			}
			else if( s > 0 )
				strSendBuf = strSendBuf.substr( s );
			else
			{
				UTIL_LogPrint( "client error - send returned garbage\n" );

				m_iState = CS_DEAD;
				return;
			}
		}
		else
		{
			m_iState = CS_DEAD;
			return;
		}
	}

	if( m_bKeepAlive )
	{
		m_iState = CS_RECEIVING;

		StartReceiving( );
	}
	else
		m_iState = CS_DEAD;
}

void StartReceiving( CClient *pClient )
{
	if( pClient )
		pClient->StartReceiving( );
}

void StartSending( CClient *pClient )
{
	if( pClient )
		pClient->StartSending( );
}
