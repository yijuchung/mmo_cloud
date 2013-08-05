#include "table.h"

int		USMXclSocket;		// user <> mx 's command packet (TCP)
int		MXZSlSocket;		// mx <> zone 's packet (UDP)
int		ZScSocket;			// zone 's command packet (TCP)

char	mx_ip[MAX_IP_LEN];
char	mx_port[MAX_PORT_LEN];

int seq = 1;

fd_set	rset;
fd_set	temprset;
int maxfdp1;

User Users[MAX_USERS_MX];

struct	Zone ZoneTable[MAX_ZONE];
struct	UserZone UserTable[MAX_USERS_MX];
struct	Cache CacheTable[MAX_CACHE];
int		iCacheNum;

void InitialTable()
{
	int	iCacheNum = 0;
	for(int i = 0 ;i<MAX_ZONE; ++i)
	{
		ZoneTable[i].bOnline = false;
		ZoneTable[i].iZid = -1;
	}
	for(int i = 0 ;i<MAX_USERS_MX; ++i)
	{
		UserTable[i].bDirtybit = false;
	}
	for(int i = 0 ;i<MAX_CACHE; ++i)
	{
		CacheTable[i].bDirtybit = false;
	}

	for(int i = 0;i < MAX_USERS_MX;++i)
	{
		Users[i].bConnected = false;
		Users[i].iUid = -1;
	}
}

int FindUserID(int uid)
{
	for(int i = 0;i < MAX_USERS_MX;++i)
	{
		if( Users[i].iUid == uid )
		{
			return i;
		}
	}
	return -1;
}

void Print_Zone ()
{
	for(int i = 0;i<MAX_ZONE;++i)
	{
		if(ZoneTable[i].bOnline)
		printf("zone [%d] ip : %s , port : %s\n",ZoneTable[i].iZid,ZoneTable[i].sIP,ZoneTable[i].sPort);
	}
}

void Print_User ()
{
	for(int i = 0;i<MAX_USERS_MX;++i)
	{
		if(UserTable[i].bDirtybit)
		printf("user [%d] uid : %d , zid : %d\n",i,UserTable[i].iUid,UserTable[i].iZid);
	}
}

void Print_Cache ()
{
	for(int i = 0;i<MAX_CACHE;++i)
	{
		if(CacheTable[i].bDirtybit)
		printf("cache [%d] uid:%d ,zid:%d\n",i,CacheTable[i].iUid,CacheTable[i].iZid);
	}
}

int SendUDP ( int sockfd , char *pBuf ,int size,char *ip , char *port)
{
	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(port));
	addr.sin_addr.S_un.S_addr   = inet_addr(ip);

	return sendto( sockfd, pBuf, size , 0 , (struct sockaddr *)&addr, sizeof(struct sockaddr) );
}

bool SendByTable( int sock , int uid , char *buf , int size )
{
	bool check = false;
	int i = 0,l = 0;
	while( i < MAX_CACHE && l < iCacheNum )
	{
		if ( CacheTable[i].bDirtybit )
		{
			l++;
			if( CacheTable[i].iUid == uid )
			{
				printf("uid %d send by cache to zid %d\n",uid,CacheTable[i].iZid);
				SendUDP( sock , buf , size , (char *)&CacheTable[i].sIP , (char *)&CacheTable[i].sPort );
				check = true;
			}
		}
		++i;
	}
	if(check)
		return true;

	int j = 0;
	while(true)
	{
		if ( UserTable[j].bDirtybit && (UserTable[j].iUid == uid) )
		{
			break;
		}
		++j;
		if( j > MAX_USERS )
			break;
	}

	int k = 0;
	while(true)
	{
		if ( ZoneTable[k].bOnline && (ZoneTable[k].iZid == UserTable[j].iZid) )
		{
			SendUDP( sock , buf , size , (char *)&ZoneTable[k].sIP , (char *)&ZoneTable[k].sPort );
			AddCache( uid , UserTable[j].iZid , (char *)&ZoneTable[k].sIP , (char *)&ZoneTable[k].sPort);
			return true;
		}
		++k;
		if( k > MAX_ZONE )
			break;
	}
	return false;
}

