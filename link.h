//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef LINK_H
 #define LINK_H

#define LINK_VER			"TrackerLINK Ver. 0.1"

#define LINKMSG_ERROR		-1
#define LINKMSG_NONE		0		// not transmitted
#define LINKMSG_VERSION		1
#define LINKMSG_INFO		2
#define LINKMSG_PASSWORD	3
#define LINKMSG_READY		4
#define LINKMSG_ANNOUNCE	7
#define LINKMSG_CLOSE		99

struct linkmsg_t
{
	long len;
	int type;
	string msg;
};

//
// CLink
//  - one instance created on the secondary tracker to connect to the primary tracker
//

class CLink
{
public:
	CLink( );
	virtual ~CLink( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

private:
	bool m_bKill;

	string m_strIP;
	string m_strPass;

	SOCKET m_sckLink;

	struct sockaddr_in sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartLink( );

//
// CLinkClient
//  - one instance created on the primary tracker for each secondary tracker
//

class CLinkClient
{
public:
	CLinkClient( SOCKET sckLink, struct sockaddr_in sinAddress );
	virtual ~CLinkClient( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

	bool m_bActive;

private:
	bool m_bKill;

	SOCKET m_sckLink;

	struct sockaddr_in sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartLinkClient( CLinkClient *pLinkClient );

//
// CLinkServer
//  - one instance created on the primary tracker
//

class CLinkServer
{
public:
	CLinkServer( );
	virtual ~CLinkServer( );

	void Update( );

	void Queue( struct linkmsg_t lm );
	void Queue( struct linkmsg_t lm, string strExclude );

	string m_strPass;

	CMutex m_mtxLinks;

	vector<CLinkClient *> m_vecLinks;

private:
	string m_strBind;

	SOCKET m_sckLinkServer;
};

#endif
