#ifndef __ZSTABLE__
#define __ZSTABLE__
#include <stdio.h>
#include "net.h"

#define MAP_POS_CASE1_X		(float)(iZoneX-iZoneW)
#define MAP_POS_CASE1_Y		(float)(iZoneY-iZoneH)
#define MAP_POS_CASE2_X		(float)(iZoneX)
#define MAP_POS_CASE2_Y		(float)(iZoneY-iZoneH)
#define MAP_POS_CASE3_X		(float)(iZoneX+iZoneW)
#define MAP_POS_CASE3_Y		(float)(iZoneY-iZoneH)
#define MAP_POS_CASE4_X		(float)(iZoneX-iZoneW)
#define MAP_POS_CASE4_Y		(float)(iZoneY)
#define MAP_POS_CASE6_X		(float)(iZoneX+iZoneW)
#define MAP_POS_CASE6_Y		(float)(iZoneY)
#define MAP_POS_CASE7_X		(float)(iZoneX-iZoneW)
#define MAP_POS_CASE7_Y		(float)(iZoneY+iZoneH)
#define MAP_POS_CASE8_X		(float)(iZoneX)
#define MAP_POS_CASE8_Y		(float)(iZoneY+iZoneH)
#define MAP_POS_CASE9_X		(float)(iZoneX+iZoneW)
#define MAP_POS_CASE9_Y		(float)(iZoneY+iZoneH)

struct AdjZone
{
	bool	connect;
	int		zid;
	int		sock;
};

bool IniZsTable( int zid, int zid_s );

int FindZsTable( int zid );

// return i for zs array
int AddZsTable( int zid, int sock );
#endif