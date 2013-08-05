#ifndef _USER_
#define _USER_
#include <winsock2.h>
#include <autoserial/binaryhandlers.h>
#include "net.h"
#include "def.h"

class User : public autoserial::ISerializable
{
	AS_CLASSDEF(User)
	AS_MEMBERS
		AS_ITEM(float,fX)
		AS_ITEM(float,fY)
		AS_ITEM(int,iHP)
		AS_ITEM(int,iMP)
		AS_ITEM(bool,bConnected)
		AS_ARRAY(char,sUserName,MAX_NAME_LEN)
		AS_ITEM(int,iUid)
		AS_ITEM(bool,bMig)
		AS_ITEM(bool,bPlaceChange)
		AS_ITEM(int,iPlace)
		AS_ITEM(int,iLastPlace)
		AS_ITEM(float,fPoiTime)
		AS_ITEM(bool,bPoi)
		AS_ITEM(int,iTime)
	AS_CLASSEND;

	public:

	User();
	~User();
	
	bool			Create( int iTid,bool ghost );
	void			Destroy();

	void			SetDirtyBit();
	void			ResetDirtyBit();
	bool			GetDirtyBit();
	bool			bGhost;
	bool			bDirtyBit;
	int				iMigFeq;
};

int FindUserID(int uid);

#endif