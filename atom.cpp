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
#include "util.h"

//
// CAtomInt
//

CAtomInt :: CAtomInt( )
{
	setValue( 0 );
}

CAtomInt :: CAtomInt( int iInt )
{
	setValue( iInt );
}

CAtomInt :: CAtomInt( const CAtomInt &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomInt :: ~CAtomInt( )
{

}

int CAtomInt :: EncodedLength( )
{
	return toString( ).size( ) + 2;
}

int CAtomInt :: Length( )
{
	return toString( ).size( );
}

string CAtomInt :: toString( )
{
	char pBuf[32];
	memset( pBuf, 0, sizeof( char ) * 32 );
	sprintf( pBuf, "%d", getValue( ) );
	return pBuf;
}

int CAtomInt :: getValue( ) const
{
	return m_iInt;
}

void CAtomInt :: setValue( int iInt )
{
	m_iInt = iInt;
}

//
// CAtomLong
//

CAtomLong :: CAtomLong( )
{
	setValue( 0 );
}

CAtomLong :: CAtomLong( int64 iLong )
{
	setValue( iLong );
}

CAtomLong :: CAtomLong( const CAtomLong &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomLong :: ~CAtomLong( )
{

}

int CAtomLong :: EncodedLength( )
{
	return toString( ).size( ) + 2;
}

int CAtomLong :: Length( )
{
	return toString( ).size( );
}

string CAtomLong :: toString( )
{
	char pBuf[32];
	memset( pBuf, 0, sizeof( char ) * 32 );

#if defined( WIN32 )
	sprintf( pBuf, "%I64d", getValue( ) );
#elif defined( __FREEBSD__ )
	sprintf( pBuf, "%qd", getValue( ) );
#else
	sprintf( pBuf, "%lld", getValue( ) );
#endif

	return pBuf;
}

int64 CAtomLong :: getValue( ) const
{
	return m_iLong;
}

void CAtomLong :: setValue( int64 iLong )
{
	m_iLong = iLong;
}

//
// CAtomString
//

CAtomString :: CAtomString( )
{

}

CAtomString :: CAtomString( string strString )
{
	setValue( strString );
}

CAtomString :: CAtomString( const CAtomString &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomString :: ~CAtomString( )
{

}

int CAtomString :: EncodedLength( )
{
	int iSize = getValue( ).size( );
	char pBuf[32];
	memset( pBuf, 0, sizeof( char ) * 32 );
	sprintf( pBuf, "%d", iSize );
	return iSize + strlen( pBuf ) + 1;
}

int CAtomString :: Length( )
{
	return getValue( ).size( );
}

string CAtomString :: toString( )
{
	return getValue( );
}

string CAtomString :: getValue( ) const
{
	return m_strString;
}

void CAtomString :: setValue( string strString )
{
	m_strString = strString;
}

//
// CAtomList
//

CAtomList :: CAtomList( )
{

}

CAtomList :: CAtomList( vector<CAtom *> vecList )
{
	setValue( vecList );
}

CAtomList :: CAtomList( const CAtomList &c )
{
	// copy constructor

	vector<CAtom *> *pvecList = c.getValuePtr( );

	for( vector<CAtom *> :: iterator i = pvecList->begin( ); i != pvecList->end( ); i++ )
	{
		if( dynamic_cast<CAtomInt *>( *i ) )
			addItem( new CAtomInt( *dynamic_cast<CAtomInt *>( *i ) ) );
		else if( dynamic_cast<CAtomLong *>( *i ) )
			addItem( new CAtomLong( *dynamic_cast<CAtomLong *>( *i ) ) );
		else if( dynamic_cast<CAtomString *>( *i ) )
			addItem( new CAtomString( *dynamic_cast<CAtomString *>( *i ) ) );
		else if( dynamic_cast<CAtomList *>( *i ) )
			addItem( new CAtomList( *dynamic_cast<CAtomList *>( *i ) ) );
		else if( dynamic_cast<CAtomDicti *>( *i ) )
			addItem( new CAtomDicti( *dynamic_cast<CAtomDicti *>( *i ) ) );
		else
			UTIL_LogPrint( "error copying list - found invalid atom, ignoring\n" );
	}
}

CAtomList :: ~CAtomList( )
{
	clear( );
}

int CAtomList :: EncodedLength( )
{
	int iLen = 0;

	for( vector<CAtom *> :: iterator i = m_vecList.begin( ); i != m_vecList.end( ); i++ )
		iLen += (*i)->EncodedLength( );

	return iLen + 2;
}

int CAtomList :: Length( )
{
	// nobody cares about you

	return 0;
}

string CAtomList :: toString( )
{
	return string( );
}

bool CAtomList :: isEmpty( )
{
	return m_vecList.empty( );
}

void CAtomList :: clear( )
{
	for( vector<CAtom *> :: iterator i = m_vecList.begin( ); i != m_vecList.end( ); i++ )
		delete *i;

	m_vecList.clear( );
}

void CAtomList :: Randomize( )
{
	random_shuffle( m_vecList.begin( ), m_vecList.end( ) );
}

vector<CAtom *> CAtomList :: getValue( ) const
{
	return m_vecList;
}

vector<CAtom *> *CAtomList :: getValuePtr( ) const
{
	return (vector<CAtom *> *)&m_vecList;
}

void CAtomList :: setValue( vector<CAtom *> vecList )
{
	m_vecList = vecList;
}

void CAtomList :: delItem( CAtom *atmItem )
{
	for( vector<CAtom *> :: iterator i = m_vecList.begin( ); i != m_vecList.end( ); i++ )
	{
		if( *i == atmItem )
		{
			delete *i;

			m_vecList.erase( i );

			return;
		}
	}
}

void CAtomList :: addItem( CAtom *atmItem )
{
	m_vecList.push_back( atmItem );
}

//
// CAtomDicti
//

CAtomDicti :: CAtomDicti( )
{

}

CAtomDicti :: CAtomDicti( const CAtomDicti &c )
{
	// copy constructor

	map<string, CAtom *> *pmapDicti = c.getValuePtr( );

	for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
	{
		if( dynamic_cast<CAtomInt *>( (*i).second ) )
			setItem( (*i).first, new CAtomInt( *dynamic_cast<CAtomInt *>( (*i).second ) ) );
		else if( dynamic_cast<CAtomLong *>( (*i).second ) )
			setItem( (*i).first, new CAtomLong( *dynamic_cast<CAtomLong *>( (*i).second ) ) );
		else if( dynamic_cast<CAtomString *>( (*i).second ) )
			setItem( (*i).first, new CAtomString( *dynamic_cast<CAtomString *>( (*i).second ) ) );
		else if( dynamic_cast<CAtomList *>( (*i).second ) )
			setItem( (*i).first, new CAtomList( *dynamic_cast<CAtomList *>( (*i).second ) ) );
		else if( dynamic_cast<CAtomDicti *>( (*i).second ) )
			setItem( (*i).first, new CAtomDicti( *dynamic_cast<CAtomDicti *>( (*i).second ) ) );
		else
			UTIL_LogPrint( "error copying dictionary - found invalid atom, ignoring\n" );
	}
}

CAtomDicti :: ~CAtomDicti( )
{
	clear( );
}

int CAtomDicti :: EncodedLength( )
{
	int iLen = 0;

	for( map<string, CAtom *> :: iterator i = m_mapDicti.begin( ); i != m_mapDicti.end( ); i++ )
		iLen += CAtomString( (*i).first ).EncodedLength( ) + (*i).second->EncodedLength( );

	return iLen + 2;
}

int CAtomDicti :: Length( )
{
	// nobody cares about you

	return 0;
}

string CAtomDicti :: toString( )
{
	return string( );
}

bool CAtomDicti :: isEmpty( )
{
	return m_mapDicti.empty( );
}

void CAtomDicti :: clear( )
{
	for( map<string, CAtom *> :: iterator i = m_mapDicti.begin( ); i != m_mapDicti.end( ); i++ )
	{
		// UTIL_LogPrint( "deleting element %s\n", (*i).first.c_str( ) );

		delete (*i).second;
	}

	m_mapDicti.clear( );
}

map<string, CAtom *> *CAtomDicti :: getValuePtr( ) const
{
	return (map<string, CAtom *> *)&m_mapDicti;
}

void CAtomDicti :: delItem( string strKey )
{
	map<string, CAtom *> :: iterator i = m_mapDicti.find( strKey );

	if( i != m_mapDicti.end( ) )
	{
		delete (*i).second;

		m_mapDicti.erase( i );
	}
}

CAtom *CAtomDicti :: getItem( string strKey )
{
	map<string, CAtom *> :: iterator i = m_mapDicti.find( strKey );

	if( i == m_mapDicti.end( ) )
		return NULL;
	else
		return (*i).second;
}

CAtom *CAtomDicti :: getItem( string strKey, CAtom *pReturn )
{
	map<string, CAtom *> :: iterator i = m_mapDicti.find( strKey );

	if( i == m_mapDicti.end( ) )
		return pReturn;
	else
		return (*i).second;
}

void CAtomDicti :: setItem( string strKey, CAtom *pValue )
{
	map<string, CAtom *> :: iterator i = m_mapDicti.find( strKey );

	if( i == m_mapDicti.end( ) )
		m_mapDicti.insert( pair<string, CAtom *>( strKey, pValue ) );
	else
	{
		delete (*i).second;

		(*i).second = pValue;
	}
}
