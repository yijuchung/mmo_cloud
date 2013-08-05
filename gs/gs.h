#include "def.h"

struct ZoneServer
{
	int		iZid;
	int		iSocket;
	char	sName[MAX_NAME_LEN];
	bool	bOpen;
	char	sIP[MAX_IP_LEN];
	char	sPort[MAX_PORT_LEN];
	int		iX;
	int		iY;
	int		iW;
	int		iH;
	int		iUser;
};

struct UserData
{
	char	sUserName[MAX_NAME_LEN];
	char	sPass[MAX_NAME_LEN];
	char	sZone[MAX_ZONE_NAME];
	bool	bOnline;
};