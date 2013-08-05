#include <time.h>
#include <math.h>
#include "net.h"
#include "gs.h"
#include "glut/glut.h"

int sSocket,sZSocket;
fd_set rset,temprset;
int maxfdp1;

ZoneServer ZS[MAX_ZONE+5];
UserData UD[MAX_USERS];

int RegUser;
int NowZone;

char gs_ip[MAX_IP_LEN];
char gs_port[MAX_PORT_LEN];
char mx_ip[MAX_IP_LEN];
char mx_port[MAX_PORT_LEN];

bool SendMXCommand(char *pBuffer, int size)
{
	int sMXcSocket = socket( AF_INET, SOCK_STREAM, 0 );

	struct sockaddr_in addr4;
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons( atoi(mx_port)+2 );
	addr4.sin_addr.S_un.S_addr = inet_addr(mx_ip);

	connect( sMXcSocket , (struct  sockaddr *)&addr4, sizeof(struct sockaddr) );

	SendTCPPacket( sMXcSocket , (char *)pBuffer , size );
	
	closesocket( sMXcSocket );
	return true;
}

void Initial ( void )
{
	FILE *fp;
 	fp = fopen("config.ini","r");
	char buf[3];
	char buf_ip[MAX_IP_LEN];
	char buf_port[MAX_PORT_LEN];
	fgets( (char *)&buf, 3, fp);
	
	do
	{		
		if (strcmp( buf , "GS") == 0)
		{
			fgetc( fp );
			fscanf( fp, "%s %s" , &buf_ip, &buf_port );
			strcpy( gs_ip, buf_ip );
			strcpy( gs_port, buf_port );

		}else if (strcmp( buf , "MX") == 0)
		{
			fgetc( fp );
			fscanf( fp, "%s %s" , &buf_ip, &buf_port );
			strcpy( mx_ip, buf_ip );
			strcpy( mx_port, buf_port );
		}
		
	}while(fgets( (char *)&buf, 3, fp)!=NULL);

	fclose(fp);
}

int CheckZoneServer( char *ZoneName, int level = 1, int zid = -1 )
{
	// i is the zid
	int i = 0;
	while( i < MAX_ZONE)
	{
		if( strcmp( ZoneName, ZS[i].sName ) == 0 )
		{
			if( ZS[i].bOpen )
			{
				printf("%s already open\n",ZS[i].sName);
				++ZS[i].iUser;
			}
			else
			{
				++ZS[i].iUser;
				char strcmd[80];
				printf("%s is opening\n",ZS[i].sName);
				sprintf( strcmd, "start zs %s %s %d %s %d %d %d %d %d %d", 
					ZS[i].sIP , ZS[i].sPort, i, ZoneName,ZS[i].iX,ZS[i].iY,ZS[i].iW,ZS[i].iH, level, zid);
				_popen( strcmd,"r" );
				ZS[i].bOpen = true;
			}
			return i;
		}
		i++;
	}
	printf("no such mig zone error!!!!\n");
	return -1;
}

int CheckMigZone( float fPosX,float fPosY )
{
	//printf("check zone x=%f,y=%f\n",fPosX,fPosY);
	for(int i = 0;i<MAX_ZONE;++i)
	{
		if ( fPosX < (float)(ZS[i].iX+ZS[i].iW) && fPosX >= (float)ZS[i].iX )
			if( fPosY < (float)(ZS[i].iY+ZS[i].iH) && fPosY >= (float)ZS[i].iY )
			{
				return CheckZoneServer( ZS[i].sName );
			}
	}
	printf("check mig zone error!!!!\n");
	return -1;
}

