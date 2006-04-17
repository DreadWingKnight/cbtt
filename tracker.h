//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef TRACKER_H
 #define TRACKER_H

struct announce_t
{
	string strInfoHash;
	string strIP;
	string strEvent;
	unsigned int iPort;
	int64 iUploaded;
	int64 iDownloaded;
	int64 iLeft;
	string strPeerID;
	string strKey;
	bool bAbusive;
	bool bIncrement;
};

struct torrent_t
{
	string strInfoHash;
	string strName;
	string strLowerName;
	int iComplete;
	int iDL;
	string strFileName;
	string strAdded;
	int64 iSize;
	int iFiles;
	int iComments;
	int64 iAverageLeft;
	int iAverageLeftPercent;
	int64 iMinLeft;
	int64 iMaxiLeft;
	string strTag;
	int iCompleted;
	int64 iTransferred;
	string strUploader;
	string strInfoLink;
};

struct peer_t
{
	string strIP;
	string strKey;
	int64 iUpped;
	int64 iDowned;
	int64 iLeft;
	unsigned long iConnected;
	float flShareRatio;
};

// user access levels

#define ACCESS_VIEW				( 1 << 0 )		// 1
#define ACCESS_DL				( 1 << 1 )		// 2
#define ACCESS_COMMENTS			( 1 << 2 )		// 4
#define ACCESS_UPLOAD			( 1 << 3 )		// 8
#define ACCESS_EDIT				( 1 << 4 )		// 16
#define ACCESS_ADMIN			( 1 << 5 )		// 32
#define ACCESS_SIGNUP			( 1 << 6 )		// 64

struct user_t
{
	string strLogin;
	string strLowerLogin;
	string strMD5;
	string strMail;
	string strLowerMail;
	string strCreated;
	int iAccess;
};

/*

#define ALW_FILENAME		0		// string
#define ALW_NAME			1		// string
#define ALW_ADDED			2		// string
#define ALW_SIZE			3		// long
#define ALW_FILES			4		// int
#define ALW_FILECOMMENT		5		// string

*/

#define MAX_FILENAME_LEN	128		// user specified filename on upload
#define MAX_INFOLINK_LEN	128		// user specified info link on upload

extern map<string, string> gmapMime;

class CTracker
{
public:
	CTracker( );
	virtual ~CTracker( );

	void saveDFile( );
	void saveScrapeFile( );
	void saveComments( );
	void saveTags( );
	void saveUsers( );
	void saveXML( );
	//addition by labarks
	void saveRSS( string strChannelTag = "" );
	void runSaveRSS( );
	//end addition
	void expireDownloaders( );
	void parseTorrents( const char *szDir );
	void parseTorrent( const char *szFile );
	bool checkTag( string &strTag );
	void addTag( string strInfoHash, string strTag, string strName, string strUploader, string strInfoLink );
	void deleteTag( string strInfoHash );
	user_t checkUser( string strLogin, string strMD5 );
	void addUser( string strLogin, string strPass, int iAccess, string strMail );
	void deleteUser( string strLogin );
	void CountUniquePeers( );
	void AddUniquePeer( string strIP );
	void RemoveUniquePeer( string strIP );
	void QueueAnnounce( struct announce_t ann );
	void Announce( struct announce_t ann );

	void RefreshFastCache( );

	// Client Ban List
	int isClientBanList( string strPeerID , bool isUserAgent = false );
	void parseClientBanList( void );
	// IP Ban List
	int isIPBanList( string strPeerIP );
	void parseIPBanList( void );

