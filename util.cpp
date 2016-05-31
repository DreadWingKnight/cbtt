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

#ifdef __GNUWIN32__
 #define unlink remove
#endif

#ifndef WIN32
 #include <sys/time.h>
#endif

#include <stdarg.h>

#include "bnbt.h"
#include "atom.h"
#include "bencode.h"
#include "sha1.h"
#include "tracker.h"
#include "util.h"

void UTIL_LogPrint( const char *format, ... )
{
	time_t tNow = time( NULL );
	char *szTime = asctime( localtime( &tNow ) );
	szTime[strlen( szTime ) - 1] = '\0';

	gmtxOutput.Claim( );

	printf( "[%s] ", szTime );

	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );

	if( !gstrErrorLogDir.empty( ) && !gstrErrorLogFilePattern.empty( ) )
	{
		char pTime[256];
		memset( pTime, 0, sizeof( char ) * 256 );
		strftime( pTime, sizeof( char ) * 256, gstrErrorLogFilePattern.data( ), localtime( &tNow ) );

		string strFile = gstrErrorLogDir + pTime;

		if( gstrErrorLogFile != strFile )
		{
			// start a new log

			gstrErrorLogFile = strFile;

			if( gpErrorLog )
			{
				fclose( gpErrorLog );

				gpErrorLog = NULL;
			}

			gpErrorLog = fopen( strFile.c_str( ), "ab" );
		}

		if( gpErrorLog )
		{
			fprintf( gpErrorLog, "[%s] ", szTime );

			va_list args;
			va_start( args, format );
			vfprintf( gpErrorLog, format, args );
			va_end( args );

			giErrorLogCount++;

			if( giErrorLogCount % giFlushInterval == 0 )
				fflush( gpErrorLog );
		}
	}

	gmtxOutput.Release( );
}

//addition by labarks
string UTIL_Date( )
{
	time_t tNow = time( NULL );
	string strDate;
	
	char pTime[256];
	memset( pTime, 0, sizeof( char ) * 256 );
	strftime( pTime, sizeof( char ) * 256, "%a, %d %b %Y %H:%M:%S", localtime( &tNow ) );
	#if defined( __APPLE__ ) || defined( __FREEBSD__ )
			long timezone = -( localtime( &tNow )->tm_gmtoff );
	#endif

	// timezone has the wrong sign, change it
			
	if( timezone > 0 )
		strDate = (string) pTime + " -" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );
	else
		strDate = (string) pTime + " +" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );

	return strDate;
}

string UTIL_AddedToDate( string strAdded )
{
	time_t rawtime;
	struct tm * tDate; //2004-05-09 01:30:21
	string strDate;
	
	time ( &rawtime );
	tDate = localtime ( &rawtime );
	tDate->tm_year = UTIL_StringToInt( strAdded.substr(0,4) ) - 1900;
	tDate->tm_mon  = UTIL_StringToInt( strAdded.substr(5,2) ) - 1;
	tDate->tm_mday  = UTIL_StringToInt( strAdded.substr(8,2) );
	tDate->tm_hour = UTIL_StringToInt( strAdded.substr(11,2) );
	tDate->tm_min  = UTIL_StringToInt( strAdded.substr(14,2) );
	tDate->tm_sec  = UTIL_StringToInt( strAdded.substr(17,2) );
	mktime(tDate);
	
	char pTime[256];
	memset( pTime, 0, sizeof( char ) * 256 );
	strftime( pTime, sizeof( char ) * 256, "%a, %d %b %Y %H:%M:%S", tDate );
#if defined( __APPLE__ ) || defined( __FREEBSD__ )
	time_t tNow = time( NULL );
	long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

	// timezone has the wrong sign, change it
			
	if( timezone > 0 )
		strDate = (string) pTime + " -" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );
	else
		strDate = (string) pTime + " +" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );

	return strDate;
}

