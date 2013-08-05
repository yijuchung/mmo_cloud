#include <time.h>
#include <math.h>
#include <stdio.h>
#include "sync.h"
#include "user.h"
#include "net.h"
#include "zstable.h"
#include "glut/glut.h"
#include "messages.h"
#include "combuf.h"

#define SEND_PACKET					40
#define SEND_USER_GS				500

//----------------------zone setting---------------------------
int			iLevel;					// 0 - silent, 1 - active
int			iZoneX;
int			iZoneY;
int			iZoneW;
int			iZoneH;
int			iZoneUser;
bool		InProMig;
//-------------------------------------------------------------
//-------------------------sync--------------------------------
struct TempObject sync_obj[MAX_SYNC_OBJ];
struct AdjZone zstable[9];

extern struct TempUser tu[MAX_SYNC_OBJ];
//-------------------------------------------------------------

int			sSocket;				// for zone udp
int			sGSSocket;				// for gs tcp
int			sMXSocket;				// for mx udp
int			sZScSocket;				// for zs tcp communication
struct		sockaddr_in addrtomx;

float		fLastUpdate;
float		fNowTime;
fd_set		rset,temprset;
int			maxfd;

int			iZid;
char		sZoneName[MAX_ZONE_NAME];

struct Redis
{
	int		iUid;
	bool	bOnline;
	char	sUserName[MAX_NAME_LEN];
	float	fX;
	float   fY;
	int		iHP;
	int		iMP;
};

Redis UserData[MAX_USERS_ZONE];				// 從檔案讀入的 user 資料
User Users[MAX_USERS_ZONE];					// 目前 handle 的玩家資料

char gs_ip[MAX_IP_LEN];
char gs_port[MAX_PORT_LEN];
char mx_ip[MAX_IP_LEN];
char mx_port[MAX_PORT_LEN];


DWORD WINAPI test_Thread( LPVOID pParam )
{	
	float time1 = GetTickCount();
	float time2;
	while( true )
	{
		time2 = GetTickCount();
		if( (time2 - time1) > 50 )
		{
			for(int i = 0;i<MAX_USERS_ZONE;++i)
			{
				if(Users[i].bConnected)
					--Users[i].iHP;
			}

			time1 = time2;
		}
	}
	return 0;
}

void InitialConfig( )
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

	IniObj();
}

void Print_Users()
{
	for(int i = 0;i<MAX_USERS_ZONE;++i)
	{
		if(Users[i].bConnected)
			printf("Users [%d] uid = %d , x %f , y %f, hp %d, time %d\n",i,Users[i].iUid,Users[i].fX,Users[i].fY,Users[i].iHP,Users[i].iTime);
	}
}

int FindUserDataID(int uid)
{
	for(int i = 0 ; i < MAX_USERS_ZONE ;++i)
	{
		if( UserData[i].iUid == uid )
			return i;
	}
	return -1;
}

void Initial( )
{
	FILE *fp;
	fp = fopen("RedisUserInZS.txt","r");

	for(int i = 0 ;i<MAX_USERS; ++i)
	{
		UserData[i].bOnline = false;
	}

	for(int i = 0 ;i<MAX_USERS_ZONE; ++i)
	{
		Users[i].bConnected = false;
		Users[i].iUid = -1;
	}
	
	char temp[50];
	
	fscanf( fp,"%s",temp);

	int			iUid;
	char		sUserName[MAX_NAME_LEN];
	float		fX,fY;
	int			iHP,iMP;
	int i = 0;
	while ( fscanf(fp,"%d %s %f %f %d %d",&iUid,&sUserName,&fX,&fY,&iHP,&iMP) != EOF)
	{
		UserData[i].iUid = iUid;
		UserData[i].bOnline = true;
		strcpy ( UserData[i].sUserName, sUserName );
		UserData[i].fX = fX;
		UserData[i].fY = fY;
		UserData[i].iHP = iHP;
		UserData[i].iMP = iMP;
		++i;
	}
	fclose(fp);

	iZoneUser = 0;
	InProMig = false;

	for( int i = 0 ; i < MAX_SYNC_OBJ ; ++i )
	{
		tu[i].used = false;

	}
}

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

bool SendMXPacket(char *pBuffer, int size)
{
	sendto( sMXSocket , (char *)pBuffer , size , 0 ,(struct sockaddr *)&addrtomx , sizeof(struct sockaddr));
	return true;
}

void BroadCastPacketLogOff ( int uid )
{
	int j = FindUserID ( uid );
	Users[j].Destroy();

	if( !Users[j].bGhost )
	{
		ZSBroadUserLogoff zbul;
		zbul.iUid_S = uid;

		for(int i = 0 ; i < MAX_USERS_ZONE ; ++i)
		{
			if(Users[i].bConnected && !Users[i].bMig)
			{
				zbul.iUid_D = Users[i].iUid;
				SendMXCommand((char *)&zbul , sizeof(ZSBroadUserLogoff));
			}
		}
	}
}

void BroadCastPacket ()
{
	if( iLevel != 0 )
	{
		for(int i = 0 ; i < MAX_USERS_ZONE ; ++i)
		{
			if( Users[i].bConnected )
			{				
				if( Users[i].GetDirtyBit() && !Users[i].bGhost )
				{
					ZSUSInfoUpdateToMX zuum;
					zuum.iUid_S = Users[i].iUid;
					zuum.fPos[0] = Users[i].fX;
					zuum.fPos[1] = Users[i].fY;
					zuum.iHP_S = Users[i].iHP;
					zuum.iMP_S = Users[i].iMP;

					//zuum.iHP_S = -1;
					//zuum.iMP_S = -1;

					ZSUSComToUS zucu;
					if( Users[i].bPoi )
					{
						if((fNowTime - Users[i].fPoiTime) > 1.0f )
						{
							Users[i].iHP -= 1;
							if( Users[i].iHP < 0 )
							{
								Users[i].iHP = 0;
								Users[i].bPoi = false;
							}else
							{
								Users[i].SetDirtyBit();
								Users[i].fPoiTime = fNowTime;

								zucu.cCom = UserCom::C_ATTACK;
								zucu.iNum = 1;
								zucu.iUid_S = Users[i].iUid;
								zucu.iZid_S = iZid;
							}
						}
					}

					for(int j = 0 ; j < MAX_USERS_ZONE ; ++j)
					{
						if(Users[j].bConnected)
						{
							if(abs(Users[i].fX - Users[j].fX)<USER_VISION_MAX && abs(Users[i].fY - Users[j].fY)<USER_VISION_MAX)
							{
								zuum.iUid_D = Users[j].iUid;
								SendMXPacket( (char *)&zuum, sizeof(zuum) );

								if( Users[i].bPoi )
								{
									zucu.iUid_D = Users[j].iUid;
									SendMXCommand( (char *)&zucu, sizeof( zucu ) );
								}
							}else
							{
								ZSBroadUserLogoff zbul;
								zbul.iUid_S = Users[i].iUid;
								zbul.iUid_D = Users[j].iUid;
								SendMXCommand((char *)&zbul , sizeof(ZSBroadUserLogoff));
							}
						}
					}
					Users[i].ResetDirtyBit();
				}
			}
		}
	}
}