bool ProcessZonePacket( ZoneServer * pZone, const char * pBuffer, int iSize )
{
	MessageHeader * mh = (MessageHeader*)pBuffer;

	switch( mh->MsgID )
	{
		case MSG_ZSUSMIGTOGS:
		{
				ZSUSMigToGS *zmg = (ZSUSMigToGS*)pBuffer;
				printf("recv uid %d mig to zid %d from zid %d\n",zmg->iUid,zmg->iZid,pZone->iZid);
				--pZone->iUser;

				//int iMap = CheckMigZone( (float)zmg->fPos[0], (float)zmg->fPos[1] );
				
				CheckZoneServer( ZS[zmg->iZid].sName );

				GSUSMigToZS gmz;
				strcpy ( gmz.sZip, ZS[zmg->iZid].sIP );
				strcpy ( gmz.sZport, ZS[zmg->iZid].sPort );
				gmz.iZid = zmg->iZid;
				gmz.iUid = zmg->iUid;
				SendTCPPacket( pZone->iSocket , (char *)&gmz , sizeof(gmz) );

		}break;
	
		case MSG_ZSUSZOOGS:
		{
				ZSUSZooGS *zuzg = (ZSUSZooGS*)pBuffer;
				
				strcpy( UD[zuzg->iUid].sZone, ZS[zuzg->iZid].sName );
		}break;

		case MSG_ZSPRODONEGS:
		{
				ZSProDoneGS *zpdg = (ZSProDoneGS*)pBuffer;

				printf("disconnect %d and replace from %d\n",zpdg->iZid_S,zpdg->iZid_D);
				ZS[zpdg->iZid_D].bOpen = false;
				FD_CLR( ZS[zpdg->iZid_S].iSocket, &rset );
				closesocket( ZS[zpdg->iZid_S].iSocket );
				ZS[zpdg->iZid_S].iSocket = ZS[zpdg->iZid_D].iSocket;
				
				printf("pro mig done cpy temp zid %d to zid %d\n",zpdg->iZid_D,zpdg->iZid_S);
		}break;

		case MSG_ZSPROMIGGS:
		{
				ZSProMigGS *zpmg = (ZSProMigGS*)pBuffer;
				//--------------------process migration------------------
				//printf("process mig!!!\n");

				int j = 0;
				for(int i = 0;i < 5;++i)
				{
					j = MAX_ZONE+i;
					if(!ZS[j].bOpen)
					{
						ZS[j].bOpen = true;
						strcpy( ZS[j].sIP , "140.109.22.252" );
						strcpy( ZS[j].sPort , "9988" );
						char strcmd[80];
						printf("Temp zs %d is opening\n",j);
						sprintf( strcmd, "start zs %s %s %d %s %d %d %d %d %d %d", ZS[j].sIP , ZS[j].sPort, j, 
							ZS[zpmg->iZid].sName, ZS[zpmg->iZid].iX,ZS[zpmg->iZid].iY,ZS[zpmg->iZid].iW,ZS[zpmg->iZid].iH, 0, zpmg->iZid);
						_popen( strcmd,"r" );
						break;
					}
				}
				
				ZSDupToMX zdm;
				zdm.iZid_D = j;
				zdm.iZid_S = zpmg->iZid;
				strcpy( zdm.sZip , ZS[j].sIP );
				strcpy( zdm.sZport , ZS[j].sPort );
				SendMXCommand( (char *)&zdm, sizeof(zdm));
				Sleep(50);
				ACKZSProMigGS apmg;
				apmg.iZid = j;
				strcpy( apmg.sZip , ZS[j].sIP );
				strcpy( apmg.sZport , ZS[j].sPort );

				SendTCPPacket( pZone->iSocket, (char *)&apmg, sizeof(apmg) );
				//-------------------------------------------------------				
		}break;

		case MSG_USERLOGOFF:
		{
				UserLogoff * ul = (UserLogoff *)pBuffer;
				UD[ul->iUid].bOnline = false;	
		}break;
	}

	return true;
}

void InitialZoneData()
{
	FILE *fp;
  
	fp = fopen("ZooZoneInGS.txt","r");

	NowZone = -1;
	char	ZoneName[MAX_ZONE_NAME];
	int		bOpen;
	char	sIP[MAX_IP_LEN];
	char	sPort[MAX_PORT_LEN];
	int		iX,iY,iW,iH,iUser;

	while ( fscanf(fp,"%d %s %d %s %s %d %d %d %d %d",&NowZone,&ZoneName,&bOpen,&sIP,&sPort,&iX,&iY,&iW,&iH,&iUser) != EOF )
	{
		strcpy ( ZS[NowZone].sName, ZoneName );
		strcpy ( ZS[NowZone].sIP, sIP );
		strcpy ( ZS[NowZone].sPort, sPort );
		ZS[NowZone].bOpen = (bool)bOpen;
		ZS[NowZone].iX = iX;
		ZS[NowZone].iY = iY;
		ZS[NowZone].iW = iW;
		ZS[NowZone].iH = iH;
		ZS[NowZone].iUser = iUser;
	}

	fclose(fp);

	for(int i =0;i<5;++i)
	{
		ZS[MAX_ZONE+i].bOpen = false;
	}
}