string addzero( unsigned long iLong ) //for timezones used in UTIL_Date and UTIL_AddedToDate
{
	if( iLong < 10 )
		return (string) "0" + string() + UTIL_LongToString( iLong );
	else
		return UTIL_LongToString( iLong );
}
//end addition

void UTIL_AccessLogPrint( string strIP, string strUser, string strRequest, int iStatus, int iBytes )
{
	gmtxOutput.Claim( );

	if( !gstrAccessLogDir.empty( ) && !gstrAccessLogFilePattern.empty( ) )
	{
		time_t tNow = time( NULL );

		char pTime[256];
		memset( pTime, 0, sizeof( char ) * 256 );
		strftime( pTime, sizeof( char ) * 256, gstrAccessLogFilePattern.data( ), localtime( &tNow ) );

		string strFile = gstrAccessLogDir + pTime;

		if( gstrAccessLogFile != strFile )
		{
			// start a new log

			gstrAccessLogFile = strFile;

			if( gpAccessLog )
			{
				fclose( gpAccessLog );

				gpAccessLog = NULL;
			}

			gpAccessLog = fopen( strFile.c_str( ), "ab" );

			if( !gpAccessLog )
				UTIL_LogPrint( "log warning - unable to open %s for writing\n", strFile.c_str( ) );
		}

		if( gpAccessLog )
		{
			fprintf( gpAccessLog, "%s - ", strIP.c_str( ) );

			if( strUser.empty( ) )
				fprintf( gpAccessLog, "- " );
			else
				fprintf( gpAccessLog, "%s ", strUser.c_str( ) );

			strftime( pTime, sizeof( char ) * 256, "%d/%b/%Y:%H:%M:%S", localtime( &tNow ) );

#if defined( __APPLE__ ) || defined( __FREEBSD__ )
			long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

			// timezone has the wrong sign, change it

			if( timezone > 0 )
				fprintf( gpAccessLog, "[%s -%02ld%02ld] ", pTime, abs( timezone / 3600 ) % 60, abs( timezone / 60 ) % 60 );
			else
				fprintf( gpAccessLog, "[%s +%02ld%02ld] ", pTime, abs( timezone / 3600 ) % 60, abs( timezone / 60 ) % 60 );

			fprintf( gpAccessLog, "\"%s\" %d %d\n", strRequest.c_str( ), iStatus, iBytes );

			giAccessLogCount++;

			if( giAccessLogCount % giFlushInterval == 0 )
				fflush( gpAccessLog );
		}
	}

	gmtxOutput.Release( );
}

//
// shamelessly stolen from wget source code
//

struct bnbttv UTIL_CurrentTime( )
{
	struct bnbttv btv;

#ifdef WIN32
	FILETIME ft;
	SYSTEMTIME st;
	GetSystemTime( &st );
	SystemTimeToFileTime( &st, &ft );
	btv.wintime.HighPart = ft.dwHighDateTime;
	btv.wintime.LowPart = ft.dwLowDateTime;
#else
	struct timeval tv;
	gettimeofday( &tv, NULL );
	btv.sec = tv.tv_sec;
	btv.usec = tv.tv_usec;
#endif

	return btv;
}

long UTIL_ElapsedTime( struct bnbttv btvStart, struct bnbttv btvEnd )
{
#ifdef WIN32
	return (long)( ( btvEnd.wintime.QuadPart - btvStart.wintime.QuadPart ) / 10000 );
#else
	return ( btvEnd.sec - btvStart.sec ) * 1000 + ( btvEnd.usec - btvStart.usec ) / 1000;
#endif
}

string UTIL_ElapsedTimeStr( struct bnbttv btvStart, struct bnbttv btvEnd )
{
	char szGen[8];
	memset( szGen, 0, sizeof( char ) * 8 );
    sprintf( szGen, "%0.3f", UTIL_ElapsedTime( btvStart, btvEnd ) / 1000.0 );

	return szGen;
}

