#ifndef _MSG_
#define _MSG_
#include "def.h"

enum eMessage
{
	MSG_USERLOGINTOGS,
	MSG_USERREJECT,
	MSG_USERZONEINFO,
	MSG_USERLOGINTOMX,
	MSG_USERLOGINTOZS,
	MSG_USERUPDATETOMX,
	MSG_USERCOMTOMX,
	MSG_USERUPDATETOZS,
	MSG_USERCOMTOZS,
	MSG_ZSOPENTOGS,
	MSG_ZSUPDATETOGS,
	MSG_ZSUPDATETOMX,
	MSG_ZSDUPTOMX,
	MSG_ZSUSCOMTOUS,
	MSG_ZSUSINFOUPDATETOMX,
	MSG_MXUSINFOUPDATETOUS,
	MSG_ZSUSMIGTOGS,
	MSG_GSUSMIGTOZS,
	MSG_ZSUSDATATOZS,
	MSG_ZSMIGUPDATETOMX,
	MSG_ZSMIGOFFLINETOMX,
	MSG_USERLOGOFF,
	MSG_ZSBROADUSERLOGOFF,
	MSG_ZSUSMIGTOZS,
	MSG_ACKZSUSMIGTOZS,
	MSG_ZSUSOBJTOZS,
	MSG_ACKZSUSOBJTOZS,
	MSG_ZSUSZOOGS,
	MSG_ZSROUTMX,
	MSG_ZSPROMIGGS,
	MSG_ACKZSPROMIGGS,
	MSG_ZSPRODONEGS,
	MSG_ZSPRODONEMX,
};

enum Objstate
{
	M_OBJ,
	M_PATCH,
};

enum Migstate
{
	M_MIGREQ,
	M_MIGFIN,
	M_PROREQ,
	M_PROFIN,
};

enum Reason
{
	R_NOTEXIST,
	R_ONLINE,
	R_FULL,
	R_WRONGPASS,
	R_SUCCESS,
};

enum UserCom
{
	C_ATTACK,
	C_POISON,
	C_RECOVER,
};

struct MessageHeader
{
	eMessage MsgID;
	int		 iSize;
};

struct UserLogInToGS
{
	MessageHeader   Header;
	char			sUserName[MAX_NAME_LEN];
	char			sPass[MAX_NAME_LEN];

	UserLogInToGS() { 
		Header.MsgID = MSG_USERLOGINTOGS;
		Header.iSize = sizeof( UserLogInToGS ) - sizeof(Header);
	}
};

struct UserReject
{
	MessageHeader   Header;
	Reason			rWhy;

	UserReject() { 
		Header.MsgID = MSG_USERREJECT;
		Header.iSize = sizeof( UserReject ) - sizeof(Header);
	}
};

struct UserZoneInfo
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;
	//char			sZoneName[MAX_ZONE_NAME];
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];

	UserZoneInfo() { 
		Header.MsgID = MSG_USERZONEINFO;
		Header.iSize = sizeof( UserZoneInfo ) - sizeof(Header);
	}
};

struct UserLoginToMX
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;
	//char			sUserName[MAX_NAME_LEN];

	UserLoginToMX() { 
		Header.MsgID = MSG_USERLOGINTOMX;
		Header.iSize = sizeof( UserLoginToMX ) - sizeof(Header);
	}
};

struct UserLoginToZS
{
	MessageHeader   Header;
	int				iUid;
	//char			sUserName[MAX_NAME_LEN];

	UserLoginToZS() { 
		Header.MsgID = MSG_USERLOGINTOZS;
		Header.iSize = sizeof( UserLoginToZS ) - sizeof(Header);
	}
};

struct UserUpdateToMX
{
	MessageHeader   Header;
	int				iUid;
	//char			sUserName[MAX_NAME_LEN];
	float			fPos[2];

	UserUpdateToMX() { 
		Header.MsgID = MSG_USERUPDATETOMX;
		Header.iSize = sizeof( UserUpdateToMX ) - sizeof(Header);
	}
};

struct UserComToMX
{
	MessageHeader   Header;
	int				iUid_S;
	int				iUid_D;
	int				iTime;
	UserCom			cCom;
	//char			sUserName[MAX_NAME_LEN];
	//float			fPos[2];

	UserComToMX() { 
		Header.MsgID = MSG_USERCOMTOMX;
		Header.iSize = sizeof( UserComToMX ) - sizeof(Header);
	}
};

struct UserUpdateToZS
{
	MessageHeader   Header;
	int				iUid;
	//char			sUserName[MAX_NAME_LEN];
	float			fPos[2];

