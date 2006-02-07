//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef CLIENT_H
 #define CLIENT_H

#define COMPRESS_NONE		0
#define COMPRESS_DEFLATE	1
#define COMPRESS_GZIP		2

#define CS_RECEIVING		0
#define CS_WAITING1			1
#define CS_PROCESSING		2
#define CS_WAITING2			3
#define CS_SENDING			4
#define CS_DEAD				5

class CClient
{
public:
	CClient( SOCKET sckClient, struct sockaddr_in sinAddress, struct timeval tvTimeOut, int iCompression );
	virtual ~CClient( );

	void StartReceiving( );
	void Process( );
	void StartSending( );

	int m_iState;

private:
	SOCKET m_sckClient;

	struct timeval m_tvTimeOut;
	int m_iCompression;
	string m_strReceiveBuf;
	struct request_t rqst;
	struct response_t rsp;
	bool m_bKeepAlive;
};

void StartReceiving( CClient *pClient );
void StartSending( CClient *pClient );

#endif
