//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef CONFIG_H
 #define CONFIG_H

#define CFG_FILE "bnbt.cfg"

extern map<string, string> gmapCFG;

void CFG_Open( const char *szFile );
void CFG_SetInt( string strKey, int x );
void CFG_SetString( string strKey, string x );
int CFG_GetInt( string strKey, int x );
string CFG_GetString( string strKey, string x );
void CFG_Delete( string strKey );
void CFG_Close( const char *szFile );
void CFG_SetDefaults( );

#endif
