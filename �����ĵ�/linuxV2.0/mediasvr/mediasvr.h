////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：dediasvr.h
// Project Code ：数据转发外部使用头文件
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 04 17, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#ifndef __LIUJUN_20150728_MEDIA_SERVER_H
#define __LIUJUN_20150728_MEDIA_SERVER_H
#include <iostream>
#include <cstdlib>
#include <stdio.h> 
using namespace std;
#include "Logger.h"
////////////////////////////////////////////////////////////////////////////
/**
*	组件的启动停止、服务器的运行状态输出INFO日志
*	输出数据可以看到连接状态或者当前执行的操作为DEBUG日志
*	服务器收到的客户端的连接、或者解析数据等失败，但是不影响服务器，输出WARN日志
*	启动失败、配置失败导致程序服务器运行输出ERROR日志
**/ 

#ifndef OUT_INFO
#define OUT_INFO(s)  g_pLogger->OutInfo(__FILE__,__LINE__,__FUNCTION__,s) 
#endif
#ifndef OUT_INFOEX
#define OUT_INFOEX(s,...)  g_pLogger->OutInfo(__FILE__,__LINE__,__FUNCTION__,s,__VA_ARGS__) 
#endif

#ifndef OUT_DEBUG
#define OUT_DEBUG(s) g_pLogger->OutDebug(__FILE__,__LINE__,__FUNCTION__,s) 
#endif
#ifndef OUT_DEBUGEX
#define OUT_DEBUGEX(s,...) g_pLogger->OutDebug(__FILE__,__LINE__,__FUNCTION__,s,__VA_ARGS__) 
#endif

#ifndef OUT_WARN
#define OUT_WARN(s) g_pLogger->OutWarn(__FILE__,__LINE__,__FUNCTION__,s) 
#endif
#ifndef OUT_WARNEX
#define OUT_WARNEX(s,...) g_pLogger->OutWarn(__FILE__,__LINE__,__FUNCTION__,s,__VA_ARGS__) 
#endif

#ifndef OUT_ERROR
#define OUT_ERROR(s) g_pLogger->OutError(__FILE__,__LINE__,__FUNCTION__,s)
#endif
#ifndef OUT_ERROREX
#define OUT_ERROREX(s,...) g_pLogger->OutError(__FILE__,__LINE__,__FUNCTION__,s,__VA_ARGS__)
#endif


//定义用户列表队列个数
#ifndef USER_PLAY_LIST_NUM				
#define  USER_PLAY_LIST_NUM				20
#endif

#ifndef MAKE_STREAM_IDENTITY
#define MAKE_STREAM_IDENTITY(r,u) ((((UNDWORD)r) << 32) | (UNDWORD)u)
#endif 

//#ifndef BUILD_STREAMID
#define BUILD_STREAMID(p,id) DWORD dwRoomID = 0,dwUserID = 0;memcpy(&dwRoomID,p,4);memcpy(&dwUserID,p + 4,4);\
	dwRoomID = ntohl(dwRoomID);dwUserID = ntohl(dwUserID);id = MAKE_STREAM_IDENTITY(dwRoomID,dwUserID)
//#endif
#endif //__LIUJUN_20150728_MEDIA_SERVER_H