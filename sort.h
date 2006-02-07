//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef SORT_H
 #define SORT_H

#define SORT_ANAME				1
#define SORT_ACOMPLETE			2
#define SORT_AINCOMPLETE		3
#define SORT_AADDED				4
#define SORT_ASIZE				5
#define SORT_AFILES				6
#define SORT_ACOMMENTS			7
#define SORT_AAVGLEFT			8
#define SORT_ACOMPLETED			9
#define SORT_ATRANSFERRED		10
#define SORT_DNAME				11
#define SORT_DCOMPLETE			12
#define SORT_DINCOMPLETE		13
#define SORT_DADDED				14
#define SORT_DSIZE				15
#define SORT_DFILES				16
#define SORT_DCOMMENTS			17
#define SORT_DAVGLEFT			18
#define SORT_DCOMPLETED			19
#define SORT_DTRANSFERRED		20

#define SORTP_AUPPED			1
#define SORTP_ADOWNED			2
#define SORTP_ALEFT				3
#define SORTP_ACONNECTED		4
#define SORTP_ASHARERATIO		5
#define SORTP_DUPPED			6
#define SORTP_DDOWNED			7
#define SORTP_DLEFT				8
#define SORTP_DCONNECTED		9
#define SORTP_DSHARERATIO		10

#define SORTU_ALOGIN			1
#define SORTU_AACCESS			2
#define SORTU_AEMAIL			3
#define SORTU_ACREATED			4
#define SORTU_DLOGIN			5
#define SORTU_DACCESS			6
#define SORTU_DEMAIL			7
#define SORTU_DCREATED			8

#define SORTSTR_ANAME			string( "1" )
#define SORTSTR_ACOMPLETE		string( "2" )
#define SORTSTR_AINCOMPLETE		string( "3" )
#define SORTSTR_AADDED			string( "4" )
#define SORTSTR_ASIZE			string( "5" )
#define SORTSTR_AFILES			string( "6" )
#define SORTSTR_ACOMMENTS		string( "7" )
#define SORTSTR_AAVGLEFT		string( "8" )
#define SORTSTR_ACOMPLETED		string( "9" )
#define SORTSTR_ATRANSFERRED	string( "10" )
#define SORTSTR_DNAME			string( "11" )
#define SORTSTR_DCOMPLETE		string( "12" )
#define SORTSTR_DINCOMPLETE		string( "13" )
#define SORTSTR_DADDED			string( "14" )
#define SORTSTR_DSIZE			string( "15" )
#define SORTSTR_DFILES			string( "16" )
#define SORTSTR_DCOMMENTS		string( "17" )
#define SORTSTR_DAVGLEFT		string( "18" )
#define SORTSTR_DCOMPLETED		string( "19" )
#define SORTSTR_DTRANSFERRED	string( "20" )

#define SORTPSTR_AUPPED			string( "1" )
#define SORTPSTR_ADOWNED		string( "2" )
#define SORTPSTR_ALEFT			string( "3" )
#define SORTPSTR_ACONNECTED		string( "4" )
#define SORTPSTR_ASHARERATIO	string( "5" )
#define SORTPSTR_DUPPED			string( "6" )
#define SORTPSTR_DDOWNED		string( "7" )
#define SORTPSTR_DLEFT			string( "8" )
#define SORTPSTR_DCONNECTED		string( "9" )
#define SORTPSTR_DSHARERATIO	string( "10" )

#define SORTUSTR_ALOGIN			string( "1" )
#define SORTUSTR_AACCESS		string( "2" )
#define SORTUSTR_AEMAIL			string( "3" )
#define SORTUSTR_ACREATED		string( "4" )
#define SORTUSTR_DLOGIN			string( "5" )
#define SORTUSTR_DACCESS		string( "6" )
#define SORTUSTR_DEMAIL			string( "7" )
#define SORTUSTR_DCREATED		string( "8" )

int asortByName( const void *elem1, const void *elem2 );
int asortByComplete( const void *elem1, const void *elem2 );
int asortByDL( const void *elem1, const void *elem2 );
int asortByAdded( const void *elem1, const void *elem2 );
int asortBySize( const void *elem1, const void *elem2 );
int asortByFiles( const void *elem1, const void *elem2 );
int asortByComments( const void *elem1, const void *elem2 );
int asortByAvgLeft( const void *elem1, const void *elem2 );
int asortByAvgLeftPercent( const void *elem1, const void *elem2 );
int asortByCompleted( const void *elem1, const void *elem2 );
int asortByTransferred( const void *elem1, const void *elem2 );
int dsortByName( const void *elem1, const void *elem2 );
int dsortByComplete( const void *elem1, const void *elem2 );
int dsortByDL( const void *elem1, const void *elem2 );
int dsortByAdded( const void *elem1, const void *elem2 );
int dsortBySize( const void *elem1, const void *elem2 );
int dsortByFiles( const void *elem1, const void *elem2 );
int dsortByComments( const void *elem1, const void *elem2 );
int dsortByAvgLeft( const void *elem1, const void *elem2 );
int dsortByAvgLeftPercent( const void *elem1, const void *elem2 );
int dsortByCompleted( const void *elem1, const void *elem2 );
int dsortByTransferred( const void *elem1, const void *elem2 );

int asortpByUpped( const void *elem1, const void *elem2 );
int asortpByDowned( const void *elem1, const void *elem2 );
int asortpByLeft( const void *elem1, const void *elem2 );
int asortpByConnected( const void *elem1, const void *elem2 );
int asortpByShareRatio( const void *elem1, const void *elem2 );
int dsortpByUpped( const void *elem1, const void *elem2 );
int dsortpByDowned( const void *elem1, const void *elem2 );
int dsortpByLeft( const void *elem1, const void *elem2 );
int dsortpByConnected( const void *elem1, const void *elem2 );
int dsortpByShareRatio( const void *elem1, const void *elem2 );

int asortuByLogin( const void *elem1, const void *elem2 );
int asortuByAccess( const void *elem1, const void *elem2 );
int asortuByMail( const void *elem1, const void *elem2 );
int asortuByCreated( const void *elem1, const void *elem2 );
int dsortuByLogin( const void *elem1, const void *elem2 );
int dsortuByAccess( const void *elem1, const void *elem2 );
int dsortuByMail( const void *elem1, const void *elem2 );
int dsortuByCreated( const void *elem1, const void *elem2 );

#endif