	UserUpdateToZS() { 
		Header.MsgID = MSG_USERUPDATETOZS;
		Header.iSize = sizeof( UserUpdateToZS ) - sizeof(Header);
	}
};

struct UserComToZS
{
	MessageHeader   Header;
	//int			iSeqNum;
	//unsigned char	cRanNum;
	int				iUid_S;
	int				iUid_D;
	int				iTime;
	UserCom			cCom;
	//char			sUserName[MAX_NAME_LEN];
	//float			fPos[2];

	UserComToZS() { 
		Header.MsgID = MSG_USERCOMTOZS;
		Header.iSize = sizeof( UserComToZS ) - sizeof(Header);
	}
};

struct ZSOpenToGS
{
	MessageHeader   Header;
	int				iZid;
	//char			sZoneName[MAX_NAME_LEN];

	ZSOpenToGS() { 
		Header.MsgID = MSG_ZSOPENTOGS;
		Header.iSize = sizeof( ZSOpenToGS ) - sizeof(Header);
	}
};

struct ZSUpdateToGS
{
	MessageHeader   Header;
	int				iZid;
	int				iUser;
	//char			sZoneName[MAX_NAME_LEN];

	ZSUpdateToGS() { 
		Header.MsgID = MSG_ZSUPDATETOGS;
		Header.iSize = sizeof( ZSUpdateToGS ) - sizeof(Header);
	}
};

struct ZSUpdateToMX
{
	MessageHeader   Header;
	int				iZid;
	//char			sZoneName[MAX_ZONE_NAME];
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];

	ZSUpdateToMX() { 
		Header.MsgID = MSG_ZSUPDATETOMX;
		Header.iSize = sizeof( ZSUpdateToMX ) - sizeof(Header);
	}
};

struct ZSDupToMX
{
	MessageHeader   Header;
	int				iZid_D;
	int				iZid_S;
	//char			sZoneName[MAX_ZONE_NAME];
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];

	ZSDupToMX() { 
		Header.MsgID = MSG_ZSDUPTOMX;
		Header.iSize = sizeof( ZSDupToMX ) - sizeof(Header);
	}
};

struct ZSUSComToUS
{
	MessageHeader   Header;
	int				iZid_S;
	int				iUid_S;
	int				iUid_D;
	int				iNum;
	UserCom			cCom;

	ZSUSComToUS() { 
		Header.MsgID = MSG_ZSUSCOMTOUS;
		Header.iSize = sizeof( ZSUSComToUS ) - sizeof(Header);
	}
};

struct ZSUSInfoUpdateToMX
{
	MessageHeader   Header;
	int				iUid_D;
	int				iUid_S;
	bool			bGhost;
	//int				iZid_D;
	int				iHP_S;
	int				iMP_S;
	//char			sUserName_D[MAX_NAME_LEN];
	//char			sUserName_S[MAX_NAME_LEN];
	float			fPos[2];

	ZSUSInfoUpdateToMX() { 
		Header.MsgID = MSG_ZSUSINFOUPDATETOMX;
		Header.iSize = sizeof( ZSUSInfoUpdateToMX ) - sizeof(Header);
	}
};

struct MXUSInfoUpdateToUS
{
	MessageHeader   Header;
	int				iUid;
	//int				iZid;
	int				iHP;
	int				iMP;
	//char			sUserName[MAX_NAME_LEN];
	float			fPos[2];

	MXUSInfoUpdateToUS() { 
		Header.MsgID = MSG_MXUSINFOUPDATETOUS;
		Header.iSize = sizeof( MXUSInfoUpdateToUS ) - sizeof(Header);
	}
};

struct ZSUSMigToGS
{
	MessageHeader   Header;
	int				iUid;
	//char			sUserName[MAX_NAME_LEN];
	int				iZid;

	ZSUSMigToGS() { 
		Header.MsgID = MSG_ZSUSMIGTOGS;
		Header.iSize = sizeof( ZSUSMigToGS ) - sizeof(Header);
	}
};

struct GSUSMigToZS
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;
	//char			sZoneName[MAX_ZONE_NAME];
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];
	//char			sUserName[MAX_NAME_LEN];

	GSUSMigToZS() { 
		Header.MsgID = MSG_GSUSMIGTOZS;
		Header.iSize = sizeof( GSUSMigToZS ) - sizeof(Header);
	}
};

struct ZSUSDataToZS
{
	MessageHeader   Header;
	int				iUid;
	int				iZid_S;

	ZSUSDataToZS() { 
		Header.MsgID = MSG_ZSUSDATATOZS;
		Header.iSize = sizeof( ZSUSDataToZS ) - sizeof(Header);
	}
};

