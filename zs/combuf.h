#include <vector>
#include "def.h"
#include "messages.h"

struct pacbuf
{
	ZSUSComToUS zucu;
	int			time;
};

struct TempUser
{
	bool						used;
	int							uid;
	std::vector<struct pacbuf>	pac;
};

void PrintTempUser();

void AddTempUser( int uid );

void AddPacket( int uid, char *pac, int time );

void DelTempUser( int uid );