#include "zstable.h"

extern struct AdjZone zstable[9];

bool IniZsTable( int zid, int zid_s )
{
	FILE *fp;

	char file[8];
	if( zid_s != -1 )
		sprintf(file, "%d.txt" , zid_s );
	else
		sprintf(file, "%d.txt" , zid );

	fp = fopen(file,"r");

	int i = 0;
	while( fscanf(fp,"%d",&zstable[i].zid) != EOF )
	{
		//printf("zs[%d] zid %d\n",i,zstable[i].zid);
		zstable[i].connect = false;
		++i;
		if(i >= 9)
			break;
	}

	fclose(fp);

	return true;
}

int FindZsTable( int zid )
{
	if( zid >= MAX_ZONE )
		zid = zstable[4].zid;
	for( int i = 0 ; i<9 ; ++i )
	{
		if( zstable[i].zid == zid )
			return i;
	}

	return -1;
}

int AddZsTable( int zid, int sock )
{
	if( zid >= MAX_ZONE )
		zid = zstable[4].zid;
	for( int i = 0 ; i<9 ; ++i )
	{
		if( zstable[i].zid == zid )
		{
			zstable[i].connect = true;
			zstable[i].sock = sock;
			return i;
		}
	}

	return -1;
}