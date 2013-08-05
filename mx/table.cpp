#include "table.h"
extern struct Zone ZoneTable[MAX_ZONE];
extern struct UserZone UserTable[MAX_USERS];
extern struct Cache CacheTable[MAX_CACHE];

extern int iCacheNum;

bool AddZone (int zid, char *zip, char *zport)
{
	int InsertPoint = -1;
	bool check = false;
	for( int i = 0 ; i < MAX_ZONE ; ++i )
	{
		if(!check && (ZoneTable[i].bOnline == false))
		{
			InsertPoint = i;
			check = true;
		}
		
		if(zid == ZoneTable[i].iZid)
		{
			if(strcmp(ZoneTable[i].sIP , zip) == 0)
			{
				printf("zone %d already exist!!\n",zid);
				return false;
			}
			else
			{
				strcpy( ZoneTable[i].sIP , zip );
				strcpy( ZoneTable[i].sPort , zport );
				printf("zone %d update ip to %s and port %s!!!\n",zid,zip,zport);
				return true;
			}
		}
	}
	
	ZoneTable[InsertPoint].bOnline = true;
	ZoneTable[InsertPoint].iZid = zid;
	strcpy( ZoneTable[InsertPoint].sIP , zip );
	strcpy( ZoneTable[InsertPoint].sPort , zport );
	//printf("new zone[%d] add ip %s , port %s!!!\n",InsertPoint,ZoneTable[InsertPoint].sIP,ZoneTable[InsertPoint].sPort);
	return true;
}

bool AddUser (int uid, int zid)
{
	int InsertPoint = -1;
	bool check = false;
	for( int i = 0 ; i < MAX_USERS_MX ; ++i )
	{

		if(!check && !UserTable[i].bDirtybit)
		{
			InsertPoint = i;
			check = true;
		}

		if(UserTable[i].iUid == uid)
			if(UserTable[i].iZid == zid)
			{
				printf("user %d table element already exist!!!\n",uid);
				return false;
			}
	}
	
	UserTable[InsertPoint].bDirtybit = true;
	UserTable[InsertPoint].iUid = uid;
	UserTable[InsertPoint].iZid = zid;
	
	//printf("new user %d add!!!\n",uid);
	return true;
}

bool AddCache (int uid, int zid, char *ip, char *port)
{
	int InsertPoint = -1;
	bool check = false;
	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{

		if(!check && !CacheTable[i].bDirtybit)
		{
			InsertPoint = i;
			check = true;
		}

		if(CacheTable[i].bDirtybit && CacheTable[i].iUid == uid)
			if(CacheTable[i].iZid == zid)
			{
				printf("cache table element already exist!!!\n");
				return false;
			}
	}
	
	CacheTable[InsertPoint].bDirtybit = true;
	CacheTable[InsertPoint].iUid = uid;
	CacheTable[InsertPoint].iZid = zid;
	strcpy( CacheTable[InsertPoint].sIP , ip );
	strcpy( CacheTable[InsertPoint].sPort , port );
	iCacheNum++;
	//printf("new cache add!!!\n");
	return true;
}

void DelUser ( int uid )
{
	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{
		if( CacheTable[i].bDirtybit )
		{
			if(CacheTable[i].iUid == uid)
			{
				CacheTable[i].bDirtybit = false;
			}
		}
	}

	for( int i = 0 ; i < MAX_USERS_MX ; ++i )
	{
		if(UserTable[i].bDirtybit)
		{
			if(UserTable[i].iUid == uid)
			{
				UserTable[i].bDirtybit = false;
				break;
			}
		}
	}
	printf("user uid = %d logoff!!!\n", uid);
}

void UpdateUserTable ( int uid , int zid )
{
	for( int i = 0 ; i < MAX_USERS_MX ; ++i )
	{
		if( UserTable[i].iUid == uid)
		{
			for( int i = 0 ; i < MAX_CACHE ; ++i )
			{
				if( CacheTable[i].iUid == uid)
				{
					UserTable[i].iZid = CacheTable[i].iZid;
					printf("user uid = %d zone update to zid = %d!!!\n", uid, CacheTable[i].iZid);
					return;
				}
			}
		}
	}
}

void DelCache ( int uid , int zid )
{
	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{
		if( CacheTable[i].bDirtybit && CacheTable[i].iUid == uid )
		{
			if(CacheTable[i].iZid == zid)
			{
				printf("CacheTable[%d] zid %d del\n",i,zid);
				CacheTable[i].bDirtybit = false;
				break;
			}
		}
	}
}

int FindZoneTable (int zid)
{
	int k = 0;
	while(true)
	{
		if ( ZoneTable[k].bOnline && (ZoneTable[k].iZid == zid) )
		{
			return k;
		}
		++k;
		if( k > MAX_ZONE )
			break;
	}
	return -1;
}

int UpdateCachePro (int zid, int zidn, char *ip, char *port)
{
	//printf("port %s\n",port);
	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{
		if( CacheTable[i].bDirtybit && CacheTable[i].iZid == zid )
		{
			AddCache( CacheTable[i].iUid, zidn, ip, port );
		}
	}
	return -1;
}

void UpdateCache ( int zid, int zidn )
{
	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{
		if( CacheTable[i].bDirtybit && CacheTable[i].iZid == zidn )
		{
			CacheTable[i].bDirtybit = false;
		}
	}

	for( int i = 0 ; i < MAX_CACHE ; ++i )
	{
		if( CacheTable[i].bDirtybit && CacheTable[i].iZid == zid )
		{
			CacheTable[i].iZid = zidn;
		}
	}
}

User::User()
{
	iSocket = -1;
	iUid = -1;
	iSeqNum = 0;
	bConnected = false;
}

User::~User()
{
	Destroy();
}

bool User::Create( int iSock, int uid )
{
	iUid = uid;
	iSocket = iSock;
	iSeqNum = 0;
	bConnected = true;

	return true;
}

void User::Destroy()
{
	bConnected = false;
	iSocket = -1;
	iUid = -1;
	iSeqNum = 0;
}

int User::SendPacket( char * pBuffer, int length )
{
	return SendTCPPacket( iSocket, pBuffer, length );
}

int User::RecvPacket( char * pBuffer, int length )
{
	return RecvTCPPacket( iSocket , pBuffer );
}