bool ProcMig( User *pUser )
{
	if( pUser->fX > iZoneX+iZoneW )
	{
		if( pUser->fY < iZoneY )
		{
			pUser->bMig = true;
			if( zstable[2].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[2].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;

		}else if( pUser->fY > iZoneY+iZoneH )
		{
			pUser->bMig = true;
			if( zstable[8].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[8].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}else
		{
			pUser->bMig = true;
			if( zstable[5].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[5].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}
	}else if( pUser->fX < iZoneX )
	{
		if( pUser->fY < iZoneY )
		{
			pUser->bMig = true;
			if( zstable[0].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[0].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}else if( pUser->fY > iZoneY+iZoneH )
		{
			pUser->bMig = true;
			if( zstable[6].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[6].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}else
		{
			pUser->bMig = true;
			if( zstable[3].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[3].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}
	}else
	{
		if( pUser->fY < iZoneY )
		{
			pUser->bMig = true;
			if( zstable[1].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[1].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}else if( pUser->fY > iZoneY+iZoneH )
		{
			pUser->bMig = true;
			if( zstable[7].zid == -1 )
			{
				printf("no this zone\n");
				return false;
			}
			
			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			zmg.iZid = zstable[7].zid;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			return true;
		}else
		{
			pUser->bMig = false;
			return true;
		}
	}
}

//---------------------------------------------------buffer area--------------------------------------------------
/*
bool ProcMig( User *pUser )
{
	pUser->bGhost = false;

	if( pUser->iLastPlace == 5 )
	{
		switch( pUser->iPlace )
		{
		case 1:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE1_X;
			zmg.fPos[1] = MAP_POS_CASE1_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );

			zmg.fPos[0] = MAP_POS_CASE2_X;
			zmg.fPos[1] = MAP_POS_CASE2_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );

			zmg.fPos[0] = MAP_POS_CASE4_X;
			zmg.fPos[1] = MAP_POS_CASE4_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			   }break;
		case 2:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE2_X;
			zmg.fPos[1] = MAP_POS_CASE2_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			   }break;
		case 3:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE2_X;
			zmg.fPos[1] = MAP_POS_CASE2_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );

			zmg.fPos[0] = MAP_POS_CASE3_X;
			zmg.fPos[1] = MAP_POS_CASE3_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );

			zmg.fPos[0] = MAP_POS_CASE6_X;
			zmg.fPos[1] = MAP_POS_CASE6_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			   }break;
		case 4:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE4_X;
			zmg.fPos[1] = MAP_POS_CASE4_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			   }break;
		case 6:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE6_X;
			zmg.fPos[1] = MAP_POS_CASE6_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			   }break;
		case 7:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE4_X;
			zmg.fPos[1] = MAP_POS_CASE4_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			zmg.fPos[0] = MAP_POS_CASE7_X;
			zmg.fPos[1] = MAP_POS_CASE7_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			zmg.fPos[0] = MAP_POS_CASE8_X;
			zmg.fPos[1] = MAP_POS_CASE8_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			   }break;
		case 8:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE8_X;
			zmg.fPos[1] = MAP_POS_CASE8_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			   }break;
		case 9:{
			pUser->bMig = true;

			ZSUSMigToGS zmg;
			zmg.iUid = pUser->iUid;
			//strcpy(zmg.sUserName , uutz->sUserName);
			zmg.fPos[0] = MAP_POS_CASE6_X;
			zmg.fPos[1] = MAP_POS_CASE6_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			zmg.fPos[0] = MAP_POS_CASE8_X;
			zmg.fPos[1] = MAP_POS_CASE8_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			zmg.fPos[0] = MAP_POS_CASE9_X;
			zmg.fPos[1] = MAP_POS_CASE9_Y;
			SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			   }break;
		}
	}else if( pUser->iLastPlace == 1 || pUser->iLastPlace == 3 || pUser->iLastPlace == 7 || pUser->iLastPlace == 9 ){
		switch( pUser->iPlace )
		{
		case 5:{
			pUser->bMig = false;
			return true;
			   };
		default:
			{
				return true;
			}break;
		}
	}else if( pUser->iLastPlace == 2 || pUser->iLastPlace == 4 || pUser->iLastPlace == 6 || pUser->iLastPlace == 8 ){
		switch( pUser->iPlace )
		{
		case 5:{
			pUser->bMig = false;
			return true;
			   }break;
		case 1:
			{
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;

				if( pUser->iLastPlace == 2 )
				{
					zmg.fPos[0] = MAP_POS_CASE4_X;
					zmg.fPos[1] = MAP_POS_CASE4_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}else if( pUser->iLastPlace == 4 )
				{
					zmg.fPos[0] = MAP_POS_CASE2_X;
					zmg.fPos[1] = MAP_POS_CASE2_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}

				zmg.fPos[0] = MAP_POS_CASE1_X;
				zmg.fPos[1] = MAP_POS_CASE1_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			}break;
		case 2:
			{	
				if( pUser->iPlace == pUser->iLastPlace )
					break;
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);
				zmg.fPos[0] = MAP_POS_CASE2_X;
				zmg.fPos[1] = MAP_POS_CASE2_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );					
			}break;
		case 3:
			{
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);

				if( pUser->iLastPlace == 2 )
				{
					zmg.fPos[0] = MAP_POS_CASE6_X;
					zmg.fPos[1] = MAP_POS_CASE6_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}else if( pUser->iLastPlace == 6 )
				{
					zmg.fPos[0] = MAP_POS_CASE2_X;
					zmg.fPos[1] = MAP_POS_CASE2_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}

				zmg.fPos[0] = MAP_POS_CASE3_X;
				zmg.fPos[1] = MAP_POS_CASE3_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			}break;
		case 4:
			{
				if( pUser->iPlace == pUser->iLastPlace )
					break;
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);
				zmg.fPos[0] = MAP_POS_CASE4_X;
				zmg.fPos[1] = MAP_POS_CASE4_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			}break;
		case 6:
			{
				if( pUser->iPlace == pUser->iLastPlace )
					break;
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);
				zmg.fPos[0] = MAP_POS_CASE6_X;
				zmg.fPos[1] = MAP_POS_CASE6_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			}break;
		case 7:
			{
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);

				if( pUser->iLastPlace == 4 )
				{
					zmg.fPos[0] = MAP_POS_CASE8_X;
					zmg.fPos[1] = MAP_POS_CASE8_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}else if( pUser->iLastPlace == 8 )
				{
					zmg.fPos[0] = MAP_POS_CASE4_X;
					zmg.fPos[1] = MAP_POS_CASE4_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}

				zmg.fPos[0] = MAP_POS_CASE7_X;
				zmg.fPos[1] = MAP_POS_CASE7_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
			}break;
		case 8:
			{
				if( pUser->iPlace == pUser->iLastPlace )
					break;
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);
				zmg.fPos[0] = MAP_POS_CASE8_X;
				zmg.fPos[1] = MAP_POS_CASE8_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );					
			}break;
		case 9:
			{
				ZSUSMigToGS zmg;
				zmg.iUid = pUser->iUid;
				//strcpy(zmg.sUserName , uutz->sUserName);

				if( pUser->iLastPlace == 6 )
				{
					zmg.fPos[0] = MAP_POS_CASE8_X;
					zmg.fPos[1] = MAP_POS_CASE8_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}else if( pUser->iLastPlace == 8 )
				{
					zmg.fPos[0] = MAP_POS_CASE6_X;
					zmg.fPos[1] = MAP_POS_CASE6_Y;
					SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );
				}

				zmg.fPos[0] = MAP_POS_CASE9_X;
				zmg.fPos[1] = MAP_POS_CASE9_Y;
				SendTCPPacket( sGSSocket , (char *)&zmg , sizeof(zmg) );	
			}break;
		}		
	}
}

int DeterPlace( User *pUser )
{
	// check if user still in mig scope
	if( (pUser->fX >= (float)(iZoneX+iZoneW+VISIBLE_SCOPE)) || (pUser->fX < (float)(iZoneX-VISIBLE_SCOPE)) || (pUser->fY >= (float)(iZoneY+iZoneH+VISIBLE_SCOPE)) || (pUser->fY < (float)(iZoneY-VISIBLE_SCOPE)) )
	{
		pUser->iLastPlace = pUser->iPlace;
		pUser->iPlace = -1;
		pUser->bGhost = true;
		pUser->bMig = false;

		return pUser->iPlace;
	}
	// 1. deter the present place
	if( (pUser->fX >= (float)iZoneX) && (pUser->fX < (float)(iZoneX+VISIBLE_SCOPE)) )
	{
		if( (pUser->fY >= (float)iZoneY) && (pUser->fY < (float)(iZoneY+VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 1;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 7;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 4;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}
	}else if( (pUser->fX >= (float)(iZoneX+iZoneW-VISIBLE_SCOPE)) && (pUser->fX < (float)(iZoneX+iZoneW)) )
	{
		if( (pUser->fY >= (float)iZoneY) && (pUser->fY < (float)(iZoneY+VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 3;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 9;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 6;
			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}
	}else if( (pUser->fX >= (float)(iZoneX+VISIBLE_SCOPE)) && (pUser->fX < (float)(iZoneX+iZoneW-VISIBLE_SCOPE)) )
	{
		if( (pUser->fY >= iZoneY) && (pUser->fY < (iZoneY+VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 2;

			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 8;

			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}else if( (pUser->fY >= (float)(iZoneY+VISIBLE_SCOPE)) && (pUser->fY < (float)(iZoneY+iZoneH-VISIBLE_SCOPE)) )
		{
			pUser->iLastPlace = pUser->iPlace;
			pUser->iPlace = 5;

			if( pUser->iLastPlace != pUser->iPlace )
				ProcMig( pUser );
			return pUser->iPlace;
		}
	}
	pUser->iLastPlace = pUser->iPlace;
	pUser->iPlace = 0;
	pUser->bGhost = true;

	return 0;
}
*/
//---------------------------------------------------buffer area--------------------------------------------------

void renderBitmapString( float x, float y, char *string)
{  
	int len, i;
	glRasterPos3f(x, y,0);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, (int)string[i]);
	}
}

void displayCB(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPointSize(10.0f);
	glColor3f(1.0, 1.0, 1.0);	
	for(int i = 0;i < MAX_USERS_ZONE;i++)
	{
		if(Users[i].bConnected)
		{
			char hpmp[30];
			sprintf( hpmp, "%d:H%d/%d:M%d/%d" ,Users[i].iUid , Users[i].iHP,MAX_HP , Users[i].iMP,MAX_MP );
			glPushMatrix();
			renderBitmapString(Users[i].fX-(float)iZoneX-30,Users[i].fY-(float)iZoneY-8,hpmp);
			glBegin(GL_POINTS);
			glVertex2f(Users[i].fX-(float)iZoneX,Users[i].fY-(float)iZoneY);
			glEnd();
			glPopMatrix();
		}
	}
	glutSwapBuffers();

}

void keyCB(unsigned char key, int x, int y)
{
	if( key == 'q' ) 
	{
		ExitThread(0);
	}
}

DWORD WINAPI glut_Thread( LPVOID pParam )
{	
	int win;
	char titlename[12];
	ZeroMemory( titlename,12 );
	sprintf(titlename,"zone_id:%d",iZid);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(MAP_RANGE_MAX,MAP_RANGE_MAX);
	win = glutCreateWindow(titlename);

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, MAP_RANGE_MAX, MAP_RANGE_MAX, 0 );

	glClearColor(0.0,0.0,0.0,0.0);

	glutDisplayFunc(displayCB);
	glutIdleFunc(displayCB);
	glutKeyboardFunc(keyCB);

	glutMainLoop();


	return S_OK;
}

void ProcessUSCom( User *pUser , int cCom ,int uid_s, int irand )
{
	switch(cCom)
	{
	case C_ATTACK:
		{
			if(pUser->iHP <= 0)
			{
				printf("user %d already die....\n",pUser->iUid);
				return ;
			}
			//int hurt = rand()%10+1;
			int hurt = 1;
			pUser->iHP -= hurt;

			if(pUser->iHP < 0)
				pUser->iHP = 0;
			
			pUser->SetDirtyBit();

			if( iLevel != 0 )
			{
				printf("return to user\n");
				ZSUSComToUS zucu;
				zucu.iUid_S = uid_s;
				zucu.iUid_D = pUser->iUid;
				zucu.iZid_S = iZid;
				zucu.iNum = hurt;
				zucu.cCom = (UserCom)cCom;

				SendMXCommand( (char *)&zucu, sizeof(zucu) );
			}
		}break;
	case C_POISON:
		{
			int i = FindUserID( uid_s );
			printf("user %d poison user %d!!\n",uid_s,Users[i].iUid);
			if( (Users[i].iMP >= 20) && (!pUser->bPoi) )
			{
				Users[i].iMP -= 20;
				pUser->bPoi = true;
				pUser->fPoiTime = GetTickCount() / 1000.0f;
				pUser->SetDirtyBit();

				return ;
			}
			printf("user %d have no enough mp or already poisoned\n",uid_s);
		}break;
	case C_RECOVER:
		{
			printf("user %d recover!!\n",pUser->iUid);
			pUser->iHP = MAX_HP;
			pUser->bPoi = false;

			if( iLevel != 0 )
			{
				printf("return to user\n");
				ZSUSComToUS zucu;
				zucu.iUid_S = uid_s;
				zucu.iUid_D = pUser->iUid;
				zucu.iZid_S = iZid;
				zucu.iNum = MAX_HP;
				zucu.cCom = (UserCom)cCom;

				SendMXCommand( (char *)&zucu, sizeof(zucu) );
			}
		}break;
	}
}

int main( int argc,char *argv[] )
{
	Initial( );	
	InitialConfig();
	srand( GetTickCount() );
	my_init_xdiff();

	char zone_ip[MAX_IP_LEN];
	char sZoneName[MAX_NAME_LEN];
	char zone_port[MAX_PORT_LEN];

	strcpy ( zone_ip , argv[1] );
	strcpy ( zone_port , argv[2] );
	iZid = atoi( argv[3] );
	strcpy ( sZoneName , argv[4] );
	iZoneX = atoi(argv[5]);
	iZoneY = atoi(argv[6]);
	iZoneW = atoi(argv[7]);
	iZoneH = atoi(argv[8]);
	iLevel = atoi(argv[9]);

	IniZsTable( iZid, atoi(argv[10]) );

	printf("%s zid %d ip %s, port %s,x=%d,y=%d,w=%d,h=%d\n",sZoneName,iZid,zone_ip,zone_port,iZoneX,iZoneY,iZoneW,iZoneH );
	if(SHOW_WINDOW)
		if( NULL == CreateThread( NULL, 0, glut_Thread, NULL, 0, NULL ))
			return -1;
	
	WSADATA wsaData;
	if( SOCKET_ERROR == WSAStartup( WINSOCK_VERSION, &wsaData ) || wsaData.wVersion != WINSOCK_VERSION )
		return -1;

	sGSSocket = socket( AF_INET, SOCK_STREAM, 0 );

	struct sockaddr_in addr3;
	addr3.sin_family = AF_INET;
	addr3.sin_port = htons( atoi(gs_port)+1 );
	addr3.sin_addr.S_un.S_addr = inet_addr(gs_ip);

	connect( sGSSocket , (struct  sockaddr *)&addr3, sizeof(struct sockaddr) );

	ZSOpenToGS zog;
	zog.iZid = iZid;
	SendTCPPacket( sGSSocket , (char *)&zog , sizeof(ZSOpenToGS) );

	sMXSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	addrtomx.sin_family = AF_INET;
	addrtomx.sin_port = htons( atoi(mx_port)+1 );
	addrtomx.sin_addr.S_un.S_addr = inet_addr(mx_ip);

	ZSUpdateToMX zum;
	zum.iZid = iZid;
	strcpy ( zum.sZip , zone_ip );
	strcpy ( zum.sZport , zone_port );

	SendMXCommand( (char *)&zum	 , sizeof(zum) );

	sSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( atoi(zone_port) );
	addr.sin_addr.S_un.S_addr = inet_addr(zone_ip);

	if( SOCKET_ERROR == bind( sSocket, (struct  sockaddr *)&addr, sizeof(struct sockaddr) ) )
		return -1;

	listen( sSocket, MAX_WAIT_LOG );
	
	sZScSocket = socket( AF_INET, SOCK_STREAM, 0 );

	struct sockaddr_in addr2;
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons( atoi(zone_port)+1 );
	addr2.sin_addr.S_un.S_addr = inet_addr(zone_ip);

	if( SOCKET_ERROR == bind( sZScSocket, (struct  sockaddr *)&addr2, sizeof(struct sockaddr) ) )
		return -1;

	listen( sZScSocket, MAX_WAIT_LOG );	
	{
		short int nready;
		struct timeval timeout;

		FD_ZERO(&rset);
		FD_SET(sSocket, &rset);
		FD_SET(sGSSocket, &rset);
		FD_SET(sZScSocket, &rset);

		maxfd = std::max(sGSSocket , sSocket);
		maxfd = std::max(sZScSocket , maxfd);

		while(true) 
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = 60000;
			FD_ZERO(&temprset);
			temprset = rset;

			fNowTime = GetTickCount() / 1000.0f;

			if((fNowTime - fLastUpdate) > SEND_PACKET / 1000.0f)
			{
				BroadCastPacket();
				fLastUpdate = GetTickCount() / 1000.0f;
			}
		
			if( (nready = select(maxfd, &temprset, NULL, NULL, &timeout)) < 0)
			{
				printf("select error %d\n", WSAGetLastError());
				break;
			}else if( nready == 0)
			{			
				fNowTime = GetTickCount() / 1000.0f;

				if((fNowTime - fLastUpdate) > SEND_PACKET / 1000.0f)
				{
					BroadCastPacket();

					fLastUpdate = GetTickCount() / 1000.0f;
				}
			}

			if (FD_ISSET(sSocket, &temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				ZeroMemory( buffer, sizeof(buffer) );

				struct sockaddr address;
				int alen = sizeof(struct sockaddr);
				int len = recvfrom( sSocket , (char *)&buffer , sizeof(buffer) , 0, (struct sockaddr *)&address,&alen);
				MessageHeader * tMh = (MessageHeader*)buffer;			

				switch( tMh->MsgID )
				{
				case MSG_USERLOGINTOZS:
					{
						UserLoginToZS * ultz = (UserLoginToZS *)&buffer;
						bool check = false;

						for( int i = 0 ; i < MAX_USERS_ZONE ; ++i)
						{
							if( !Users[i].bConnected )
							{
								printf("user login\n");
								check = true;
								Users[i].Create( ultz->iUid, false );

								ZSUSInfoUpdateToMX zium;
								zium.iUid_D = ultz->iUid;
								zium.iUid_S = ultz->iUid;

								int j = FindUserDataID(ultz->iUid);
								Users[i].fX = UserData[j].fX;
								Users[i].fY = UserData[j].fY;
								Users[i].iHP = UserData[j].iHP;
								Users[i].iMP = UserData[j].iMP;
								zium.fPos[0] = Users[i].fX;
								zium.fPos[1] = Users[i].fY;
								zium.iHP_S = Users[i].iHP;
								zium.iMP_S = Users[i].iMP;

								ProcMig( &Users[i] );
								
								//----------------------buffer area-------------------
								//Users[i].iPlace = 5;
								//DeterPlace( &Users[i] );
								//----------------------------------------------------

								for(int i = 0 ; i < MAX_USERS_ZONE ; ++i)
								{
									if(Users[i].bConnected)
										Users[i].SetDirtyBit();
								}
								SendMXPacket ((char *)&zium , sizeof(zium) );
								
								if( iZoneUser >= 10 && !InProMig )
								{
									InProMig= true;

									//------------------test thread------------------
									//CreateThread( NULL, 0, test_Thread, NULL, 0, NULL );
									//-----------------------------------------------
									
									ZSProMigGS zpmg;
									zpmg.iZid = iZid;

									SendTCPPacket( sGSSocket, (char *)&zpmg, sizeof(zpmg) );
									//Print_Users();
								}

								break;
							}
						}

						if(!check)
							printf("No enough space or already exist for new players!!\n");
					}break;

				case MSG_USERUPDATETOZS:
					{
						UserUpdateToZS * uutz = (UserUpdateToZS *)&buffer;

						for( int i = 0 ; i < MAX_USERS_ZONE ; ++i)
						{
							if( (uutz->iUid == Users[i].iUid) && Users[i].bConnected )
							{
								Users[i].fX = uutz->fPos[0];
								Users[i].fY = uutz->fPos[1];

								if( !Users[i].bMig )
									ProcMig( &Users[i] );
								//----------------------buffer area-----------------------
								/*
								int pla = DeterPlace( &Users[i] );
								if( pla < 0 )
								{
									BroadCastPacketLogOff( uutz->iUid );

									ZSMigOfflineToMX zmom;
									zmom.iUid = uutz->iUid;
									zmom.iZid = iZid;

									SendMXCommand( (char *)&zmom , sizeof(ZSMigOfflineToMX) );
									break;
								}
								
								if( Users[i].iPlace >= 0 )
									Users[i].SetDirtyBit();
								*/
								//---------------------------------------------------------
								break;
							}
						}

					}break;

				case MSG_USERCOMTOZS:
					{
						UserComToZS * ucz = (UserComToZS *)&buffer;

						for( int i = 0 ; i < MAX_USERS_ZONE ; ++i)
						{
							if( (ucz->iUid_D == Users[i].iUid) && Users[i].bConnected)
							{
								if( Users[i].iTime >= ucz->iTime )
									break;
								if( Users[i].bGhost )
								{
									AddPacket( ucz->iUid_D, (char *)ucz , ucz->iTime );
									//printf("buf uid %d com packet time %d!!\n",ucz->iUid_D, ucz->iTime);
								}

								Users[i].iTime = ucz->iTime;
								//printf("process uid %d com packet time %d!!\n",ucz->iUid_D, ucz->iTime);
								ProcessUSCom( &Users[i] , ucz->cCom, ucz->iUid_S, 0 );
								Print_Users();
								break;
							}
						}
					}break;

				case MSG_USERLOGOFF:
					{
						UserLogoff * ul = (UserLogoff *)buffer;
						BroadCastPacketLogOff( ul->iUid );
						SendTCPPacket( sGSSocket , (char *)ul , sizeof(ul) );
					}break;
				}
			}

			if (FD_ISSET(sGSSocket,&temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				ZeroMemory( buffer, sizeof(buffer) );
				int len = RecvTCPPacket( sGSSocket , (char *)&buffer );
				MessageHeader * tMh = (MessageHeader*)buffer;			

				switch( tMh->MsgID )
				{
				case MSG_GSUSMIGTOZS:
					{
						
						//printf("recv from gs\n");
						GSUSMigToZS *gmz = (GSUSMigToZS *)buffer;

						ZSMigUpdateToMX zmum;
						zmum.iUid = gmz->iUid;
						zmum.iZid = gmz->iZid;
						strcpy( zmum.sZip , gmz->sZip );
						strcpy( zmum.sZport , gmz->sZport );
						SendMXCommand( (char *)&zmum , sizeof(ZSMigUpdateToMX) );

						//printf("send mx\n");
						ZSUSMigToZS zumz;
						zumz.iUid = gmz->iUid;
						zumz.iZid = iZid;
						zumz.MigHeader = Migstate::M_MIGREQ;				
						
						//---------send to new zone------------------
					
						int sockfd = socket( AF_INET, SOCK_STREAM, 0 );

						struct sockaddr_in addrt;
						addrt.sin_family = AF_INET;
						addrt.sin_port = htons( atoi(gmz->sZport)+1 );
						addrt.sin_addr.S_un.S_addr = inet_addr(gmz->sZip);

						connect( sockfd , (struct  sockaddr *)&addrt, sizeof(struct sockaddr) );
						AddZsTable( gmz->iZid, sockfd );
						FD_SET( sockfd, &rset );
						maxfd = std::max( maxfd, sockfd );
						///printf("send req\n");
						SendTCPPacket( sockfd , (char *)&zumz , sizeof(zumz) );
						
						//------------------------------------------------
					}break;

				case MSG_ACKZSPROMIGGS:
					{
						ACKZSProMigGS *apmg = (ACKZSProMigGS *)buffer;

						int sockfd = socket( AF_INET, SOCK_STREAM, 0 );

						struct sockaddr_in addrt;
						addrt.sin_family = AF_INET;
						addrt.sin_port = htons( atoi(apmg->sZport)+1 );
						addrt.sin_addr.S_un.S_addr = inet_addr(apmg->sZip);

						connect( sockfd , (struct  sockaddr *)&addrt, sizeof(struct sockaddr) );

						ZSUSMigToZS zumz;
						zumz.MigHeader = Migstate::M_PROREQ;
						zumz.iUid = iZoneUser;
						zumz.iZid = iZid;

						char *usr = (char *)malloc(iZoneUser*3+1);
						//char usr[50];
						//ZeroMemory( usr, iZoneUser*3+1 );
						int point = 0;

						//ZSMigUpdateToMX zmum;
						//zmum.iZid = apmg->iZid;
						//strcpy( zmum.sZip , apmg->sZip );
						//strcpy( zmum.sZport , apmg->sZport );

						for( int i = 0 ; i<MAX_USERS_ZONE ; ++i )
						{
							if(Users[i].bConnected)
							{
								sprintf( usr+point,"%2d ",Users[i].iUid );
								point += 3;

								//zmum.iUid = Users[i].iUid;
								//SendMXCommand( (char *)&zmum , sizeof(ZSMigUpdateToMX) );
								//printf("send to mx update uid %d\n",Users[i].iUid);
							}
						}

						AddZsTable( apmg->iZid, sockfd );
						FD_SET( sockfd, &rset );
						maxfd = std::max( maxfd, sockfd );
						//printf("send pro req\n");
						SendTCPPacket( sockfd , (char *)&zumz , sizeof(zumz) );
						send( sockfd , usr, iZoneUser*3, 0 );
						free( usr );
					}break;
				}
				continue;
			}

			if (FD_ISSET(sZScSocket,&temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				struct sockaddr address;
				int len = sizeof(struct sockaddr);

				ZeroMemory( buffer, sizeof(buffer) );

				int sockfd = accept ( sZScSocket, (struct sockaddr *)&address, &len );
				RecvTCPPacket( sockfd, (char *)&buffer );
				MessageHeader * tMh = (MessageHeader*)buffer;	

				switch( tMh->MsgID )
				{
				case MSG_ZSUSMIGTOZS:
					{
						ZSUSMigToZS *zumz = (ZSUSMigToZS *)buffer;

						switch( zumz->MigHeader )
						{
						case Migstate::M_MIGREQ:
							{
								//printf("recv mig req from zid %d uid %d\n",zumz->iZid,zumz->iUid);
								AddZsTable( zumz->iZid, sockfd );
								FD_SET( sockfd, &rset );
								maxfd = std::max( maxfd, sockfd );

								bool check = false;

								for( int i = 0 ; i < MAX_USERS_ZONE ; ++i)
								{
									if( !Users[i].bConnected )
									{
										check = true;
										Users[i].Create( zumz->iUid, true );
										ACKZSUSMigToZS ack;
										ack.MigHeader = Migstate::M_MIGREQ;
										ack.iUid = zumz->iUid;
										ack.iZid = iZid;

										SendTCPPacket( sockfd, (char *)&ack, sizeof(ack));
										//printf("send req ack\n");
										break;
									}
								}
							}break;

						case Migstate::M_PROREQ:
							{
								AddZsTable( zumz->iZid, sockfd );
								FD_SET( sockfd, &rset );
								maxfd = std::max( maxfd, sockfd );

								char *usr = (char *)malloc(zumz->iUid*3);
								//ZeroMemory( usr, zumz->iUid*3 );
								//char usr[50];
								recv( sockfd, usr, zumz->iUid*3 , 0 );

								ACKZSUSMigToZS ack;
								ack.MigHeader = Migstate::M_MIGREQ;
								ack.iZid = iZid;

								char tec[3];
								int point = 0;
								int tempid;

								for( int i = 0 ; i < zumz->iUid ; ++i)
								{
									sscanf( usr+point,"%d", &tempid );
									point += 3;
									//printf("add new usr uid %d\n",tempid);
									AddTempUser( tempid );
									Users[i].Create( tempid, true );

									ack.iUid = tempid;
									SendTCPPacket( sockfd, (char *)&ack, sizeof(ack));
									//printf("send user uid %d req ack\n",Users[i].iUid);
								}
								
								free(usr);
							}break;
						}
						//------------------------------------------------
					}break;
				}
				continue;
			}

			for( int i = 0; i<9 ; ++i )
			{
				if( zstable[i].connect )
				{
					if( FD_ISSET(zstable[i].sock,&temprset) )
					{
						//printf("recv from zid %d\n",zstable[i].zid);
						char buffer[MAX_PACKET_SIZE];
						ZeroMemory( buffer, sizeof(buffer) );
						int len = 0;
						if( (len = RecvTCPPacket( zstable[i].sock , (char *)&buffer )) < 0 )
						{
							FD_CLR( zstable[i].sock, &rset );
							closesocket( zstable[i].sock );
							zstable[i].connect = false;
							continue;
						}
						MessageHeader * tMh = (MessageHeader*)buffer;			

						switch( tMh->MsgID )
						{
						case MSG_ZSUSMIGTOZS:
							{
								ZSUSMigToZS *zumz = (ZSUSMigToZS *)buffer;

								switch( zumz->MigHeader )
								{
								case Migstate::M_MIGFIN:
									{
										//printf("recv fin\n");
										int j = FindUserID( zumz->iUid );
										Users[j].bGhost = false;
										Users[j].bMig = false;
										int k = FindObj( zumz->iUid );
										
										ACKZSUSMigToZS ack;
										ack.MigHeader = Migstate::M_MIGFIN;
										ack.iZid = iZid;
										ack.iUid = zumz->iUid;

										SendTCPPacket( zstable[i].sock , (char *)&ack, sizeof(ack) );
										
										for( int i = 0 ; i<MAX_SYNC_OBJ ; ++i )
										{
											if(tu[i].used && (tu[i].uid == zumz->iUid))
											{
												while( !(tu[i].pac.empty()) )
												{
													//printf("there is cmd buf time %d obj time %d\n",tu[i].pac.back().time,sync_obj[k].time);
													tu[i].pac.pop_back();
												}
												break;
											}									
										}
										
										DelObj( zumz->iUid );
										DelTempUser( zumz->iUid );
										//printf("after final process cmd buf user %d\n",zumz->iUid);
										Print_Users();
									}break;
								case Migstate::M_PROFIN:
									{
										//printf("recv pro fin\n");										
										iLevel = 1;

										ACKZSUSMigToZS ack;
										ack.MigHeader = Migstate::M_PROFIN;
										ack.iZid = iZid;

										iZid = zumz->iZid;

										SendTCPPacket( zstable[i].sock , (char *)&ack, sizeof(ack) );
										//printf("send pro ack\n");

										ZSUpdateToMX zum;
										zum.iZid = iZid;
										strcpy( zum.sZip , zone_ip );
										strcpy( zum.sZport , zone_port );

										SendMXCommand( (char *)&zum, sizeof(zum) );
										//printf("update mx zone table's\n");
									}break;
								}
							}break;

						case MSG_ACKZSUSMIGTOZS:
							{
								ACKZSUSMigToZS *ack = (ACKZSUSMigToZS *)buffer;

								switch( ack->MigHeader )
								{
								case Migstate::M_MIGREQ:
									{
										//printf("recv req ack\n");
										char tmp[3*sizeof(User)];

										BinaryCharWriter syncwriter(tmp);
										int j = FindUserID( ack->iUid );
										Users[j].iMigFeq = 0;
										syncwriter.write( &Users[j] );
										
										ZSUSObjToZS zuoz;
										zuoz.iUid = ack->iUid;
										zuoz.ObjHeader = Objstate::M_OBJ;
										zuoz.iSize = syncwriter.bytecount;
										//printf("send obj user %d time %d\n",ack->iUid,Users[j].iTime);
										SendTCPPacket( zstable[i].sock, (char *)&zuoz, sizeof(zuoz) );
										SendTCPPacket( zstable[i].sock , tmp , zuoz.iSize );

										CreateObj( zuoz.iUid , tmp , zuoz.iSize );
									}break;
								case Migstate::M_MIGFIN:
									{
										if( !InProMig )
										{
											ZSUSZooGS zuzg;
											zuzg.iUid = ack->iUid;
											zuzg.iZid = ack->iZid;

											SendTCPPacket( sGSSocket, (char *)&zuzg, sizeof(zuzg) );
											//printf("send gs change loca\n");
											
											ZSMigOfflineToMX zmom;
											zmom.iUid = ack->iUid;
											zmom.iZid = iZid;

											SendMXCommand( (char *)&zmom, sizeof(zmom) );
											//printf("send mx del entry\n");

											int j = FindUserID( ack->iUid );
											Users[j].Destroy();
										}else
										{
											--iZoneUser;
											int j = FindUserID( ack->iUid );
											Users[j].bGhost = false;
											if( iZoneUser <= 0 )
											{
												//printf("zid %d Pro mig finish!!\n", iZid);

												ZSUSMigToZS zumz;
												zumz.iZid = iZid;
												zumz.MigHeader = Migstate::M_PROFIN;

												SendTCPPacket( zstable[i].sock, (char *)&zumz, sizeof(zumz) );

												iLevel = 0;

												ZSProDoneMX zpdm;
												zpdm.iZid_D = iZid;
												zpdm.iZid_S = ack->iZid;

												SendMXCommand( (char *)&zpdm, sizeof(zpdm) );
												//printf("send mx del entry\n");
												
												ZSProDoneGS zpdg;
												zpdg.iZid_S = iZid;
												zpdg.iZid_D = zstable[i].zid;

												SendTCPPacket( sGSSocket, (char *)&zpdg, sizeof(zpdg) );
												//system("pause");
												exit(0);
											}
										}

									}break;
								}
							}break;

						case MSG_ZSUSOBJTOZS:
							{
								ZSUSObjToZS *zuoz = (ZSUSObjToZS *)buffer;

								switch( zuoz->ObjHeader )
								{
								case Objstate::M_OBJ:
									{						
										char *tmp = (char *)malloc( zuoz->iSize );
										recv( zstable[i].sock , tmp , zuoz->iSize , 0 );
										CreateObj( zuoz->iUid , tmp , zuoz->iSize );
			
										User *ttu = (User *)malloc(sizeof(User));
										BinaryCharReader syncreader(tmp);
										syncreader.read( (ISerializable**)&ttu );

										int j = FindUserID( zuoz->iUid );
										memcpy( &Users[j], ttu, sizeof(User) );
										free( ttu );	
										int k = FindObj( zuoz->iUid );
										sync_obj[k].time = Users[j].iTime;

										ACKZSUSObjToZS ack;
										ack.ObjHeader = Objstate::M_OBJ;
										ack.iTime = Users[j].iTime;
										ack.iUid = zuoz->iUid;
										//printf("after recv obj user %d time %d\n", zuoz->iUid, Users[j].iTime);
										//Print_Users();
										//Sleep( 10 );

										SendTCPPacket( zstable[i].sock, (char *)&ack, sizeof(ack) );
										//printf("send obj ack\n");
										
										for( int i = 0 ; i<MAX_SYNC_OBJ ; ++i )
										{
											if(tu[i].used && (tu[i].uid == zuoz->iUid))
											{
												while( !(tu[i].pac.empty()) )
												{
													//printf("there is cmd buf time %d obj time %d\n",tu[i].pac.back().time,sync_obj[k].time);
													
													if( tu[i].pac.back().time <= sync_obj[k].time )
													{
														//printf("no exe\n");
														tu[i].pac.pop_back();
														continue;
													}else
													{
														//printf("exe\n");
														Users[j].iTime = tu[i].pac.back().time;
														ProcessUSCom( &Users[j], tu[i].pac.back().zucu.cCom, tu[i].pac.back().zucu.iUid_S,
															tu[i].pac.back().zucu.iNum );
														tu[i].pac.pop_back();
														continue;
													}
												}
												break;
											}									
										}

										//printf("after exe cmd buf\n");
										//Print_Users();
									}break;
								case Objstate::M_PATCH:
									{
										char *tmp = (char *)malloc( zuoz->iSize );
										recv( zstable[i].sock , tmp , zuoz->iSize , 0 );
										int j = FindObj(zuoz->iUid);
										int k = FindUserID(zuoz->iUid);

										mmfile_t mp;
										xdl_init_mmfile( &mp , zuoz->iSize , XDL_MMF_ATOMIC );
										xdl_write_mmfile( &mp , tmp , zuoz->iSize );

										xdl_seek_mmfile( &mp , 0 );
										ObjectPatch( mp , (char *)sync_obj[j].obj, &(sync_obj[j].size) );
				
										User *ttu = (User *)malloc(sizeof(User));
										BinaryCharReader syncreader(sync_obj[j].obj);
										syncreader.read( (ISerializable**)&ttu );
				
										memcpy( &Users[k], ttu, sizeof(User) );
										free( tmp );
										free( ttu );
										//printf("patch obj user %d time %d\n", zuoz->iUid, Users[k].iTime);
										
										sync_obj[j].time = Users[k].iTime;

										ACKZSUSObjToZS ack;
										ack.ObjHeader = Objstate::M_PATCH;
										ack.iTime = Users[k].iTime;
										ack.iUid = zuoz->iUid;
										
										//printf("send patch ack\n");
										SendTCPPacket( zstable[i].sock, (char *)&ack, sizeof(ack) );
										
										for( int i = 0 ; i<MAX_SYNC_OBJ ; ++i )
										{
											if(tu[i].used && (tu[i].uid == zuoz->iUid))
											{
												while( !(tu[i].pac.empty()) )
												{
													//printf("there is cmd buf time %d obj time %d\n",tu[i].pac.back().time,sync_obj[k].time);
													
													if( tu[i].pac.back().time <= sync_obj[k].time )
													{
														//printf("no exe\n");
														tu[i].pac.pop_back();
														continue;
													}else
													{
														//printf("exe\n");
														Users[j].iTime = tu[i].pac.back().time;
														ProcessUSCom( &Users[j], tu[i].pac.back().zucu.cCom, tu[i].pac.back().zucu.iUid_S,
															tu[i].pac.back().zucu.iNum );
														tu[i].pac.pop_back();
														continue;
													}
												}
												break;
											}									
										}

										//printf("after exe cmd buf\n");
										//Print_Users();
									}break;
								}
							}break;

						case MSG_ACKZSUSOBJTOZS:
							{
								ACKZSUSObjToZS *ack = (ACKZSUSObjToZS *)buffer;

								switch( ack->ObjHeader )
								{
								case Objstate::M_OBJ:
									{
										int j = FindUserID( ack->iUid );
										//printf("recv user[%d] %d obj ack\n",j,ack->iUid);	
										++Users[j].iMigFeq;
										if( (Users[j].iMigFeq > 3) || (Users[j].iTime <= ack->iTime) )
										{
											ZSUSMigToZS zumz;
											zumz.iUid = ack->iUid;
											zumz.iZid = iZid;
											zumz.MigHeader = Migstate::M_MIGFIN;

											//printf("user %d no need patch send fin\n",ack->iUid);
											SendTCPPacket( zstable[i].sock, (char *)&zumz, sizeof(zumz) );
											break;
										}
										char tmp[3*sizeof(User)];
										BinaryCharWriter syncwriter(tmp);
										syncwriter.write( &Users[j] );

										int k = FindObj( ack->iUid );
										mmfile_t mp;

										xdl_init_mmfile(&mp, std::max(sync_obj[k].size,syncwriter.bytecount), XDL_MMF_ATOMIC);
										mp = ObjectDiff( sync_obj[k].obj , sync_obj[k].size , tmp, syncwriter.bytecount );

										UpdateObj( k, tmp, syncwriter.bytecount, Users[j].iTime);

										ZSUSObjToZS zuoz;
										zuoz.iUid = ack->iUid;
										zuoz.iTime = Users[j].iTime;
										zuoz.ObjHeader = Objstate::M_PATCH;
										zuoz.iSize = xdl_mmfile_size(&mp);

										char *tpatch = (char *)malloc(zuoz.iSize);

										xdl_read_mmfile( &mp , tpatch , zuoz.iSize );
										//printf("send patch obj user %d time %d\n",ack->iUid,Users[j].iTime);
										SendTCPPacket( zstable[i].sock, (char *)&zuoz, sizeof(zuoz) );
										SendTCPPacket( zstable[i].sock, tpatch, zuoz.iSize );

										free( tpatch );
									}break;
								case Objstate::M_PATCH:
									{
										//printf("recv patch ack\n");
										int j = FindUserID( ack->iUid );
										++Users[j].iMigFeq;
										if( Users[j].iMigFeq > 3 || Users[j].iTime <= ack->iTime )
										{
											ZSUSMigToZS zumz;
											zumz.iUid = ack->iUid;
											zumz.iZid = iZid;
											zumz.MigHeader = Migstate::M_MIGFIN;

											SendTCPPacket( zstable[i].sock, (char *)&zumz, sizeof(zumz) );
											//printf("send fin over 3 times\n");
											break;
										}
										PrintObj();
										char tmp[3*sizeof(User)];
										
										BinaryCharWriter syncwriter(tmp);
										syncwriter.write( &Users[j] );
										
										int k = FindObj( ack->iUid );

										mmfile_t mp;

										xdl_init_mmfile(&mp, std::max(sync_obj[k].size,syncwriter.bytecount), XDL_MMF_ATOMIC);
										//printf("diff input %d %d\n",sync_obj[k].size,syncwriter.bytecount);
										mp = ObjectDiff( sync_obj[k].obj , sync_obj[k].size , tmp, syncwriter.bytecount );
										
										UpdateObj( k, tmp, syncwriter.bytecount, Users[j].iTime);

										ZSUSObjToZS zuoz;
										zuoz.iUid = ack->iUid;
										zuoz.iTime = Users[j].iTime;
										zuoz.ObjHeader = Objstate::M_PATCH;
										zuoz.iSize = xdl_mmfile_size(&mp);

										char *tpatch = (char *)malloc(zuoz.iSize);

										xdl_read_mmfile( &mp , tpatch , zuoz.iSize );
										//printf("send patch obj user %d time %d\n",ack->iUid,Users[j].iTime);
										SendTCPPacket( zstable[i].sock, (char *)&zuoz, sizeof(zuoz) );
										SendTCPPacket( zstable[i].sock, tpatch, zuoz.iSize );

										free( tpatch );
									}break;
								}
							}break;
						}

					}
				}
			}

		}
	}

	for( int i = 0; i < MAX_USERS_ZONE; ++i )
	{
		if(Users[i].bConnected)
			Users[i].Destroy();
	}
	closesocket( sSocket );
	closesocket( sGSSocket );
	closesocket( sZScSocket );
	WSACleanup();

	return 0;
} 