	void serverResponseGET( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponsePOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost, user_t user );
	void serverResponseIndex( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseAnnounce( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseScrape( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseStats( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseTorrent( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseFile( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseRobots( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseIcon( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseLogin( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseSignup( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseUploadGET( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseUploadPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost, user_t user );
	void serverResponseInfo( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseBencodeInfo( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseAdmin( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseUsers( struct request_t *pRequest, struct response_t *pResponse, user_t user );
	void serverResponseComments( struct request_t *pRequest, struct response_t *pResponse, user_t user );

	void Update( );

private:
	string m_strAllowedDir;
	string m_strUploadDir;
	string m_strExternalTorrentDir;
	string m_strArchiveDir;
	string m_strFileDir;
	string m_strDFile;
	// Scrape Stats
	string m_strSCFile;
	string m_strCommentsFile;
	string m_strTagFile;
	string m_strUsersFile;
	string m_strStaticHeaderFile;
	string m_strStaticHeader;
	string m_strStaticFooterFile;
	string m_strStaticFooter;
	string m_strRobotsFile;
	string m_strRobots;
	string m_strDumpXMLFile;
	//RSS Support - code by labarks
	string m_strDumpRSSFile;
	string m_strDumpRSSFileDir;
	int m_iDumpRSSFileMode;
	string m_strDumpRSSTitle;
	string m_strDumpRSSLink;
	string m_strDumpRSSDescription;
	int m_iDumpRSS_TTL;
	string m_strDumpRSSLanguage;
	string m_strDumpRSSImageURL;
	int m_iDumpRSSImageWidth;
	int m_iDumpRSSImageHeight;
	string m_strDumpRSSCopyright;
	int m_iDumpRSSLimit;
	int m_iDumpRSSInterval;
	unsigned long m_iDumpRSSNext;
	//end addition

	string m_strImageBarFill;
	string m_strImageBarTrans;
	string m_strForceAnnounceURL;
	bool m_bForceAnnounceOnDL;
	bool m_bDumpXMLPeers;
	int m_iParseAllowedInterval;
	int m_iSaveDFileInterval;
	int m_iSaveScrapeInterval;
	int m_iDownloaderTimeOutInterval;
	int m_iRefreshStaticInterval;
	int m_iDumpXMLInterval;
	int m_iMySQLRefreshAllowedInterval;
	int m_iMySQLRefreshStatsInterval;
	unsigned long m_iParseAllowedNext;
	unsigned long m_iSaveDFileNext;
	unsigned long m_iSaveScrapeNext;
	unsigned long m_iPrevTime;
	unsigned long m_iDownloaderTimeOutNext;
	unsigned long m_iRefreshStaticNext;
	unsigned long m_iDumpXMLNext;
	unsigned long m_iMySQLRefreshAllowedNext;
	unsigned long m_iMySQLRefreshStatsNext;
	int m_iAnnounceInterval;
	int m_iMinRequestInterval;
	int m_iResponseSize;
	int m_iMaxGive;
	bool m_bKeepDead;
	bool m_bAllowScrape;
	bool m_bCountUniquePeers;
	bool m_bDeleteInvalid;
	bool m_bParseOnUpload;
	int m_iMaxTorrents;
	bool m_bShowInfoHash;
	bool m_bShowNames;
	bool m_bShowStats;
	bool m_bAllowTorrentDownloads;
	bool m_bAllowComments;
	bool m_bShowAdded;
	bool m_bShowSize;
	bool m_bShowNumFiles;
	bool m_bShowCompleted;
	bool m_bShowTransferred;
	bool m_bShowMinLeft;
	bool m_bShowAverageLeft;
	bool m_bShowMaxiLeft;
	bool m_bShowLeftAsProgress;
	bool m_bShowUploader;
	bool m_bAllowInfoLink;
	bool m_bSearch;
	bool m_bSort;
	bool m_bShowFileComment;
	bool m_bShowFileContents;
	bool m_bShowShareRatios;
	bool m_bShowAvgDLRate;
	bool m_bShowAvgULRate;
	bool m_bDeleteOwnTorrents;
	bool m_bGen;
	bool m_bMySQLOverrideDState;
	int m_iPerPage;
	int m_iUsersPerPage;
	int m_iMaxPeersDisplay;
	int m_iGuestAccess;
	int m_iMemberAccess;
	int m_iFileExpires;
	int m_iNameLength;
	int m_iCommentLength;

	string m_strClientBanFile;
	int i_intBanMode;
	int i_intPeerSpoofRestrict;
	int m_iDontCompressTorrent;
	string strOverFlowLimit;
	int m_iRestrictOverflow;
	int m_iIPBanMode;
	string m_strIPBanFile;
	int m_iBlockNATedIP;
	int m_iLocalOnly;
	int m_iTorrentTraderCompatibility;
	int m_iBlacklistP2PPorts;
	int m_iBlackListServicePorts;
	int m_iPrivateTracker;
	bool m_bSwapTorrentLink;
	string m_strDownloadLinkImage;
	string m_strStatsLinkImage;
	bool m_bAllowGlobalScrape;
	// Announce 'key' support
	bool m_bAnnounceKeySupport;
	bool m_bRequireKeySupport;
	// Abusive Announce Code
	bool m_bEnableAbuseBlock;
	int m_iMinAnnounceInterval;
	int m_iGlobalAbuseHammer;
	int m_iGlobalAbuseLimit;

	bool m_bRequireNoPeerID;
	bool m_bRequireCompact;

	string m_strECommand;
	int m_iECommandCycle;
	unsigned long m_iNextCommandCycle;
	bool m_bEnableExternal;

	unsigned int m_iPageRange;

	int m_iRefreshFastCacheInterval;
	unsigned long m_iRefreshFastCacheNext;


	bool m_bDisableHTML;

	string gstrTrackerTitle;

	string m_strIconFile;
	string m_strFavicon;

	vector< pair<string, string> > m_vecTags;

	// queued announces

	CMutex m_mtxQueued;

	vector<struct announce_t> m_vecQueued;

	CAtomDicti *m_pAllowed;		// self.allowed
	CAtomDicti *m_pState;		// self.state
	CAtomDicti *m_pDFile;		// self.downloads
	CAtomDicti *m_pCompleted;	// self.completed
	CAtomDicti *m_pAbuse;	// self.abuse - Abuse Log
	CAtomDicti *m_pTimeDicti;	// self.times
	CAtomDicti *m_pCached;		// self.cached
	CAtomDicti *m_pComments;
	CAtomDicti *m_pTags;
	CAtomDicti *m_pUsers;
	CAtomDicti *m_pIPs;
	CAtomDicti *m_pFastCache;	// FastCache for per-torrent Totals

	CAtomList *m_pClientBannedList;   // banned Clients
	CAtomList *m_pIPBannedList;   // banned IPs
};

#endif
