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
#include "sort.h"
#include "tracker.h"
#include "util.h"

int asortByName( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->strLowerName.compare( ( (struct torrent_t *)elem2 )->strLowerName );
}

int asortByComplete( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iComplete - ( (struct torrent_t *)elem2 )->iComplete;
}

int asortByDL( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iDL - ( (struct torrent_t *)elem2 )->iDL;
}

int asortByAdded( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->strAdded.compare( ( (struct torrent_t *)elem2 )->strAdded );
}

int asortBySize( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iSize < tor2->iSize )
		return -1;
	else if( tor1->iSize > tor2->iSize )
		return 1;
	else
		return 0;
}

int asortByFiles( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iFiles - ( (struct torrent_t *)elem2 )->iFiles;
}

int asortByComments( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iComments - ( (struct torrent_t *)elem2 )->iComments;
}

int asortByAvgLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iAverageLeft < tor2->iAverageLeft )
		return -1;
	else if( tor1->iAverageLeft > tor2->iAverageLeft )
		return 1;
	else
		return 0;
}

int asortByAvgLeftPercent( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iAverageLeftPercent - ( (struct torrent_t *)elem2 )->iAverageLeftPercent;
}

int asortByCompleted( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem1 )->iCompleted - ( (struct torrent_t *)elem2 )->iCompleted;
}

int asortByTransferred( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iTransferred < tor2->iTransferred )
		return -1;
	else if( tor1->iTransferred > tor2->iTransferred )
		return 1;
	else
		return 0;
}

int dsortByName( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->strLowerName.compare( ( (struct torrent_t *)elem1 )->strLowerName );
}

int dsortByComplete( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iComplete - ( (struct torrent_t *)elem1 )->iComplete;
}

int dsortByDL( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iDL - ( (struct torrent_t *)elem1 )->iDL;
}

int dsortByAdded( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->strAdded.compare( ( (struct torrent_t *)elem1 )->strAdded );
}

int dsortBySize( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iSize < tor2->iSize )
		return 1;
	else if( tor1->iSize > tor2->iSize )
		return -1;
	else
		return 0;
}

int dsortByFiles( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iFiles - ( (struct torrent_t *)elem1 )->iFiles;
}

int dsortByComments( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iComments - ( (struct torrent_t *)elem1 )->iComments;
}

int dsortByAvgLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iAverageLeft < tor2->iAverageLeft )
		return 1;
	else if( tor1->iAverageLeft > tor2->iAverageLeft )
		return -1;
	else
		return 0;
}

int dsortByAvgLeftPercent( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iAverageLeftPercent - ( (struct torrent_t *)elem1 )->iAverageLeftPercent;
}

int dsortByCompleted( const void *elem1, const void *elem2 )
{
	return ( (struct torrent_t *)elem2 )->iCompleted - ( (struct torrent_t *)elem1 )->iCompleted;
}

int dsortByTransferred( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct torrent_t *tor1 = (struct torrent_t *)elem1;
	struct torrent_t *tor2 = (struct torrent_t *)elem2;

	if( tor1->iTransferred < tor2->iTransferred )
		return 1;
	else if( tor1->iTransferred > tor2->iTransferred )
		return -1;
	else
		return 0;
}

int asortpByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iUpped < peer2->iUpped )
		return -1;
	else if( peer1->iUpped > peer2->iUpped )
		return 1;
	else
		return 0;
}

int asortpByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iDowned < peer2->iDowned )
		return -1;
	else if( peer1->iDowned > peer2->iDowned )
		return 1;
	else
		return 0;
}

int asortpByLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iLeft < peer2->iLeft )
		return -1;
	else if( peer1->iLeft > peer2->iLeft )
		return 1;
	else
		return 0;
}

int asortpByConnected( const void *elem1, const void *elem2 )
{
	return ( (struct peer_t *)elem1 )->iConnected - ( (struct peer_t *)elem2 )->iConnected;
}

int asortpByShareRatio( const void *elem1, const void *elem2 )
{
	float fl1 = ( (struct peer_t *)elem1 )->flShareRatio;
	float fl2 = ( (struct peer_t *)elem2 )->flShareRatio;

	// this is complicated because -1 means infinite and casting to ints won't work

	if( fl1 == fl2 )
		return 0;
	else if( fl1 == -1.0 )
		return 1;
	else if( fl2 == -1.0 )
		return -1;
	else if( fl1 < fl2 )
		return -1;
	else
		return 1;
}

int dsortpByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iUpped < peer2->iUpped )
		return 1;
	else if( peer1->iUpped > peer2->iUpped )
		return -1;
	else
		return 0;
}

int dsortpByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iDowned < peer2->iDowned )
		return 1;
	else if( peer1->iDowned > peer2->iDowned )
		return -1;
	else
		return 0;
}

int dsortpByLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	struct peer_t *peer1 = (struct peer_t *)elem1;
	struct peer_t *peer2 = (struct peer_t *)elem2;

	if( peer1->iLeft < peer2->iLeft )
		return 1;
	else if( peer1->iLeft > peer2->iLeft )
		return -1;
	else
		return 0;
}

int dsortpByConnected( const void *elem1, const void *elem2 )
{
	return ( (struct peer_t *)elem2 )->iConnected - ( (struct peer_t *)elem1 )->iConnected;
}

int dsortpByShareRatio( const void *elem1, const void *elem2 )
{
	float fl1 = ( (struct peer_t *)elem1 )->flShareRatio;
	float fl2 = ( (struct peer_t *)elem2 )->flShareRatio;

	// this is complicated because -1 means infinite and casting to ints won't work

	if( fl1 == fl2 )
		return 0;
	else if( fl1 == -1.0 )
		return -1;
	else if( fl2 == -1.0 )
		return 1;
	else if( fl1 < fl2 )
		return 1;
	else
		return -1;
}

int asortuByLogin( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem1 )->strLowerLogin.compare( ( (struct user_t *)elem2 )->strLowerLogin );
}

int asortuByAccess( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem2 )->iAccess - ( (struct user_t *)elem1 )->iAccess;
}

int asortuByMail( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem1 )->strLowerMail.compare( ( (struct user_t *)elem2 )->strLowerMail );
}

int asortuByCreated( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem1 )->strCreated.compare( ( (struct user_t *)elem2 )->strCreated );
}

int dsortuByLogin( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem2 )->strLowerLogin.compare( ( (struct user_t *)elem1 )->strLowerLogin );
}

int dsortuByAccess( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem1 )->iAccess - ( (struct user_t *)elem2 )->iAccess;
}

int dsortuByMail( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem2 )->strLowerMail.compare( ( (struct user_t *)elem1 )->strLowerMail );
}

int dsortuByCreated( const void *elem1, const void *elem2 )
{
	return ( (struct user_t *)elem2 )->strCreated.compare( ( (struct user_t *)elem1 )->strCreated );
}