bool ProcessZSPacket( const char * pBuffer )
{
    MessageHeader * pMh = (MessageHeader*)pBuffer;

	switch( pMh->MsgID )
    {
		case MSG_ZSUSINFOUPDATETOMX:
			{
				ZSUSInfoUpdateToMX * zium = (ZSUSInfoUpdateToMX *)pBuffer;
				
				MXUSInfoUpdateToUS miuu;
				miuu.iUid = zium->iUid_S;
				miuu.fPos[0] = zium->fPos[0];
				miuu.fPos[1] = zium->fPos[1];
				miuu.iHP = zium->iHP_S;
				miuu.iMP = zium->iMP_S;
				
				int i = FindUserID( zium->iUid_D );
				int len = Users[i].SendPacket( (char *)&miuu, sizeof(miuu));
				//printf("mx send to us[%d] uid = %d update uid = %d 'packet hp %d mp %d!!\n",i,zium->iUid_D,zium->iUid_S,zium->iHP_S,zium->iMP_S);
			}break;

	}
    return true;
}

bool ProcessZScPacket( const char * pBuffer )
{
    MessageHeader * pMh = (MessageHeader*)pBuffer;

	switch( pMh->MsgID )
    {
		case MSG_ZSUPDATETOMX:
			{
				//printf("1");
				ZSUpdateToMX * zutm = (ZSUpdateToMX*)pBuffer;
				//printf("new Zid=%d,ip=%s,port=%s !!!!!\n",zutm->iZid, zutm->sZip , zutm->sZport);
				AddZone ( zutm->iZid, zutm->sZip , zutm->sZport );
			}break;

		case MSG_ZSMIGUPDATETOMX:
			{		
				ZSMigUpdateToMX * zmum = (ZSMigUpdateToMX *)pBuffer;

				AddCache( zmum->iUid , zmum->iZid , zmum->sZip , zmum->sZport);
				//printf("after mig update\n");
				//Print_Cache();
			}break;

		case MSG_ZSDUPTOMX:
			{
				ZSDupToMX * zdm = (ZSDupToMX *)pBuffer;

				UpdateCachePro( zdm->iZid_S, zdm->iZid_D, zdm->sZip,zdm->sZport );
				//Print_Cache();
			}break;

		case MSG_ZSUSCOMTOUS:
			{
				//printf("3");
				//printf("recv user com\n");
				ZSUSComToUS * zucu = (ZSUSComToUS *)pBuffer;

				int i = FindUserID( zucu->iUid_S );
				int j = FindUserID( zucu->iUid_D );
				Users[i].SendPacket( (char *)&zucu, sizeof(zucu));
				Users[j].SendPacket( (char *)&zucu, sizeof(zucu));
			}break;

		case MSG_ZSMIGOFFLINETOMX:
			{
				//printf("4");
				ZSMigOfflineToMX * zmom = (ZSMigOfflineToMX *)pBuffer;
				//printf("mx del uid %d zid %d\n",zmom->iUid , zmom->iZid);
				DelCache( zmom->iUid , zmom->iZid );
				//printf("after mig del\n");
				//Print_Cache();
			}break;

		case MSG_ZSPRODONEMX:
			{
				//printf("5");
				ZSProDoneMX * zpdm = (ZSProDoneMX *)pBuffer;
				//printf("mx pro mig zid %d to zid %d\n",zpdm->iZid_S , zpdm->iZid_D);
				UpdateCache( zpdm->iZid_S , zpdm->iZid_D );
				
				//Print_Cache();
			}break;

        case MSG_ZSBROADUSERLOGOFF:
			{
				//printf("6");
				ZSBroadUserLogoff *zbul = (ZSBroadUserLogoff *)pBuffer;

				UserLogoff ulf;
				ulf.iUid = zbul->iUid_S;
				
				int i = FindUserID( zbul->iUid_D );
				Users[i].SendPacket( (char *)&ulf, sizeof(ulf));
			}break;
	}
    return true;
}

