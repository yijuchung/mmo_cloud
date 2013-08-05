#ifndef _NET_
#define _NET_
#include <winsock2.h>
#include "messages.h"
#define WINSOCK_VERSION			MAKEWORD(2,2)

int SendTCPPacket( int sock , char *buf , int size);

int RecvTCPPacket( int sock , char *buf );

int RecvTCPPacket( int sock , char *buf , int size);
#endif