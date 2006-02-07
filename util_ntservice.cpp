/***
*
* BNBT Beta 8.0 - A C++ BitTorrent Tracker
* Copyright (C) 2003 Trevor Hogan
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

/***********************************************************************
* NT Service code written by ConfusedFish and modified by Trevor Hogan *
***********************************************************************/

#include "bnbt.h"
#include "server.h"
#include "util_ntservice.h"
#include "util.h"
#include "config.h"


BOOL UTIL_NTServiceTest( )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );

		// test if the service is installed or not

	BOOL bResult = FALSE;

	// open the Service Control Manager

	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if( hSCM )
	{
		// try to open the service

		SC_HANDLE hService = OpenService( hSCM, BNBT_SERVICE_NAME, SERVICE_QUERY_CONFIG );

		if( hService )
		{
			bResult = TRUE;

			CloseServiceHandle( hService );
		}

		CloseServiceHandle( hSCM );
	}

	return bResult;
}

BOOL UTIL_NTServiceInstall( )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	// open the Service Control Manager

	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if( !hSCM )
		return FALSE;

	// get the executable file path

	char szFilePath[_MAX_PATH];

	GetModuleFileName( NULL, szFilePath, sizeof( szFilePath ) );

	// add the run as service parameter

	strcat( szFilePath, " -s" );

	// create the service

	SC_HANDLE hService = CreateService(	hSCM,
										BNBT_SERVICE_NAME,
										BNBT_SERVICE_NAME,
										SERVICE_ALL_ACCESS,
										SERVICE_WIN32_OWN_PROCESS,
										SERVICE_AUTO_START,
										SERVICE_ERROR_NORMAL,
										szFilePath,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL );

	if( !hService )
	{
		CloseServiceHandle( hSCM );

		return FALSE;
	}

	// make registry entries to support logging messages to the EventLog

	char szKey[256];

	HKEY hKey = NULL;

	strcpy( szKey, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" );

	strcat( szKey, BNBT_SERVICE_NAME );

	if( RegCreateKey( HKEY_LOCAL_MACHINE, szKey, &hKey ) != ERROR_SUCCESS )
	{
		CloseServiceHandle( hService );
		CloseServiceHandle( hSCM );

		return FALSE;
	}

	RegSetValueEx( hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (CONST BYTE *)szFilePath, strlen( szFilePath ) + 1 );

	// set the supported types flags

	DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

	RegSetValueEx( hKey, "TypesSupported", 0, REG_DWORD, (CONST BYTE *)&dwData, sizeof( DWORD ) );

	RegCloseKey( hKey );

	UTIL_NTLogEvent( EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, BNBT_SERVICE_NAME );

	// tidy up

	CloseServiceHandle( hService );
	CloseServiceHandle( hSCM );

	return TRUE;
}

BOOL UTIL_NTServiceUninstall( )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	// open the Service Control Manager

	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if( !hSCM )
		return FALSE;

	BOOL bResult = FALSE;

	SC_HANDLE hService = OpenService( hSCM, BNBT_SERVICE_NAME, DELETE );

	if( hService )
	{
		if( DeleteService( hService ) )
		{
			UTIL_NTLogEvent( EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, BNBT_SERVICE_NAME );

			bResult = TRUE;
		}
		else
			UTIL_NTLogEvent( EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, BNBT_SERVICE_NAME );

		CloseServiceHandle( hService );
	}

	CloseServiceHandle( hSCM );

	return bResult;
}

BOOL UTIL_NTServiceStart( )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	BOOL bResult = FALSE;

	// open the Service Control Manager

	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if( hSCM )
	{
		// try to open the service

		SC_HANDLE hService = OpenService( hSCM, BNBT_SERVICE_NAME, SERVICE_START );

		if( hService )
		{
			StartService( hService, 0, NULL );

			bResult = TRUE;

			CloseServiceHandle( hService );
		}

		// tidy up

		CloseServiceHandle( hSCM );
	}

	return bResult;
}

BOOL UTIL_NTServiceStop( )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	BOOL bResult = FALSE;

	// open the Service Control Manager

	SC_HANDLE hSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

	if( hSCM )
	{
		// try to open the service

		SC_HANDLE hService = OpenService( hSCM, BNBT_SERVICE_NAME, SERVICE_STOP );

		if( hService )
		{
			SERVICE_STATUS ssStatus;

			if( ControlService( hService, SERVICE_CONTROL_STOP, &ssStatus ) )
				bResult = TRUE;

			CloseServiceHandle( hService );
		}

		CloseServiceHandle( hSCM );
	}

	return bResult;
}

void UTIL_NTLogEvent( WORD wType, DWORD dwID, const char *pszS1, const char *pszS2, const char *pszS3 )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	// make an entry into the application event log

	const char *ps[3];
	ps[0] = pszS1;
	ps[1] = pszS2;
	ps[2] = pszS3;

	int iStr = 0;

	for( int i = 0; i < 3; i++ )
	{
		if( ps[i] != NULL )
			iStr++;
	}

	// check if the event source has been registered, and if not then register it now

	HANDLE hNTEventSource = RegisterEventSource( NULL, BNBT_SERVICE_NAME );

	if( hNTEventSource )
	{
		ReportEvent( hNTEventSource, wType, 0, dwID, NULL, iStr, 0, ps, NULL );

		DeregisterEventSource( hNTEventSource );
	}
}

void WINAPI NTServiceHandler( DWORD dwOpcode )
{
		CFG_Open( CFG_FILE );
//#undef BNBT_SERVICE_NAME
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "CBTT Service" ).c_str())
		CFG_Close( CFG_FILE );
	// handle commands from the Service Control Manager

	if( dwOpcode == SERVICE_CONTROL_STOP )
	{
		gssStatus.dwCurrentState = SERVICE_STOP_PENDING;

		SetServiceStatus( ghServiceStatus, &gssStatus );

		gpServer->Kill( );
	}

	SetServiceStatus( ghServiceStatus, &gssStatus );
}

void WINAPI NTServiceMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
	gssStatus.dwCheckPoint = 0;
	gssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	gssStatus.dwCurrentState = SERVICE_START_PENDING;
	gssStatus.dwServiceSpecificExitCode = 0;
	gssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gssStatus.dwWaitHint = 0;
	gssStatus.dwWin32ExitCode = 0;

	// register the control request handler

	if( !( ghServiceStatus = RegisterServiceCtrlHandler( BNBT_SERVICE_NAME, NTServiceHandler ) ) )
		return;

	SetServiceStatus( ghServiceStatus, &gssStatus );

	gssStatus.dwWin32ExitCode = 0;
	gssStatus.dwCheckPoint = 0;
	gssStatus.dwWaitHint = 0;

	UTIL_NTLogEvent( EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED );

	// tell the Service Control Manager we started

	gssStatus.dwCurrentState = SERVICE_RUNNING;

	SetServiceStatus( ghServiceStatus, &gssStatus );

	gssStatus.dwWin32ExitCode = 0;
	gssStatus.dwCheckPoint = 0;
	gssStatus.dwWaitHint = 0;

	// run the tracker

	bnbtmain( );

	// tell the Service Control Manager we stopped

	UTIL_NTLogEvent( EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED );

	gssStatus.dwCurrentState = SERVICE_STOPPED;

	SetServiceStatus( ghServiceStatus, &gssStatus );
}

SERVICE_STATUS_HANDLE ghServiceStatus;
SERVICE_STATUS gssStatus;