bool ProcessUSPacket( const char * pBuffer )
{
    MessageHeader * pMh = (MessageHeader*)pBuffer;
	
	switch( pMh->MsgID )
    {
        case MSG_USERUPDATETOMX:
            {
				//printf("us update to mx packet !!!!!\n");
				UserUpdateToMX * uutm = (UserUpdateToMX *)pBuffer;

				UserUpdateToZS uutz;
				uutz.iUid = uutm->iUid;
				uutz.fPos[0] = uutm->fPos[0];
				uutz.fPos[1] = uutm->fPos[1];

				if ( !SendByTable( MXZSlSocket , uutm->iUid , (char *)&uutz , sizeof(UserUpdateToZS) ) )
				{
					printf("send update but table search error!!!\n");
					return false;
				}
				return true;
            }break;
        case MSG_USERCOMTOMX:
            {
				//printf("us update to mx packet !!!!!\n");
				UserComToMX * ucm = (UserComToMX *)pBuffer;

				UserComToZS ucz;
				int i = FindUserID( ucm->iUid_D );
				
				ucz.iUid_D = ucm->iUid_D;
				ucz.iUid_S = ucm->iUid_S;
				ucz.cCom = ucm->cCom;
				ucz.iTime = ucm->iTime;
				//ucz.iTime = seq;
				//printf("send com to zs %d\n",seq);
				//++seq;
				
				if ( !SendByTable( MXZSlSocket , ucz.iUid_D , (char *)&ucz , sizeof(UserComToZS) ) )
				{
					printf("send update but table search error!!!\n");
					return false;
				}

				return true;
            }break;
        case MSG_USERLOGOFF:
			{
				UserLogoff *ulf = (UserLogoff *)pBuffer;
				
				if ( !SendByTable( MXZSlSocket , ulf->iUid , (char *)ulf , sizeof(UserLogoff) ) )
				{
					//printf("send logoff but table search error!!!\n");
					return false;
				}
				int i = FindUserID( ulf->iUid );
				DelUser(ulf->iUid);
				Users[i].Destroy();
				return true;
			}break;
    }
}

bool ProcessUScPacket( const char * pBuffer , int sockfd )
{
    MessageHeader * pMh = (MessageHeader*)pBuffer;
	
	switch( pMh->MsgID )
    {
        case MSG_USERLOGINTOMX:
            {
				UserLoginToMX * ultm = (UserLoginToMX *)pBuffer;
				printf("recv uid %d login\n",ultm->iUid);
				if ( AddUser( ultm->iUid, ultm->iZid ) )
				{
					for(int i = 0 ; i < MAX_USERS_MX; ++i)
					{
						if( !Users[i].bConnected )
						{
							if( !Users[i].Create(sockfd , ultm->iUid ) )
							{
								return false;
							}
							break;
						}

					}					
				}else
				{
					printf( "user %d already online no need to login !!!\n",ultm->iUid );
				}

				FD_SET ( sockfd , &rset );
				maxfdp1 = max ( sockfd , (maxfdp1 - 1) ) + 1;

				UserLoginToZS ulz;
				ulz.iUid = ultm->iUid;
				int i = 0;
				while(true)
				{
					if ( ZoneTable[i].bOnline && (ZoneTable[i].iZid == ultm->iZid) )
					{
						//printf("mx send user login to zs , ip = %s, port = %s\n",ZoneTable[i].sIP,ZoneTable[i].sPort);
						SendUDP( MXZSlSocket , (char *)&ulz ,sizeof(UserLoginToZS), (char *)&ZoneTable[i].sIP , (char *)&ZoneTable[i].sPort );
						AddCache( ultm->iUid , ultm->iZid , (char *)&ZoneTable[i].sIP , (char *)&ZoneTable[i].sPort);
						//printf("addcache %d %d %s %s\n", ultm->iUid , ultm->iZid , ZoneTable[i].sIP , ZoneTable[i].sPort);
						return true;
					}
					++i;
				}
            }break;
    }
}

