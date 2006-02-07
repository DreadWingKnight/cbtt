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
#include "util.h"

string EncodeInt( const CAtomInt &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( char ) * 128 );

	sprintf( pBuf, "%d", x.getValue( ) );

	string strDest;

	strDest += "i";
	strDest += pBuf;
	strDest += "e";

	return strDest;
}

string EncodeLong( const CAtomLong &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( char ) * 128 );

#if defined( WIN32 )
	sprintf( pBuf, "%I64d", x.getValue( ) );
#elif defined( __FREEBSD__ )
	sprintf( pBuf, "%qd", x.getValue( ) );
#else
	sprintf( pBuf, "%lld", x.getValue( ) );
#endif

	string strDest;

	strDest += "i";
	strDest += pBuf;
	strDest += "e";

	return strDest;
}

string EncodeString( const CAtomString &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( char ) * 128 );

	sprintf( pBuf, "%u", (unsigned int)x.getValue( ).size( ) );

	string strDest;

	strDest += pBuf;
	strDest += ":";
	strDest += x.getValue( );

	return strDest;
}

string EncodeList( const CAtomList &x )
{
	vector<CAtom *> *pv = x.getValuePtr( );

	string strDest;

	strDest += "l";

	for( vector<CAtom *> :: iterator i = pv->begin( ); i != pv->end( ); i++ )
	{
		if( dynamic_cast<CAtomInt *>( *i ) )
			strDest += EncodeInt( *dynamic_cast<CAtomInt *>( *i ) );
		else if( dynamic_cast<CAtomLong *>( *i ) )
			strDest += EncodeLong( *dynamic_cast<CAtomLong *>( *i ) );
		else if( dynamic_cast<CAtomString *>( *i ) )
			strDest += EncodeString( *dynamic_cast<CAtomString *>( *i ) );
		else if( dynamic_cast<CAtomList *>( *i ) )
			strDest += EncodeList( *dynamic_cast<CAtomList *>( *i ) );
		else if( dynamic_cast<CAtomDicti *>( *i ) )
			strDest += EncodeDicti( *dynamic_cast<CAtomDicti *>( *i ) );
	}

	strDest += "e";

	return strDest;
}

string EncodeDicti( const CAtomDicti &x )
{
	map<string, CAtom *> *pmapDicti = x.getValuePtr( );

	string strDest;

	strDest += "d";

	for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
	{
		strDest += EncodeString( CAtomString( (*i).first ) );

		if( dynamic_cast<CAtomInt *>( (*i).second ) )
			strDest += EncodeInt( *dynamic_cast<CAtomInt *>( (*i).second ) );
		else if( dynamic_cast<CAtomLong *>( (*i).second ) )
			strDest += EncodeLong( *dynamic_cast<CAtomLong *>( (*i).second ) );
		else if( dynamic_cast<CAtomString *>( (*i).second ) )
			strDest += EncodeString( *dynamic_cast<CAtomString *>( (*i).second ) );
		else if( dynamic_cast<CAtomList *>( (*i).second ) )
			strDest += EncodeList( *dynamic_cast<CAtomList *>( (*i).second ) );
		else if( dynamic_cast<CAtomDicti *>( (*i).second ) )
			strDest += EncodeDicti( *dynamic_cast<CAtomDicti *>( (*i).second ) );
	}

	strDest += "e";

	return strDest;
}

string Encode( CAtom *pAtom )
{
	if( dynamic_cast<CAtomInt *>( pAtom ) )
		return EncodeInt( *dynamic_cast<CAtomInt *>( pAtom ) );
	else if( dynamic_cast<CAtomLong *>( pAtom ) )
		return EncodeLong( *dynamic_cast<CAtomLong *>( pAtom ) );
	else if( dynamic_cast<CAtomString *>( pAtom ) )
		return EncodeString( *dynamic_cast<CAtomString *>( pAtom ) );
	else if( dynamic_cast<CAtomList *>( pAtom ) )
		return EncodeList( *dynamic_cast<CAtomList *>( pAtom ) );
	else if( dynamic_cast<CAtomDicti *>( pAtom ) )
		return EncodeDicti( *dynamic_cast<CAtomDicti *>( pAtom ) );

	return string( );
}

