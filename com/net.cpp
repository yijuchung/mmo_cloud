#include "net.h"

int SendTCPPacket( int sock , char *buf , int size)
{
	int offset = 0;
	int len = 0;

	//char buffer[MAX_PACKET_SIZE];

	do{
		len = send( sock , (char *)buf+offset , size , 0 );
		if(len == -1)
		{
			printf("send error = %d\n",WSAGetLastError());
			return len;
		}else if(len == 0)
			printf("sock close!!\n");

		//printf("len = %d\n", len);
		offset = offset + len;
		//printf("offset = %d\n", offset);
	}while(offset != size);
}

int RecvTCPPacket( int sock , char *buf )
{
	int offset = 0;
	int len = 0;

	len = recv( sock, (char *)buf, sizeof(MessageHeader), 0);
	//printf("recv header len = %d\n", len);
	if(len == -1)
	{
		printf("recv error = %d len = %d\n",WSAGetLastError(),len);
		return len;
	}else if(len == 0)
	{
		printf("sock close!!\n");
		return len;
	}

	MessageHeader * Mh = (MessageHeader*)buf;

	//buftemp = (char*)malloc(Mh->iSize);
	int len2 = 0;
	len2 = recv( sock, (char *)buf+len, Mh->iSize , 0);
	//printf("recv total len = %d\n", len+len2);
	return len+len2;
}

int RecvTCPPacket( int sock , char *buf , int size)
{
	int len = recv( sock, (char *)buf, size , 0);
	return len;
}