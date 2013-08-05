#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <time.h>
#include <math.h>
#include "net.h"
#include "glut/glut.h"

#define UPDATE_FREQUENCY    50

#define V_RANGE_MAX			3								
#define V_RANGE_MIN			1

#define RANDOM_SCOPE		150
#define RANDOM_DEGREE		360
//-----------------------GUI------------------------------------
int			iSelected = -1;
char		con;
//--------------------------------------------------------------

//-----------------------zone setting---------------------------
//zone range
int			iZoneX;
int			iZoneY;
int			iZoneW;
int			iZoneH;

float		fLastUpdate;
float		fNowTime;
float		fTime;

int			sMXSocket;
fd_set		rset,temprset;

//----------------------------------setting-----------------------------------------------

char gs_ip[MAX_IP_LEN];
char gs_port[MAX_PORT_LEN];

char mx_ip[MAX_IP_LEN];
char mx_port[MAX_PORT_LEN];

//----------------------------------------------------------------------------------------


void Initial( )
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

			printf("GS ip = %s , port = %s\n" , buf_ip, buf_port);

			strcpy( gs_ip, buf_ip );
			strcpy( gs_port, buf_port );

		}else if (strcmp( buf , "MX") == 0)
		{
			fgetc( fp );
			fscanf( fp, "%s %s" , &buf_ip, &buf_port );
			printf("MX ip = %s , port = %s\n" , buf_ip, buf_port);

			strcpy( mx_ip, buf_ip );
			strcpy( mx_port, buf_port );
		}

	}while(fgets( (char *)&buf, 3, fp)!=NULL);

	fclose(fp);
}

struct Player
{
	int			iZid;
	int			iUid;
	char		sUserName[MAX_NAME_LEN];
	float		fTargetX;
	float		fTargetY;
	float		fVel[2];
	float		fCurrentX;
	float		fCurrentY;
	float		fPos[2];
	char		sIP[MAX_IP_LEN];
	char		sPort[MAX_PORT_LEN];
	int			iState;					//0 = think, 1 = move, 2 = wait
	int			iHP;
	int			iMP;
};

Player player;

struct OtherPlayer
{
	int				iUid;
	bool			bOnline;
	char			sUserName[MAX_NAME_LEN];
	float			fPos[2];
	int				iHP;
	int				iMP;
};

OtherPlayer players[MAX_USERS_CLI];

void Print_Players()
{
	for(int i = 0;i<MAX_USERS_CLI;++i)
	{
		if(players[i].bOnline)
		{
			printf("players [%d] , x = %f , y = %f\n",i,players[i].fPos[0],players[i].fPos[1]);
		}
	}
}

int FindPlayerID(int uid)
{
	for(int i = 0;i < MAX_USERS_CLI; ++i)
	{
		if( (players[i].iUid == uid) && (players[i].bOnline) )
		{
			return i;
		}
	}
	return -1;
}

int FindXYPlayerID(int x , int y)
{
	for(int i = 0;i < MAX_USERS_CLI; ++i)
	{
		if( players[i].bOnline )
		{
			if( abs(x-players[i].fPos[0]) < 5 && abs(y-players[i].fPos[1]) < 5)
				return players[i].iUid;
		}
	}
	return -1;
}

bool AddPlayer ( MXUSInfoUpdateToUS *m )
{
	for(int i = 0;i < MAX_USERS_CLI; ++i)
	{
		if( !players[i].bOnline )
		{
			players[i].iUid = m->iUid;
			players[i].bOnline = true;
			players[i].fPos[0] = m->fPos[0];
			players[i].fPos[1] = m->fPos[1];
			players[i].iHP = m->iHP;
			players[i].iMP = m->iMP;
			return true;
		}
	}
	return false;
}
int tt = 2;
void UpdatePlayer()
{
	fTime = GetTickCount() / 1000.0f;

	if((fTime - fLastUpdate) > (UPDATE_FREQUENCY/1000.0f))
	{
		//printf("player move!!\n");
		if( player.fCurrentX > 650 )
			tt = 1;
		else if( player.fCurrentX < 300 )
			tt = 2;
		
		switch(tt)
		{
		case 1:
			--player.fCurrentX;
			break;
		case 2:
			++player.fCurrentX;
		}
		
		UserUpdateToMX uutm;
		uutm.iUid = player.iUid;
		uutm.fPos[0] = player.fCurrentX;
		uutm.fPos[1] = player.fCurrentY;

		SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );

		fLastUpdate = GetTickCount() / 1000.0f;
	}