string UTIL_EscapedToString( const string &strEscape )
{
	// according to RFC 2396

	string strString;

	for( unsigned long i = 0; i < strEscape.size( ); )
	{
		if( strEscape[i] == '%' )
		{
			if( i < strEscape.size( ) - 2 )
			{
				char pBuf[4];

				memset( pBuf, 0, sizeof( char ) * 4 );

				pBuf[0] = strEscape[i + 1];
				pBuf[1] = strEscape[i + 2];

				unsigned int c;

				sscanf( pBuf, "%02X", &c );

				strString += c;

				i += 3;
			}
			else
			{
				UTIL_LogPrint( "error decoding escaped string - possible truncation, halting decode\n" );

				return strString;
			}
		}
		else if( strEscape[i] == '+' )
		{
			strString += ' ';

			i++;
		}
		else
		{
			strString += strEscape[i];

			i++;
		}
	}

	return strString;
}

string UTIL_HashToString( const string &strHash )
{
	// convert a 20 character hash to a readable string

	string strString;

	if( strHash.size( ) != 20 )
		return string( );

	for( unsigned long i = 0; i < strHash.size( ); i++ )
	{
		char pBuf[4];

		memset( pBuf, 0, sizeof( char ) * 4 );

		// this must be unsigned or some really strange errors appear (i.e. the value of i is reset to zero every loop)

		unsigned char c = strHash[i];

		sprintf( pBuf, "%02x", c );

		strString += pBuf;
	}

	return strString;
}

