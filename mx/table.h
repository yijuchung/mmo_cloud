#include "net.h"

struct Cache
{
	bool		bDirtybit;
	int			iUid;
	int			iZid;
	char		sIP[MAX_IP_LEN];
	char		sPort[MAX_PORT_LEN];	
};

struct Zone
{
	int			iZid;
	bool		bOnline;
	char		sIP[MAX_IP_LEN];
	char		sPort[MAX_PORT_LEN];
};

struct UserZone
{
	bool		bDirtybit;
	int			iUid;
	int			iZid;
};

bool AddZone (int zid, char *zip, char *zport);
bool AddUser (int uid, int zid);
bool AddCache (int uid, int zid, char *ip, char *port);
void DelUser ( int uid );
void UpdateUserTable ( int uid , int zid );
void DelCache ( int uid , int zid );
void UpdateCache ( int zid, int zidn );
int UpdateCachePro (int zid, int zidn, char *ip, char *port);
int FindZoneTable (int zid);

class	 User
{
public:

	User();
	~User();
	
	bool	Create( int iSock, int uid );
	void	Destroy();

	int		SendPacket( char * pBuffer, int length );
	int		RecvPacket( char * pBuffer, int length );

	bool	bConnected;
	int		iSocket;
	int		iUid;
	int		iSeqNum;
};