/**************************AI*******************************
	float v;
	float fX_R,fY_R,vecx,vecy;

	// iState = 0 think, 1 move, 2 sleep
	if(player.iState == 0)
	{
		srand( GetTickCount() );
		v = (float)(((V_RANGE_MAX - V_RANGE_MIN)*rand()/RAND_MAX) + V_RANGE_MIN);

		if((fX_R = (float)(rand()%RANDOM_SCOPE - (RANDOM_SCOPE/2))) == 0)
			fX_R = 1;
		if((fY_R = (float)(rand()%RANDOM_SCOPE - (RANDOM_SCOPE/2))) == 0)
			fY_R = 1;
		//printf("x = %f, y = %f \n",x,y);

		vecx = player.fCurrentX + fX_R;
		vecy = player.fCurrentY + fY_R;

		//printf("vecx = %f, vecy = %f \n",vecx,vecy);

		if(vecx > TOTAL_MAP_X)
			player.fTargetX = TOTAL_MAP_X;
		else if(vecx < 0)
			player.fTargetX = 0;
		else
			player.fTargetX = vecx;

		if(vecy > TOTAL_MAP_Y)
			player.fTargetY = TOTAL_MAP_Y;
		else if(vecy < 0)
			player.fTargetY = 0;
		else
			player.fTargetY = vecy;

		//printf("Justed : fTargetX = %f, fTargetY = %f \n",player.fTargetX,player.fTargetY);
		player.fVel[0] = (fX_R/abs(fX_R)) * v;
		player.fVel[1] = (fY_R/abs(fY_R)) * v;
		//printf("VelX = %f, VelY = %f \n",player.fVel[0],player.fVel[1]);
		player.iState = 1;
	}else if(player.iState == 1) //Player is moving
	{
		fTime = GetTickCount() / 1000.0f;

		if((fTime - fLastUpdate) > (UPDATE_FREQUENCY/1000.0f))
		{
			player.fPos[0] = player.fCurrentX;
			player.fPos[1] = player.fCurrentY;		

			if(abs(player.fPos[0] - player.fTargetX) <= abs(player.fVel[0]))
			{
				player.fCurrentX = player.fTargetX;
			}else
			{
				player.fCurrentX += player.fVel[0];
			}

			if(abs(player.fPos[1] - player.fTargetY) <= abs(player.fVel[1]))
			{
				player.fCurrentY = player.fTargetY;
			}else
			{
				player.fCurrentY += player.fVel[1];
			}

			// Send update messages to map server            
			UserUpdateToMX uutm;
			uutm.iUid = player.iUid;
			//strcpy ( uutm.sUserName , player.sUserName );
			uutm.fPos[0] = player.fCurrentX;
			uutm.fPos[1] = player.fCurrentY;

			//send( sMXSocket, (char*)&uutm, sizeof(uutm), 0 );

			SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );
			//printf("user uid = %d ,x=%f,y=%f send update\n",player.iUid,player.fCurrentX,player.fCurrentY);
			//fflush(fp);
			//printf("Target user place x=%f,y=%f\n",player.fTargetX,player.fTargetY);
			//system("pause");

			fLastUpdate = GetTickCount() / 1000.0f;

			if((player.fCurrentX == player.fTargetX) && (player.fCurrentY == player.fTargetY))
			{
				player.iState = 2;
			}
		}

	}else
	{
		//sleep_time = rand()%MAX_SLEEP;

		//Sleep(sleep_time);
		//Reserved section, for iState = 2(wait)
		player.iState = 0;
	}
******************************************************************************/
}