void InitialUserData( )
{
	FILE *fp;
	
	fp = fopen("ZooUserDataInGS.txt","r");

	RegUser = -1;
	char	sUserName[MAX_NAME_LEN];
	char	sPass[MAX_NAME_LEN];
	char	sZone[MAX_ZONE_NAME];
	int		bOnline;
	
	while ( fscanf(fp,"%d %s %s %s %d",&RegUser,&sUserName,&sPass,&sZone,&bOnline) != EOF)
	{
		strcpy ( UD[RegUser].sUserName, sUserName );
		strcpy ( UD[RegUser].sPass, sPass );
		strcpy ( UD[RegUser].sZone, sZone );
		UD[RegUser].bOnline = (bool)bOnline;
	}

	fclose(fp);
}

Reason CheckNewPlayer ( UserLogInToGS *um , int *po)
{
	int i = 0;
	while(true)
	{
		if(strcmp ( UD[i].sUserName,um->sUserName ) == 0)
		{
			if(strcmp ( UD[i].sPass,um->sPass) == 0)
			{
				if(UD[i].bOnline)
				{
					printf("User already online!!\n");
					return R_ONLINE;
				}
				UD[i].bOnline = true;
				*po = i;
				return R_SUCCESS;
			}
			else
			{
				printf("Pass Wrong!!\n");
				return R_WRONGPASS;
			}
		}

		++i;
	}
	printf("Username Wrong!!\n");
	return R_NOTEXIST;
}

void SendZoneInfo( int sTSocket,int up )
{
	int iMap = CheckZoneServer(UD[up].sZone);

	if(iMap >= 0)
	{
		UserZoneInfo uzi;
		strcpy( uzi.sZip , ZS[iMap].sIP );
		strcpy( uzi.sZport , ZS[iMap].sPort );
		++ZS[iMap].iUser;
		uzi.iZid = iMap;
		uzi.iUid = up;
		SendTCPPacket( sTSocket , (char *)&uzi , sizeof(uzi) );

	}else
		printf("No such Zone !!\n");
}