string UTIL_BytesToString( int64 iBytes )
{
	int64 iB = iBytes % 1024;
	iBytes /= 1024;
	int64 iKB = iBytes % 1024;
	iBytes /= 1024;
	int64 iMB = iBytes % 1024;
	iBytes /= 1024;
	int64 iGB = iBytes % 1024;
	iBytes /= 1024;
	int64 iTB = iBytes % 1024;
	iBytes /= 1024;
	int64 iPB = iBytes;

	// B -> KB -> MB -> GB -> TB -> PB -> EB -> ZB -> YB

	string strBytes;

	if( iPB > 0 )
	{
		int iFrac = (int)( (float)iTB / (float)1024 * (float)100 );

		strBytes += CAtomLong( iPB ).toString( );
		strBytes += ".";

		if( CAtomInt( iFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( iFrac ).toString( );
		strBytes += " PB";
	}
	else if( iTB > 0 )
	{
		int iFrac = (int)( (float)iGB / (float)1024 * (float)100 );

		strBytes += CAtomLong( iTB ).toString( );
		strBytes += ".";

		if( CAtomInt( iFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( iFrac ).toString( );
		strBytes += " TB";
	}
	else if( iGB > 0 )
	{
		int iFrac = (int)( (float)iMB / (float)1024 * (float)100 );

		strBytes += CAtomLong( iGB ).toString( );
		strBytes += ".";

		if( CAtomInt( iFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( iFrac ).toString( );
		strBytes += " GB";
	}
	else if( iMB > 0 )
	{
		int iFrac = (int)( (float)iKB / (float)1024 * (float)100 );

		strBytes += CAtomLong( iMB ).toString( );
		strBytes += ".";

		if( CAtomInt( iFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( iFrac ).toString( );
		strBytes += " MB";
	}
	else if( iKB > 0 )
	{
		int iFrac = (int)( (float)iB / (float)1024 * (float)100 );

		strBytes += CAtomLong( iKB ).toString( );
		strBytes += ".";

		if( CAtomInt( iFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( iFrac ).toString( );
		strBytes += " KB";
	}
	else
	{
		strBytes += CAtomLong( iB ).toString( );
		strBytes += " B";
	}

	return strBytes;
}

string UTIL_SecondsToString( unsigned long iSeconds )
{
	// int iS = iSeconds % 60;

	iSeconds /= 60;
	int iM = iSeconds % 60;
	iSeconds /= 60;
	int iH = iSeconds % 24;
	iSeconds /= 24;
	int iD = iSeconds;

	string strSeconds;

	strSeconds += CAtomInt( iD ).toString( );
	strSeconds += "d ";

	if( CAtomInt( iH ).toString( ).size( ) == 1 )
		strSeconds += "0";

	strSeconds += CAtomInt( iH ).toString( );
	strSeconds += ":";

	if( CAtomInt( iM ).toString( ).size( ) == 1 )
		strSeconds += "0";

	strSeconds += CAtomInt( iM ).toString( );

	/*

	strSeconds += ":";

	if( CAtomInt( iS ).toString( ).size( ) == 1 )
		strSeconds += "0";

	strSeconds += CAtomInt( iS ).toString( );

	*/

	return strSeconds;
}
//addition by labarks
string UTIL_IntToString( int intInt )
{
	string strInt;

	strInt = CAtomInt( intInt ).toString( );
	
	return strInt;
}

string UTIL_LongToString( unsigned long iLong )
{
	string strLong;
	
	iLong = (int64) iLong;

	strLong = CAtomLong( iLong ).toString( );
	
	return strLong;
}

int UTIL_StringToInt( string strInt )
{
	return atoi( strInt.c_str() );
}

unsigned long UTIL_FileSize( const char *szFile )
{
	FILE *pFile = NULL;
	if( ( pFile = fopen( szFile, "rb" ) ) == NULL )
	{
		//UTIL_LogPrint( "error opening file - unable to open %s for reading\n", szFile );
		return 0;
	}
	fseek( pFile, 0, SEEK_END );
	unsigned long ulFileSize = ftell( pFile );
	fclose( pFile );

	return ulFileSize;
}

string UTIL_FileSizeToString( const char *szFile )
{
	return UTIL_LongToString( UTIL_FileSize( szFile ) );
}
//end addition

string UTIL_StringToEscaped( const string &strString )
{
	// according to RFC 2396

	string strEscape;

	for( unsigned long i = 0; i < strString.size( ); i++ )
	{
		unsigned char c = strString[i];

		if( isalpha( c ) || isdigit( c ) ||
			c == '-' ||
			c == '_' ||
			c == '.' ||
			c == '!' ||
			c == '~' ||
			c == '*' ||
			c == '\'' ||
			c == '(' ||
			c == ')' )
		{
			// found an unreserved character

			strEscape += c;
		}
		else if( c == ' ' )
			strEscape += '+';
		else
		{
			// found a reserved character

			char pBuf[4];

			memset( pBuf, 0, sizeof( char ) * 4 );

			sprintf( pBuf, "%02X", c );

			strEscape += "%";
			strEscape += pBuf;
		}
	}

	return strEscape;
}

//addition by labarks
string UTIL_URLEncode( const string &strString )
{
	// according to RFC 2396

	string strEscape;

	for( unsigned long i = 0; i < strString.size( ); i++ )
	{
		unsigned char c = strString[i];

		if( isalpha( c ) || isdigit( c ) ||
			c == '/' ||
			c == '&' ||
			c == ';' ||
			c == '?' ||
			c == ':' ||
			c == '@' ||
			c == '+' ||
			c == '=' ||
			c == '$' ||
			c == ',' ||
			c == '-' ||
			c == '_' ||
			c == '.' ||
			c == '!' ||
			c == '~' ||
			c == '*' ||
			c == '\'' ||
			c == '(' ||
			c == ')' )
		{
			// found an unreserved character

			strEscape += c;
		}
		else
		{
			// found a reserved character

			char pBuf[4];

			memset( pBuf, 0, sizeof( char ) * 4 );

			sprintf( pBuf, "%02X", c );

			strEscape += "%";
			strEscape += pBuf;
		}
	}

	return strEscape;
}
//end addition

string UTIL_StringToEscapedStrict( const string &strString )
{
	string strEscape;

	for( unsigned long i = 0; i < strString.size( ); i++ )
	{
		unsigned char c = strString[i];

		if( isalpha( c ) || isdigit( c ) ||
			c == '-' ||
			c == '_' ||
			c == '.' ||
			c == '!' ||
			c == '~' ||
			c == '*' ||
			c == '\'' ||
			c == '(' ||
			c == ')' )
		{
			// found an unreserved character

			strEscape += c;
		}
		else
		{
			// found a reserved character

			char pBuf[4];

			memset( pBuf, 0, sizeof( char ) * 4 );

			sprintf( pBuf, "%02X", c );

			strEscape += "%";
			strEscape += pBuf;
		}
	}

	return strEscape;
}

string UTIL_StringToHash( const string &strString )
{
	// convert a readable string hash to a 20 character hash

	string strHash;

	if( strString.size( ) != 40 )
		return string( );

	for( unsigned long i = 0; i < strString.size( ); i += 2 )
	{
		char pBuf[4];

		memset( pBuf, 0, sizeof( char ) * 4 );

		pBuf[0] = strString[i];
		pBuf[1] = strString[i + 1];

		unsigned int c;

		sscanf( pBuf, "%02x", &c );

		strHash += c;
	}

	return strHash;
}

string UTIL_AccessToString( int iAccess )
{
	if( iAccess & ACCESS_ADMIN )
		return "Admin";
	else if( iAccess & ACCESS_EDIT )
		return "Moderator";
	else if( iAccess & ACCESS_UPLOAD )
		return "Uploader";
	else if( iAccess & ACCESS_COMMENTS )
		return "Poster";
	else if( iAccess & ACCESS_DL )
		return "Downloader";
	else if( iAccess & ACCESS_VIEW )
		return "Basic";
	else
		return "None";
}

int64 UTIL_StringTo64( const char *sz64 )
{
	int64 i;

#if defined( WIN32 )
	sscanf( sz64, "%I64d", &i );
#elif defined( __FREEBSD__ )
	sscanf( sz64, "%qd", &i );
#else
	sscanf( sz64, "%lld", &i );
#endif

	return i;
}

string UTIL_InfoHash( CAtom *pTorrent )
{
	if( pTorrent && pTorrent->isDicti( ) )
	{
		CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );

		if( pInfo && pInfo->isDicti( ) )
		{
			// encode the string

			string strData = Encode( pInfo );

			// hash it

			CSHA1 hasher;

			hasher.Update( (unsigned char *)strData.c_str( ), strData.size( ) );
			hasher.Final( );

			UINT_8 binHash[20];
			string resultstr;

			if (hasher.GetHash(binHash)) {
				for (int i=0; i<20; i++) {
					resultstr+=binHash[i];
				}
				return resultstr;
			}
		}
	}

	return string( );
}

bool UTIL_CheckFile( const char *szFile )
{
	// check if file exists

	FILE *pFile = NULL;

	if( ( pFile = fopen( szFile, "r" ) ) == NULL )
		return false;

	fclose( pFile );

	return true;
}

void UTIL_MakeFile( const char *szFile, string strContents )
{
	FILE *pFile = NULL;

	if( ( pFile = fopen( szFile, "wb" ) ) == NULL )
	{
		UTIL_LogPrint( "warning - unable to open %s for writing\n", szFile );

		return;
	}

	fwrite( (void *)strContents.c_str( ), sizeof( char ), strContents.size( ), pFile );
	fclose( pFile );
}

void UTIL_DeleteFile( const char *szFile )
{
	if( unlink( szFile ) == 0 )
		UTIL_LogPrint( "deleted \"%s\"\n", szFile );
	else
	{
#ifdef WIN32
		UTIL_LogPrint( "error deleting \"%s\"\n", szFile );
#else
		UTIL_LogPrint( "error deleting \"%s\" - %s\n", szFile, strerror( errno ) );
#endif
	}
}

void UTIL_MoveFile( const char *szFile, const char *szDest )
{
	if( UTIL_CheckFile( szDest ) )
		UTIL_LogPrint( "error archiving \"%s\" - destination file already exists\n", szDest );
	else
		UTIL_MakeFile( szDest, UTIL_ReadFile( szFile ) );

	// thanks MrMister

	UTIL_DeleteFile( szFile );
}

string UTIL_ReadFile( const char *szFile )
{
	FILE *pFile = NULL;

	if( ( pFile = fopen( szFile, "rb" ) ) == NULL )
	{
		UTIL_LogPrint( "warning - unable to open %s for reading\n", szFile );

		return string( );
	}

	fseek( pFile, 0, SEEK_END );
	long ulFileSize = ftell( pFile );
	fseek( pFile, 0, SEEK_SET );
	char *pData = (char *)malloc( sizeof( char ) * ulFileSize );
	memset( pData, 0, sizeof( char ) * ulFileSize );
	fread( (void *)pData, sizeof( char ), ulFileSize, pFile );
	fclose( pFile );
	string strFile( pData, ulFileSize );
	free( pData );
	return strFile;
}

string UTIL_ToLower( string strUpper )
{
	for( unsigned long i = 0; i < strUpper.size( ); i++ )
		strUpper[i] = tolower( strUpper[i] );

	return strUpper;
}

string UTIL_StripPath( string strPath )
{
	string :: size_type iFileStart = strPath.rfind( '\\' );

	if( iFileStart == string :: npos )
	{
		iFileStart = strPath.rfind( '/' );

		if( iFileStart == string :: npos )
			iFileStart = 0;
		else
			iFileStart++;
	}
	else
		iFileStart++;

	return strPath.substr( iFileStart );
}

string UTIL_RemoveHTML( string strHTML )
{
	for( unsigned long i = 0; i < strHTML.size( ); i++ )
	{
		if( strHTML[i] == '<' )
			strHTML.replace( i, 1, "&lt;" );
		else if( strHTML[i] == '>' )
			strHTML.replace( i, 1, "&gt;" );
		else if( strHTML[i] == '&' )
			strHTML.replace( i, 1, "&amp;" );
		else if( strHTML[i] == '"' )
			strHTML.replace( i, 1, "&quot;" );
		else if( strHTML[i] == '\n' )
		{
			if( i > 0 )
			{
				if( strHTML[i - 1] == '\r' )
				{
					strHTML.replace( i - 1, 2, "<br>" );

					i += 2;
				}
				else
				{
					strHTML.replace( i, 1, "<br>" );

					i += 3;
				}
			}
			else
			{
				strHTML.replace( i, 1, "<br>" );

				i += 3;
			}
		}
	}

	return strHTML;
}

string UTIL_FailureReason( string strFailureReason )
{
	CAtomDicti dict;

	dict.setItem( "failure reason", new CAtomString( strFailureReason ) );

	return Encode( &dict );
}

/*

===================
UTIL_DecodeHTTPPost
===================

[
  {
    disposition->{
                   key1->string (quotes stripped)
                   key2->string (quotes stripped)
                   key3->string (quotes stripped)
                   key4->string (quotes stripped)
                   ...
                 }
    data->string
  },

  {
    disposition->{
                   key1->string (quotes stripped)
                   key2->string (quotes stripped)
                   key3->string (quotes stripped)
                   key4->string (quotes stripped)
                   ...
                 }
    data->string
  }
]

*/

CAtomList *UTIL_DecodeHTTPPost( string strPost )
{
	// find the boundary

	string :: size_type iBoundary = strPost.find( "boundary=" );

	if( iBoundary == string :: npos )
		return NULL;

	iBoundary += strlen( "boundary=" );

	string strBoundary = strPost.substr( iBoundary );

	string :: size_type iBoundEnd = strBoundary.find( "\r\n" );

	if( iBoundEnd == string :: npos )
		return NULL;

	strBoundary = strBoundary.substr( 0, iBoundEnd );

	// strBoundary now contains the boundary

	string :: size_type iContent = strPost.find( "\r\n\r\n" );

	if( iContent == string :: npos )
		return NULL;

	iContent += strlen( "\r\n\r\n" );

	string strContent = strPost.substr( iContent );

	// decode

	CAtomList *pList = new CAtomList( );

	string :: size_type iSegStart = 0;
	string :: size_type iSegEnd = 0;

	while( 1 )
	{
		// segment start

		iSegStart = strContent.find( strBoundary, iSegStart );

		if( iSegStart == string :: npos )
			return pList;

		iSegStart += strBoundary.size( );

		if( strContent.substr( iSegStart, 2 ) == "--" )
			return pList;

		iSegStart += strlen( "\r\n" );

		// segment end

		iSegEnd = strContent.find( strBoundary, iSegStart );

		if( iSegEnd == string :: npos )
		{
			UTIL_LogPrint( "error decoding HTTP POST request - unexpected end of request\n" );

			delete pList;

			pList = NULL;

			return NULL;
		}

		iSegEnd -= strlen( "\r\n--" );

		// found segment

		CAtomDicti *pSegment = new CAtomDicti( );

		pList->addItem( pSegment );

		// this could do with some serious optimizing...

		string strSeg = strContent.substr( iSegStart, iSegEnd - iSegStart );

		string :: size_type iDispStart = strSeg.find( "Content-Disposition: " );

		if( iDispStart == string :: npos )
		{
			UTIL_LogPrint( "error decoding HTTP POST request - couldn't find Content-Disposition\n" );

			delete pList;

			pList = NULL;

			return NULL;
		}

		iDispStart += strlen( "Content-Disposition: " );

		string :: size_type iDispEnd = strSeg.find( "\r\n", iDispStart );

		if( iDispEnd == string :: npos )
		{
			UTIL_LogPrint( "error decoding HTTP POST request - malformed Content-Disposition\n" );

			delete pList;

			pList = NULL;

			return NULL;
		}

		string strDisp = strSeg.substr( iDispStart, iDispEnd - iDispStart );

		string :: size_type iDispPrev = 0;
		string :: size_type iDispPos = 0;

		CAtomDicti *pDisp = new CAtomDicti( );

		pSegment->setItem( "disposition", pDisp );

		while( 1 )
		{
			// assume a semicolon indicates the end of the item and will never appear inside the item (probably a bad assumption)

			iDispPrev = iDispPos;
			iDispPos = strDisp.find( ";", iDispPos );

			if( iDispPos == string :: npos )
			{
				// decode last item

				iDispPos = strDisp.size( );
			}

			string strCurr = strDisp.substr( iDispPrev, iDispPos - iDispPrev );

			string :: size_type iSplit = strCurr.find( "=" );

			if( iSplit == string :: npos )
			{
				// found a key without value, i.e. "form-data", useless so ignore it

				if( iDispPos == strDisp.size( ) )
					break;

				// + strlen( ";" )

				iDispPos++;

				continue;
			}

			// strip whitespace

			string :: size_type iKeyStart = strCurr.find_first_not_of( " " );

			if( iKeyStart == string :: npos || iKeyStart > iSplit )
			{
				UTIL_LogPrint( "error decoding HTTP POST request - malformed Content-Disposition\n" );

				delete pList;

				pList = NULL;

				return NULL;
			}

			string strKey = strCurr.substr( iKeyStart, iSplit - iKeyStart );
			string strValue = strCurr.substr( iSplit + 1 );

			// strip quotes

			if( strValue.size( ) > 1 && strValue[0] == '"' )
				strValue = strValue.substr( 1, strValue.size( ) - 2 );

			pDisp->setItem( strKey, new CAtomString( strValue ) );

			if( iDispPos == strDisp.size( ) )
				break;

			// + strlen( ";" )

			iDispPos++;
		}

		// data

		string :: size_type iDataStart = strSeg.find( "\r\n\r\n" );

		if( iDataStart == string :: npos )
		{
			UTIL_LogPrint( "error decoding HTTP POST request - malformed segment\n" );

			delete pList;

			pList = NULL;

			return NULL;
		}

		iDataStart += strlen( "\r\n\r\n" );

		pSegment->setItem( "data", new CAtomString( strSeg.substr( iDataStart ) ) );
	}

	// this should never happen, so who cares

	delete pList;

	pList = NULL;

	return NULL;
}
