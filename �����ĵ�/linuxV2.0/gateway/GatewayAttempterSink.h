////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：GatewayAttempterSink.h
// Project Code ：服务端回调，主要是客户端连接（用户）
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 07 28, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#ifndef __LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
#define __LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "cluster.h"
#include <map>
using namespace std;

////////////////////////////////////////////////////////////////////////////
//定义连接信息
typedef struct tagConnectInfo
{
	WORD			wStatus;			//0未登录，1登录
	WORD			wClientType;		//取值globaldef.h:CONNECT_TYPE
	DWORD			dwRoundID;
	UNDWORD			udwID;				//客户端ID
	UNDWORD			udwPublishStreamID; //上传的流ID
}CONNECT_INFO;


////定义带选择媒体服务器信息
typedef struct tagMediaInfo
{
	tagMediaInfo()
		:port(0),load(0)
	{
		memset(ip,0,sizeof(ip));
	}
	char ip[IP_LEN+1];
	WORD port;
	DWORD load;
}MEDIA_SERVER_INFO;

////////////////////////////////////////////////////////////////////////////
class CGatewayAttempterSink:public IAttemperEngineSink
{
private:
	CTcpNetKernelHelper					m_tcpNetworkHelper;
	ITcpIOAttemperEngine				*m_pITcpIOAttemperEngine;

private:
	CONNECT_INFO					*m_pConnectInfo;			//客户端连接索引

private:
	IPPORTID						m_ipPortID;					//本机
	DWORD							m_dwMaxConnectNum;			//最大连接数

	//Media负载信息
private:
	std::map<DWORD,MEDIA_SERVER_INFO> m_mediaLoadMap;

public:
	CGatewayAttempterSink();
	virtual ~CGatewayAttempterSink();

	//from IUnknownEx
public:
	//是否有效
	virtual bool  IsValid() {return this != NULL;}
	//释放对象
	virtual bool  Release(){if(this != NULL) delete this;return true;}
	//接口查询
	virtual void *  QueryInterface(DWORD dwQueryVer);

public:
	//初始化
	bool InitServer(DWORD dwMaxConnectNum, char* ip, WORD wdPort);
	//启动
	bool StartServer(char* IP);
	//停止
	bool StopServer();

	//from IAttemperEngineSink
public:
	//网络连接消息
	virtual bool OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent);
	//网络读取消息
	virtual bool OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize);
	//网络关闭消息
	virtual bool OnCloseEvent(NTY_IOCloseEvent *pCloseEvent);
	 
private:
	//心跳
	bool OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command);
	//验证登录
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//客户端登陆
	bool OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//服务器登录
	bool OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	//服务器负载信息
	bool OnReqUpMediaLoad(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	bool OnReqSvrAllocate(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
};
#endif //__LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