int main()
{	
	//FILE *fp;
	//fp=fopen("gs.txt","wt");

	//*stdout = *fp;
	Initial();
	InitialZoneData();	
	InitialUserData();
	
	WSADATA wsaData;
	
	if( SOCKET_ERROR == WSAStartup( WINSOCK_VERSION, &wsaData ) || wsaData.wVersion != WINSOCK_VERSION )
		return -1;

	{
		int on = 1;
		sSocket = socket( AF_INET, SOCK_STREAM, 0 );

		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( atoi(gs_port) );
		addr.sin_addr.S_un.S_addr = inet_addr(gs_ip);

		if( SOCKET_ERROR == bind( sSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr) ) )
			return -1;
	
		listen(sSocket,MAX_WAIT_LOG);
		
		// listen connection for new zone add
		sZSocket = socket( AF_INET, SOCK_STREAM, 0 );
		
		struct sockaddr_in addr2;
		addr2.sin_family = AF_INET;
		addr2.sin_port = htons( atoi(gs_port)+1 );
		addr2.sin_addr.S_un.S_addr = inet_addr(gs_ip);

		if( SOCKET_ERROR == bind( sZSocket, (struct  sockaddr *)&addr2, sizeof(struct sockaddr) ) )
			return -1;
		
		listen(sZSocket,MAX_WAIT_LOG);

		int mx_num = 1;
		char strcmd[50];
		sprintf( strcmd, "start mx %d %s %s", mx_num , mx_ip, mx_port);
		_popen( strcmd,"r" );
		
		short int nready;

		FD_ZERO(&rset);	
		FD_SET(sSocket, &rset);
		FD_SET(sZSocket, &rset);
		
		maxfdp1 = max(sSocket,sZSocket) + 1;

		while(true) 
		{
			FD_ZERO(&temprset);
			temprset = rset;

			if ( (nready = select(maxfdp1, &temprset, NULL, NULL, NULL)) < 0)
			{
				break;
			}

			if (FD_ISSET(sSocket, &temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				struct sockaddr address;
				int len = sizeof(struct sockaddr);

				ZeroMemory( buffer, sizeof(buffer) );

				int sockfd = accept ( sSocket, (struct sockaddr *)&address, &len );
				
				RecvTCPPacket( sockfd , (char *)&buffer );

				int try_num = 0;
				bool check = false;
				while(try_num < 3)
				{

					MessageHeader * tMh = (MessageHeader*)buffer;
				
					switch( tMh->MsgID )
					{
						case MSG_USERLOGINTOGS:
						{
							UserLogInToGS *ulim = (UserLogInToGS*)buffer;
							UserReject urm;
							int up = 0;
							switch(CheckNewPlayer( ulim , &up ))
							{
								case 	R_NOTEXIST:
									{
										urm.rWhy = R_NOTEXIST;
										SendTCPPacket( sockfd, (char *)&urm, sizeof(urm) );
										//send( sockfd, (char *)&urm, sizeof(urm) , 0);
									}break;
								case 	R_ONLINE:
									{	
										urm.rWhy = R_ONLINE;
										SendTCPPacket( sockfd, (char *)&urm, sizeof(urm) );
									}break;
								case	R_FULL:
									{
										urm.rWhy = R_FULL;
										SendTCPPacket( sockfd, (char *)&urm, sizeof(urm) );
									}break;
								case	R_WRONGPASS:
									{
										urm.rWhy = R_WRONGPASS;
										SendTCPPacket( sockfd, (char *)&urm, sizeof(urm) );
									}break;
								case	R_SUCCESS:
									{
										SendZoneInfo( sockfd , up );
										check = true;
									}break;
							}
						}break;
					}
					
					if(!check)
					{
						++try_num;
						RecvTCPPacket( sockfd , (char *)&buffer );
					}else
						break;
				}

				closesocket( sockfd );
				continue;
			}

			if (FD_ISSET(sZSocket, &temprset))
			{
				//printf("GS get ZS first packet !!!!!\n");
				char buffer[MAX_PACKET_SIZE];
				struct sockaddr address;
				int len = sizeof(struct sockaddr);

				ZeroMemory( buffer, sizeof(buffer) );
				
				int sockfd = accept ( sZSocket, (struct sockaddr *)&address, &len );
				RecvTCPPacket( sockfd , (char *)&buffer );

				MessageHeader * tMh = (MessageHeader*)buffer;
				
				switch( tMh->MsgID )
				{
					case MSG_ZSOPENTOGS:
					{						
						ZSOpenToGS *zog = (ZSOpenToGS*)buffer;
						ZS[zog->iZid].bOpen = true;
						ZS[zog->iZid].iSocket = sockfd;
						FD_SET ( ZS[zog->iZid].iSocket , &rset );
						maxfdp1 = max( maxfdp1 - 1,ZS[zog->iZid].iSocket ) + 1;

					}break;
				}
				
				continue;
			}


			for(int i = 0 ; i < MAX_ZONE+5 ; ++i)
			{
				if( ZS[i].bOpen && FD_ISSET(ZS[i].iSocket, &temprset))
				{
					//printf("recv zs\n");
					char buffer[MAX_PACKET_SIZE];
					int size = 0;

					if(((size = RecvTCPPacket( ZS[i].iSocket , (char *)&buffer )) == 0) && (ZS[i].bOpen == true))
					{
						ZS[i].bOpen = false;
						FD_CLR( ZS[i].iSocket, &rset );
						
						//if( i >= MAX_ZONE )
						//{
						//	closesocket( ZS[i].iSocket );
						//}else{
						//	CheckZoneServer(ZS[i].sName);
						//}
						break;
					}
					
					ProcessZonePacket( &ZS[i] , (char *)&buffer, size );

					break;
				}
			}
		}
	}

	closesocket( sSocket );
	closesocket( sZSocket );
	WSACleanup();
	//fclose(fp);

	return 0;
}