bool ConnectToMX( void )
{
	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof(addr) );

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(mx_port));
	addr.sin_addr.S_un.S_addr = inet_addr(mx_ip);

	connect( sMXSocket, (struct  sockaddr *)&addr, sizeof(struct sockaddr) );

	UserLoginToMX ulm;
	ulm.iUid = player.iUid;
	ulm.iZid = player.iZid;

	SendTCPPacket( sMXSocket , (char *)&ulm , sizeof(ulm) );

	char buf[MAX_PACKET_SIZE];
	ZeroMemory( &buf, sizeof(buf) );

	while(true)
	{
		RecvTCPPacket( sMXSocket , (char *)&buf );
		
		MessageHeader *Mh = (MessageHeader *)&buf;

		if( Mh->MsgID == MSG_MXUSINFOUPDATETOUS )
		{
			MXUSInfoUpdateToUS *miuu = (MXUSInfoUpdateToUS *)&buf;
			if ( miuu->iUid != player.iUid )
			{
				continue;
			}

			player.fCurrentX = miuu->fPos[0];
			player.fCurrentY = miuu->fPos[1];
			player.iHP = miuu->iHP;
			player.iMP = miuu->iMP;
			break;
		}
	}
	//printf("uid = %d first pos : x = %f, y = %f\n" ,player.iUid,player.fCurrentX,player.fCurrentY );

	return true;
}

bool ProcessPacket( const char * pBuffer )
{
	MessageHeader * pMh = (MessageHeader*)pBuffer;

	switch( pMh->MsgID )
	{
	case MSG_MXUSINFOUPDATETOUS:
		{
			MXUSInfoUpdateToUS * miuu = (MXUSInfoUpdateToUS *)pBuffer;

			if ( miuu->iUid == player.iUid )
			{
				player.iHP = miuu->iHP;
				player.iMP = miuu->iMP;
				break;
			}

			int i = FindPlayerID( miuu->iUid );
			if( i < 0 )
			{
				if ( AddPlayer( miuu ) )
				{
					break;
				}
				else
					printf("No space for player in the mx!!!\n");
			}else
			{
				//printf("recv otherplayer uid = %d,x = %f,y = %f\n",miuu->iUid,miuu->fPos[0],miuu->fPos[1]);
				players[i].fPos[0] = miuu->fPos[0];
				players[i].fPos[1] = miuu->fPos[1];
				players[i].iHP = miuu->iHP;
				players[i].iMP = miuu->iMP;
				break;
			}
		}break;

	case MSG_ZSUSCOMTOUS:
		{
			//printf("recv user com\n");
			ZSUSComToUS * zucu = (ZSUSComToUS *)pBuffer;

			if( zucu->iUid_D == player.iUid )
			{
				switch( zucu->cCom )
				{
				case C_ATTACK:
					{
						if(player.iHP <= 0)
						{
							printf("I'm already dead....\n");
							break;
						}

						player.iHP -= zucu->iNum;
						printf("be attck 1\n");
						if(player.iHP < 0)
							player.iHP = 0;
			
					}break;
				
				case C_RECOVER:
					{
						printf("I'm recovering!!\n");
						player.iHP = zucu->iNum;
					}break;
				}
			}else
			{
				int j = FindPlayerID( zucu->iUid_D );
				switch( zucu->cCom )
				{
				case C_ATTACK:
					{
						if(players[j].iHP <= 0)
						{
							printf("user %d already dead....\n",players[j].iUid);
							break;
						}

						players[j].iHP -= zucu->iNum;

						if(players[j].iHP < 0)
							players[j].iHP = 0;
			
					}break;
				
				case C_RECOVER:
					{
						printf("user %d is recovering!!\n",players[j].iUid);
						players[j].iHP = zucu->iNum;
					}break;
				}
			}
		}break;

	case MSG_USERLOGOFF:
		{
			UserLogoff * ul = (UserLogoff *)pBuffer;
			//printf("recv mx to us uid %d logoff\n",ul->iUid);
			int i = FindPlayerID( ul->iUid );
			if( i >= 0 )
			{
				players[i].bOnline = false;
			}
		}break;
	}

	return true;
}

void renderBitmapString( float x, float y, char *string)
{  
	char *c;
	glRasterPos2f(x,y);
	for (c=string; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
	}
}

