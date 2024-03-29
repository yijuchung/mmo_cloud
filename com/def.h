#ifndef _DEF_
#define _DEF_
#define MAX_WAIT_LOG		5
#define MAP_RANGE_MAX		500			// 一個 zone 多大
#define USER_VISION_MAX		200			// 一個 user vision 多大
#define MAX_PACKET_SIZE     1024
#define MAX_USERS           100			// 所有遊戲註冊會員
#define MAX_USERS_ZONE		100			// 一個 zone 可以 handle 的會員數
#define MAX_USERS_MX		100			// 一個 mx 可以 handle 的會員數
#define MAX_USERS_CLI		100			// 一個 cli 可以看到的其他玩家數
#define MAX_ZONE			9
#define MAX_TEMP_ZONE		5			// process migration
#define MAX_CACHE			200			// cache 可以有多少 entry
#define IDLE_TIME			80
#define SHOW_WINDOW			true

#define MAX_SYNC_OBJ		100			// 同時 sync 最大物件

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