struct ZSMigUpdateToMX
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];
	//char			sZoneName[MAX_ZONE_NAME];
	//char			sUserName[MAX_NAME_LEN];

	ZSMigUpdateToMX() { 
		Header.MsgID = MSG_ZSMIGUPDATETOMX;
		Header.iSize = sizeof( ZSMigUpdateToMX ) - sizeof(Header);
	}
};

struct ZSMigOfflineToMX
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;

	ZSMigOfflineToMX() { 
		Header.MsgID = MSG_ZSMIGOFFLINETOMX;
		Header.iSize = sizeof( ZSMigOfflineToMX ) - sizeof(Header);
	}
};

struct UserLogoff
{
	MessageHeader   Header;
	//bool			bMig;
	int				iUid;
	//char			sUserName[MAX_NAME_LEN];

	UserLogoff() { 
		Header.MsgID = MSG_USERLOGOFF;
		Header.iSize = sizeof( UserLogoff ) - sizeof(Header);
	}
};

struct ZSBroadUserLogoff
{
	MessageHeader   Header;
	int				iUid_D;
	int				iUid_S;
	//char			sUserName_D[MAX_NAME_LEN];
	//char			sUserName_S[MAX_NAME_LEN];

	ZSBroadUserLogoff() { 
		Header.MsgID = MSG_ZSBROADUSERLOGOFF;
		Header.iSize = sizeof( ZSBroadUserLogoff ) - sizeof(Header);
	}
};

struct ZSUSMigToZS
{
	MessageHeader   Header;
	Migstate		MigHeader;
	int				iUid;
	int				iZid;

	ZSUSMigToZS() { 
		Header.MsgID = MSG_ZSUSMIGTOZS;
		Header.iSize = sizeof( ZSUSMigToZS ) - sizeof(Header);
	}
};

struct ACKZSUSMigToZS
{
	MessageHeader   Header;
	Migstate		MigHeader;
	int				iUid;
	int				iZid;

	ACKZSUSMigToZS() { 
		Header.MsgID = MSG_ACKZSUSMIGTOZS;
		Header.iSize = sizeof( ACKZSUSMigToZS ) - sizeof(Header);
	}
};

struct ZSUSObjToZS
{
	MessageHeader   Header;
	Objstate		ObjHeader;
	int				iUid;
	int				iTime;
	int				iSize;

	ZSUSObjToZS() { 
		Header.MsgID = MSG_ZSUSOBJTOZS;
		Header.iSize = sizeof( ZSUSObjToZS ) - sizeof(Header);
	}
};

struct ACKZSUSObjToZS
{
	MessageHeader   Header;
	Objstate		ObjHeader;
	int				iUid;
	int				iTime;

	ACKZSUSObjToZS() { 
		Header.MsgID = MSG_ACKZSUSOBJTOZS;
		Header.iSize = sizeof( ACKZSUSObjToZS ) - sizeof(Header);
	}
};

struct ZSUSZooGS
{
	MessageHeader   Header;
	int				iUid;
	int				iZid;

	ZSUSZooGS() { 
		Header.MsgID = MSG_ZSUSZOOGS;
		Header.iSize = sizeof( ZSUSZooGS ) - sizeof(Header);
	}
};

struct ZSProMigGS
{
	MessageHeader   Header;
	int				iZid;

	ZSProMigGS() { 
		Header.MsgID = MSG_ZSPROMIGGS;
		Header.iSize = sizeof( ZSProMigGS ) - sizeof(Header);
	}
};

struct ACKZSProMigGS
{
	MessageHeader   Header;
	int				iZid;
	char			sZip[MAX_IP_LEN];
	char			sZport[MAX_PORT_LEN];

	ACKZSProMigGS() { 
		Header.MsgID = MSG_ACKZSPROMIGGS;
		Header.iSize = sizeof( ACKZSProMigGS ) - sizeof(Header);
	}
};

struct ZSProDoneGS
{
	MessageHeader   Header;
	int				iZid_S;
	int				iZid_D;

	ZSProDoneGS() { 
		Header.MsgID = MSG_ZSPRODONEGS;
		Header.iSize = sizeof( ZSProDoneGS ) - sizeof(Header);
	}
};

struct ZSProDoneMX
{
	MessageHeader   Header;
	int				iZid_S;
	int				iZid_D;

	ZSProDoneMX() { 
		Header.MsgID = MSG_ZSPRODONEMX;
		Header.iSize = sizeof( ZSProDoneMX ) - sizeof(Header);
	}
};
#endif