int main(int argc,char *argv[])
{
	srand ( GetTickCount() );
	InitialTable();
	strcpy ( mx_ip, argv[2] );
	strcpy ( mx_port, argv[3] );
	
	WSADATA wsaData;
	if( SOCKET_ERROR == WSAStartup( WINSOCK_VERSION, &wsaData ) || wsaData.wVersion != WINSOCK_VERSION )
            return false;

	USMXclSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof(addr) );
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(mx_port));
	addr.sin_addr.S_un.S_addr   = inet_addr(mx_ip);
	
	if( SOCKET_ERROR == bind( USMXclSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr) ) )
		return -1;
	
	listen(USMXclSocket,MAX_WAIT_LOG);

	MXZSlSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in addr2;
	ZeroMemory( &addr2, sizeof(addr2) );
	addr2.sin_family = AF_INET;
	addr2.sin_port   = htons(atoi(mx_port) + 1);
	addr2.sin_addr.S_un.S_addr   = inet_addr(mx_ip);
	
	if( SOCKET_ERROR == bind( MXZSlSocket, (struct sockaddr*)&addr2, sizeof(struct sockaddr) ) )
		return -1;
	
	listen(MXZSlSocket,MAX_WAIT_LOG);

	ZScSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr3;
	ZeroMemory( &addr3, sizeof(addr3) );
	addr3.sin_family = AF_INET;
	addr3.sin_port   = htons(atoi(mx_port) + 2);
	addr3.sin_addr.S_un.S_addr   = inet_addr(mx_ip);
	
	if( SOCKET_ERROR == bind( ZScSocket, (struct sockaddr*)&addr3, sizeof(struct sockaddr) ) )
		return -1;

	listen(ZScSocket,MAX_WAIT_LOG);	
   
   	int nready;
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
		
	FD_ZERO(&rset);
	FD_ZERO(&temprset);
	FD_SET( MXZSlSocket, &rset);
	FD_SET( USMXclSocket, &rset);
	FD_SET( ZScSocket, &rset);

	int maxfdp1 = max (MXZSlSocket ,USMXclSocket ) + 1;
	maxfdp1 = max (ZScSocket,maxfdp1 - 1) + 1;

	while(true)
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		FD_ZERO(&temprset);
		temprset = rset;

		if ( (nready = select(maxfdp1, &temprset, NULL, NULL, NULL)) < 0)
		{
			break;

		}else if( nready == 0)
		{
			continue;
		}

		if (FD_ISSET(MXZSlSocket, &temprset))
		{
			//printf("recv packet from zs !!!!!\n");
			char buffer[MAX_PACKET_SIZE];
			struct sockaddr address;
			int alen = sizeof(struct sockaddr);
			ZeroMemory( buffer, sizeof(buffer) );

			int len = recvfrom( MXZSlSocket, (char *)&buffer , sizeof(buffer) , 0, (struct sockaddr *)&address,&alen);

			ProcessZSPacket( (char *)&buffer );
			continue;
		}

		if (FD_ISSET(ZScSocket, &temprset))
		{
			//printf("recv zs command !!!!!\n");
			char buffer[MAX_PACKET_SIZE];
			struct sockaddr address;
			int len = sizeof(struct sockaddr);

			ZeroMemory( buffer, sizeof(buffer) );
			int sockfd = accept ( ZScSocket, (struct sockaddr *)&address, &len );
			RecvTCPPacket( sockfd, (char *)&buffer );

			ProcessZScPacket( (char *)&buffer );

			closesocket( sockfd );
			continue;
		}

		if (FD_ISSET(USMXclSocket, &temprset))
		{

			printf("recv command packet from us !!!!!\n");
			char buffer[MAX_PACKET_SIZE];
			struct sockaddr address;
			int len = sizeof(struct sockaddr);

			ZeroMemory( buffer, sizeof(buffer) );

			int sockfd = accept ( USMXclSocket, (struct sockaddr *)&address, &len );
			RecvTCPPacket( sockfd, (char *)&buffer );

			if ( !ProcessUScPacket( (char *)&buffer, sockfd ) )
			{
				closesocket( sockfd );
			}
			continue;
		}

		for(int i = 0;i < MAX_USERS;++i)
		{
			if(Users[i].bConnected && FD_ISSET(Users[i].iSocket, &temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				ZeroMemory( buffer, sizeof(buffer) );

				RecvTCPPacket( Users[i].iSocket , (char *)&buffer );				
				ProcessUSPacket( (char *)&buffer );
				continue;
			}
		}

	}

	closesocket( MXZSlSocket );
	closesocket( USMXclSocket );
	WSACleanup();
   
	return 0;
}