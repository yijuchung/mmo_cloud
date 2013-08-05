#include "combuf.h"

struct TempUser tu[MAX_SYNC_OBJ];

void PrintTempUser()
{
	for(int i = 0;i<MAX_SYNC_OBJ;++i)
	{
		printf("tu %d use %d uid %d packet %d\n",i,tu[i].used,tu[i].uid,tu[i].pac.size());
	}
}

void AddTempUser( int uid )
{
	for(int i = 0;i<MAX_SYNC_OBJ;++i)
	{
		if( !tu[i].used )
		{
			tu[i].uid = uid;
			tu[i].used = true;
			break;
		}
	}
}

void AddPacket( int uid, char *pac, int time )
{
	for(int i = 0;i<MAX_SYNC_OBJ;++i)
	{
		if( tu[i].used )
		{
			if( tu[i].uid == uid )
			{
				pacbuf pb;
				pb.time = time;
				memcpy( (char *)&(pb.zucu), pac, sizeof(ZSUSComToUS) );

				tu[i].pac.push_back( pb );
				break;
			}
		}
	}
}

void DelTempUser( int uid )
{
	for(int i = 0;i<MAX_SYNC_OBJ;++i)
	{
		if( tu[i].used )
		{
			if( tu[i].uid == uid )
			{
				tu[i].pac.clear();
				tu[i].used = false;
				break;
			}
		}
	}
}