void displayCB(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glPointSize(5.0f);
	glColor3f(1.0f, 1.0f, 1.0f);	

	for(int i = 0;i < MAX_USERS_CLI;i++)
	{
		if(players[i].bOnline)
		{
			float rx = players[i].fPos[0] - player.fCurrentX;
			float ry = players[i].fPos[1] - player.fCurrentY;
			if((abs(rx) > USER_VISION_MAX/2) || (abs(ry) > USER_VISION_MAX/2))
				players[i].bOnline = false;
			else
			{
				char hpmp[30];
				sprintf( hpmp, "%d:H%d/%d:M%d/%d", players[i].iUid , players[i].iHP,MAX_HP , players[i].iMP,MAX_MP );
				glPushMatrix();
				renderBitmapString( rx + USER_VISION_MAX/2-30, ry + USER_VISION_MAX/2-8,hpmp );
				if( players[i].iUid == iSelected )
				{
					glColor3f(0.0f, 1.0f, 0.0f);
				}
				glBegin(GL_POINTS);
				glVertex2f(rx + USER_VISION_MAX/2,ry + USER_VISION_MAX/2);
				glEnd();
				glColor3f(1.0f, 1.0f, 1.0f);
				glPopMatrix();
			}
		}
	}
	glPointSize(10.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	glPushMatrix();
	char hpmp[30];
	sprintf( hpmp, "%d:H%d/%d:M%d/%d", player.iUid , player.iHP,MAX_HP , player.iMP,MAX_MP );
	renderBitmapString( USER_VISION_MAX/2-30,USER_VISION_MAX/2-8,hpmp );
	glBegin(GL_POINTS);
	glVertex2f(USER_VISION_MAX/2,USER_VISION_MAX/2);
	glEnd();
	glPopMatrix();
	glutSwapBuffers();

	if( con != 'n')
	{
		/*
		fTime = GetTickCount() / 1000.0f;
		if((fTime - fLastUpdate) > (UPDATE_FREQUENCY/1000.0f))
		{
			UserUpdateToMX uutm;
			uutm.iUid = player.iUid;
			uutm.fPos[0] = player.fCurrentX;
			uutm.fPos[1] = player.fCurrentY;

			SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );

			fLastUpdate = GetTickCount() / 1000.0f;
		}
		*/
	}

}

void keydown(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'q':{
		UserLogoff ulf;
		ulf.iUid = player.iUid;
		SendTCPPacket( sMXSocket, (char*)&ulf, sizeof(ulf) );
		exit(0);}
	case 'h':{
		if( iSelected == -1 )
		{
			printf("no attack target!!\n");
		}else
		{
			UserComToMX ucm;
			ucm.iUid_S = player.iUid;
			ucm.iUid_D = iSelected;
			ucm.cCom = C_ATTACK;
			ucm.iTime = GetTickCount();
			printf("attack uid=%d!!\n",iSelected);
			SendTCPPacket( sMXSocket, (char*)&ucm, sizeof(ucm) );
		}
			 }break;
	case 'j':{
		if( iSelected == -1 )
		{
			printf("no poison target!!\n");
		}else
		{
			UserComToMX ucm;
			ucm.iUid_S = player.iUid;
			ucm.iUid_D = iSelected;
			ucm.cCom = C_POISON;
			ucm.iTime = GetTickCount();
			printf("poison uid=%d!!\n",iSelected);
			SendTCPPacket( sMXSocket, (char*)&ucm, sizeof(ucm) );
		}
			 }break;
	case 'k':{
		UserComToMX ucm;
		ucm.iUid_S = player.iUid;
		ucm.iUid_D = player.iUid;
		ucm.cCom = C_RECOVER;
		ucm.iTime = GetTickCount();
		printf("recover!!\n");

		SendTCPPacket( sMXSocket, (char*)&ucm, sizeof(ucm) );
			 }break;
	case 'w':{
		--player.fCurrentY;
		if(player.fCurrentY < 0 )
		{
			player.fCurrentY = 0;
		}
		UserUpdateToMX uutm;
		uutm.iUid = player.iUid;
		uutm.fPos[0] = player.fCurrentX;
		uutm.fPos[1] = player.fCurrentY;

		SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );
		fLastUpdate = GetTickCount() / 1000.0f;
			 }break;
	case 'a':{
		--player.fCurrentX;
		if(player.fCurrentX < 0 )
		{
			player.fCurrentX = 0;
		}
		UserUpdateToMX uutm;
		uutm.iUid = player.iUid;
		uutm.fPos[0] = player.fCurrentX;
		uutm.fPos[1] = player.fCurrentY;

		SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );
		fLastUpdate = GetTickCount() / 1000.0f;
			 }break;
	case 's':{
		++player.fCurrentY;
		if(player.fCurrentY++ > TOTAL_MAP_Y )
		{
			player.fCurrentY = TOTAL_MAP_Y;
		}
		UserUpdateToMX uutm;
		uutm.iUid = player.iUid;
		uutm.fPos[0] = player.fCurrentX;
		uutm.fPos[1] = player.fCurrentY;

		SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );
		fLastUpdate = GetTickCount() / 1000.0f;
			 }break;
	case 'd':{
		++player.fCurrentX;
		if(player.fCurrentX++ > TOTAL_MAP_X )
		{
			player.fCurrentX = TOTAL_MAP_X;
		}
		UserUpdateToMX uutm;
		uutm.iUid = player.iUid;
		uutm.fPos[0] = player.fCurrentX;
		uutm.fPos[1] = player.fCurrentY;

		SendTCPPacket( sMXSocket, (char*)&uutm, sizeof(uutm) );
		fLastUpdate = GetTickCount() / 1000.0f;
			 }break;
	}
}

