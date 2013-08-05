#ifndef _DEF_
#define _DEF_
#define MAX_WAIT_LOG		5
#define MAP_RANGE_MAX		500			// �@�� zone �h�j
#define USER_VISION_MAX		200			// �@�� user vision �h�j
#define MAX_PACKET_SIZE     1024
#define MAX_USERS           100			// �Ҧ��C�����U�|��
#define MAX_USERS_ZONE		100			// �@�� zone �i�H handle ���|����
#define MAX_USERS_MX		100			// �@�� mx �i�H handle ���|����
#define MAX_USERS_CLI		100			// �@�� cli �i�H�ݨ쪺��L���a��
#define MAX_ZONE			9
#define MAX_TEMP_ZONE		5			// process migration
#define MAX_CACHE			200			// cache �i�H���h�� entry
#define IDLE_TIME			80
#define SHOW_WINDOW			true

#define MAX_SYNC_OBJ		100			// �P�� sync �̤j����

#define MAX_HP				10000
#define MAX_MP				100

#define TOTAL_MAP_X			((float)MAP_RANGE_MAX * pow((float)MAX_ZONE,0.5f))
#define TOTAL_MAP_Y			((float)MAP_RANGE_MAX * pow((float)MAX_ZONE,0.5f))
#define VISIBLE_SCOPE		100

#define MAX_NAME_LEN		10
#define MAX_ZONE_NAME		10
#define MAX_IP_LEN			16
#define MAX_PORT_LEN		6

#define PI					3.14159265
#endif

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>