/*

CAtomInt *DecodeInt( const string &x, unsigned long iStart )
{
	string :: size_type iEnd = x.find( "e" );

	if( iEnd == string :: npos )
	{
		UTIL_LogPrint( "error decoding int - couldn't find \"e\", halting decode\n" );

		return NULL;
	}

	return new CAtomInt( atoi( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ) ) );
}

*/

CAtomLong *DecodeLong( const string &x, unsigned long iStart )
{
	string :: size_type iEnd = x.find( "e", iStart );

	if( iEnd == string :: npos )
	{
		UTIL_LogPrint( "error decoding long - couldn't find \"e\", halting decode\n" );

		return NULL;
	}

	int64 i;

#if defined( WIN32 )
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%I64d", &i );
#elif defined( __FREEBSD__ )
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%qd", &i );
#else
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%lld", &i );
#endif

	return new CAtomLong( i );
}

CAtomString *DecodeString( const string &x, unsigned long iStart )
{
	string :: size_type iSplit = x.find_first_not_of( "1234567890", iStart );

	if( iSplit == string :: npos )
	{
		UTIL_LogPrint( "error decoding string - couldn't find \":\", halting decode\n" );

		return NULL;
	}

	return new CAtomString( x.substr( iSplit + 1, atoi( x.substr( iStart, iSplit - iStart ).c_str( ) ) ) );
}

CAtomList *DecodeList( const string &x, unsigned long iStart )
{
	unsigned long i = iStart + 1;

	CAtomList *pList = new CAtomList( );

	while( i < x.size( ) && x[i] != 'e' )
	{
		CAtom *pAtom = Decode( x, i );

		if( pAtom )
		{
			i += pAtom->EncodedLength( );

			pList->addItem( pAtom );
		}
		else
		{
			UTIL_LogPrint( "error decoding list - error decoding list item, discarding list\n" );

			delete pList;

			return NULL;
		}
	}

	return pList;
}

CAtomDicti *DecodeDicti( const string &x, unsigned long iStart )
{
	unsigned long i = iStart + 1;

	CAtomDicti *pDicti = new CAtomDicti( );

	while( i < x.size( ) && x[i] != 'e' )
	{
		CAtom *pKey = Decode( x, i );

		if( pKey && dynamic_cast<CAtomString *>( pKey ) )
		{
			i += pKey->EncodedLength( );

			string strKey = pKey->toString( );

			delete pKey;

			if( i < x.size( ) )
			{
				CAtom *pValue = Decode( x, i );

				if( pValue )
				{
					i += pValue->EncodedLength( );

					pDicti->setItem( strKey, pValue );
				}
				else
				{
					UTIL_LogPrint( "error decoding dictionary - error decoding value, discarding dictionary\n" );

					delete pDicti;

					return NULL;
				}
			}
		}
		else
		{
			UTIL_LogPrint( "error decoding dictionary - error decoding key, discarding dictionary\n" );

			delete pDicti;

			return NULL;
		}
	}

	return pDicti;
}

CAtom *Decode( const string &x, unsigned long iStart )
{
	if( iStart < x.size( ) )
	{
		if( x[iStart] == 'i' )
			return DecodeLong( x, iStart );
		else if( isdigit( x[iStart] ) )
			return DecodeString( x, iStart );
		else if( x[iStart] == 'l' )
			return DecodeList( x, iStart );
		else if( x[iStart] == 'd' )
			return DecodeDicti( x, iStart );

		string temp = x.substr( iStart );

		UTIL_LogPrint( "error decoding - found unexpected character %u, halting decode\n", (unsigned char)x[iStart] );
	}
	else
		UTIL_LogPrint( "error decoding - out of range\n" );

	return NULL;
}

CAtom *DecodeFile( const char *szFile )
{
	FILE *pFile = NULL;

	if( ( pFile = fopen( szFile, "rb" ) ) == NULL )
	{
		UTIL_LogPrint( "warning - unable to open %s for reading\n", szFile );

		return NULL;
	}

	fseek( pFile, 0, SEEK_END );
	unsigned long ulFileSize = ftell( pFile );
	fseek( pFile, 0, SEEK_SET );
	char *pData = (char *)malloc( sizeof( char ) * ulFileSize );
	memset( pData, 0, sizeof( char ) * ulFileSize );
	fread( (void *)pData, sizeof( char ), ulFileSize, pFile );
	fclose( pFile );
	string strFile( pData, ulFileSize );
	free( pData );

	return Decode( strFile );
}