void processMouse(int button, int state, int x, int y)
{
	if ( state == GLUT_DOWN )
	{
		if ( button == GLUT_LEFT_BUTTON )
		{
			int curx = x-USER_VISION_MAX/2+player.fCurrentX;
			int cury = y-USER_VISION_MAX/2+player.fCurrentY;
			int is = FindXYPlayerID( curx,cury );
			if( is == iSelected )
				iSelected = -1;
			else
				iSelected = is;
			printf("select uid %d\n",iSelected);
		}
	}
}

void processMouseActiveMotion(int x, int y)
{
}

void processMousePassiveMotion(int x, int y)
{
}

DWORD WINAPI glut_Thread( LPVOID pParam )
{	
	int win;
	char clititle[10];
	sprintf( clititle , "cli_name:%s" , pParam );

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(USER_VISION_MAX,USER_VISION_MAX);
	win = glutCreateWindow(clititle);

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, USER_VISION_MAX, USER_VISION_MAX, 0 );

	glClearColor(0.0,0.0,0.0,0.0);

	glutDisplayFunc(displayCB);
	glutIdleFunc(displayCB);
	glutKeyboardFunc(keydown);

	//adding here the mouse processing callbacks
	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);
	glutPassiveMotionFunc(processMousePassiveMotion);
	//glutEntryFunc(processMouseEntry);
	
	glutMainLoop();


	return S_OK;
}

