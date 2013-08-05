#include "user.h"

extern User Users[MAX_USERS_ZONE];

extern int iZoneUser;

User::User()
{
}

User::~User()
{
}

bool User::Create( int iTid, bool ghost )
{
	++iZoneUser;
	bGhost = ghost;
	iUid = iTid;
	printf("new user %d create!!!!\n",iUid);
	bConnected = true;
	bDirtyBit = false;
	bMig = false;
	bPoi = false;
	bPlaceChange = false;
//	iSeqNum = 0;
	fPoiTime = -1;
	iPlace = -1;
	iLastPlace = -1;
	return true;
}

void User::Destroy()
{
	//ghost = false;
	--iZoneUser;
	printf("uid %d destroy!!!\n",iUid);
	iUid = -1;
	iHP = -1;
	iMP = -1;
	bPoi = false;
	bConnected = false;
	bMig = false;
	bPlaceChange = false;
//	iSeqNum = 0;
	fPoiTime = -1;
	iPlace = -1;
	iLastPlace = -1;
}

void User::SetDirtyBit()
{
	bDirtyBit = true;
}


void User::ResetDirtyBit()
{
	bDirtyBit = false;
}

bool User::GetDirtyBit()
{
	return bDirtyBit;
}

int FindUserID(int uid)
{
	for(int i = 0 ; i < MAX_USERS_ZONE ;++i)
	{
		if( Users[i].iUid == uid )
			return i;
	}
	return -1;
}