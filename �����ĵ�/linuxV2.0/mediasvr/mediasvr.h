////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��dediasvr.h
// Project Code ������ת���ⲿʹ��ͷ�ļ�
// Abstract     ��
// Reference    ��
//
// Version      ��1.0
// Author       ��LiuJun
// Accomplished date �� 04 17, 2015
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
*	���������ֹͣ��������������״̬���INFO��־
*	������ݿ��Կ�������״̬���ߵ�ǰִ�еĲ���ΪDEBUG��־
*	�������յ��Ŀͻ��˵����ӡ����߽������ݵ�ʧ�ܣ����ǲ�Ӱ������������WARN��־
*	����ʧ�ܡ�����ʧ�ܵ��³���������������ERROR��־
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


//�����û��б���и���
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