int main(int argc,char *argv[])
{

	//FILE *fp;
	//char file[15];
	//strcpy(file,argv[1]);
	//strcat(file,".txt");

	//fp=fopen(file,"wt");

	//*stdout = *fp;

	//memset( &player,'-1' ,sizeof(player) );
	player.iUid = -1;

	for(int i = 0 ; i < MAX_USERS_CLI ; ++i )
	{
		players[i].iUid = -1;
		players[i].bOnline = false;
	}

	//----------------------for RWP--------------------------
	srand( (unsigned)time( NULL ) );
	player.iState = 0;
	//-------------------------------------------------------

	Initial();
	strcpy ( player.sUserName,argv[1]);
	int show = ( atoi(argv[3]) );

	WSADATA wsaData;
	if( SOCKET_ERROR == WSAStartup( WINSOCK_VERSION, &wsaData ) || wsaData.wVersion != WINSOCK_VERSION )
		return false;

	//------------------for user login to gs request zone ip----------------
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	ZeroMemory( &addr, sizeof(addr) );

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(gs_port));
	addr.sin_addr.S_un.S_addr   = inet_addr(gs_ip);

	connect( sockfd, (struct sockaddr *)&addr , sizeof(struct sockaddr));

	UserLogInToGS ultg;
	strcpy ( ultg.sUserName,player.sUserName);
	strcpy ( ultg.sPass,argv[2]);

	SendTCPPacket( sockfd, (char *)&ultg , sizeof(ultg) );

	char buf[MAX_PACKET_SIZE];
	ZeroMemory( &buf, sizeof(buf) );

	RecvTCPPacket( sockfd, (char *)&buf );

	int try_num = 0;
	while(try_num < 3)
	{
		MessageHeader * tMh = (MessageHeader*)buf;

		if(tMh->MsgID == MSG_USERREJECT)
		{
			UserReject *urm = (UserReject *)buf;
			switch(urm->rWhy)
			{
			case 	R_NOTEXIST:
				{
					printf("Wrong username!!!\n");
				}break;
			case	R_FULL:
				{
					printf("zone server FULL!!!\n");
				}break;
			case	R_WRONGPASS:
				{
					printf("Pass wrong!!!\n");
				}break;
			}

			printf("Retry ? (username pass)\n");
			scanf("%s %s\n",&ultg.sUserName,&ultg.sPass );

			SendTCPPacket( sockfd, (char *)&ultg , sizeof(ultg) );
		}else
		{
			UserZoneInfo *uzi = (UserZoneInfo *)buf;
			player.iUid = uzi->iUid;
			player.iZid = uzi->iZid;
			strcpy ( player.sIP, uzi->sZip );
			strcpy ( player.sPort , uzi->sZport );		

			printf("uid = %d get zone info ip %s,port %s\n",player.iUid,player.sIP,player.sPort);
			break;
		}

		ZeroMemory( &buf, sizeof(buf) );
		++try_num;
		RecvTCPPacket(sockfd, (char *)&buf);
	}
	closesocket(sockfd);

	//---------------get zone info and go to login mx and zone---------------
	printf("Manual control ? (y/n)\n (W,S,A,D direction,Q exit)\n");
	scanf("%c",&con);

	if(show)
	{
		if( NULL == CreateThread( NULL, 0, glut_Thread, player.sUserName, 0, NULL ))
			return -1;
	}

	if( SOCKET_ERROR == (sMXSocket = socket( AF_INET, SOCK_STREAM, 0 )))
	{
		printf("Socket Error !!!!\n");
		return false;
	}

	if( ConnectToMX( ) )
	{	
		fLastUpdate = 0.0f;

		int nready;
		struct timeval timeout;

		FD_ZERO(&rset);
		FD_ZERO(&temprset);
		FD_SET( sMXSocket, &rset);
		int maxfdp1 = sMXSocket + 1;

		//int time1 = GetTickCount();

		//int time2;

		//int attack_t = 50;

		while(true)
		{
			/*
			time2 = GetTickCount();

			if( ((time2 - time1)>1) && player.iUid == 1 && attack_t != 0 )
			{
				UserComToMX ucm;
				ucm.iUid_S = player.iUid;
				ucm.iUid_D = 10;
				ucm.cCom = C_ATTACK;
				ucm.iTime = time2;
				printf("attack uid=10!!\n");
				SendTCPPacket( sMXSocket, (char*)&ucm, sizeof(ucm) );
				--attack_t;
				time1 = time2;
			}
			*/
			if(con == 'n')
				UpdatePlayer();
			FD_ZERO(&temprset);
			temprset = rset;
			timeout.tv_sec = 0;
			timeout.tv_usec = 5000;

			if ( (nready = select(maxfdp1, &temprset, NULL, NULL, &timeout)) < 0)
			{
				break;

			}else if( nready == 0)
			{
				if(con == 'n')
					UpdatePlayer();
				continue;
			}

			if (FD_ISSET(sMXSocket, &temprset))
			{
				char buffer[MAX_PACKET_SIZE];
				ZeroMemory( buffer, sizeof(buffer) );
				RecvTCPPacket( sMXSocket , (char *)&buffer );

				ProcessPacket( (char *)&buffer );
			}
		}

	}

	closesocket( sMXSocket );
	WSACleanup();
	//fclose( fp );

	return 0;
}