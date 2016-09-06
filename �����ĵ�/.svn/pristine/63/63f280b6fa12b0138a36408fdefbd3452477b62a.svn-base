////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：GatewayAttempterSink.cpp
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
#include "GatewayAttempterSink.h"
#include "gatewaysvr.h"
#include "globaldef.h"
////////////////////////////////////////////////////////////////////////////
CGatewayAttempterSink::CGatewayAttempterSink()
{
	m_pITcpIOAttemperEngine = NULL;
	m_pConnectInfo = NULL;
	m_dwMaxConnectNum = 0;
}
CGatewayAttempterSink::~CGatewayAttempterSink()
{
	if(m_pConnectInfo != NULL)
	{
		delete [] m_pConnectInfo;
		m_pConnectInfo = NULL;
	}
}
//接口查询
void *  CGatewayAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//初始化
bool CGatewayAttempterSink::InitServer(DWORD dwMaxConnectNum, char* ip, WORD wdPort)
{
	
	//初始化集群网络服务端 
	if(m_tcpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("创建网关服务器服务端失败，错误描述：%s",m_tcpNetworkHelper.GetErrorMessage()); 
		return false;
	}
	m_pITcpIOAttemperEngine = m_tcpNetworkHelper.GetInterface(); 
	m_pITcpIOAttemperEngine->SetAttemperEngineSink((IAttemperEngineSink*)this);
	
	m_dwMaxConnectNum = dwMaxConnectNum;

	ASSERT(m_pConnectInfo == NULL);
	if(m_pConnectInfo != NULL)
	{
		delete [] m_pConnectInfo;
		m_pConnectInfo = NULL;
	}

	m_pConnectInfo = new tagConnectInfo[m_dwMaxConnectNum];
	memset(m_pConnectInfo,-1,sizeof(tagConnectInfo) * m_dwMaxConnectNum);
	 
	//DWORD		dwInitMemSize;				// DataStorage 的内存初始化大小 - 变长数据存储 
	//DWORD		dwIocpThreadCount;			// IOCP的工作线程个数 : < SystemInfo.dwNumberOfProcessors * 5
	//DWORD       dwRevcQueueServerNum;		// 接收队列个数
	//DWORD		dwInitSendMemSize;			// 每个客户端分配的初始发送缓冲区大小(单位：KB）
	//DWORD		dwMaxSendMemSize;			// 客户端发送缓冲区最大值，超出断开连接(单位：字节）
	//DWORD		dwMaxAcceptCount;			// 最大连接数，默认1024
	AttemperParam attempParam;
	attempParam.dwInitMemSize = 1;
	attempParam.dwIocpThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRevcQueueServerNum = attempParam.dwIocpThreadCount;// / 2;
	attempParam.dwMaxSendMemSize = 8192000;
	m_pITcpIOAttemperEngine->InitServer(&attempParam);
	
	OUT_INFOEX("初始化网关服务器完成,服务线程%u,队列线程%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);
	m_ipPortID.wPort = wdPort;
	//m_ipPortID.dwIp = inet_addr(IP);
	if(NULL==ip || !wdPort)
	{
	}
	return true;
}
//启动
bool CGatewayAttempterSink::StartServer(char *IP)
{
	DWORD dwPort = m_ipPortID.wPort;
	if(dwPort > 0xFFFE || !IP)
	{
		OUT_ERROREX("启动网关服务器失败，本地服务端口非法%u",dwPort);
		StopServer();
		return false;
	}
	WORD wPort = (WORD)dwPort;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	if(m_pITcpIOAttemperEngine->StartServer(wPort,IP) == false)
	{
		OUT_ERROREX("启动网关服务器服务端失败，监听地址(%s:%u)",IP,wPort); 
		StopServer();
		return false;
	}
	OUT_INFOEX("启动网关服务器完成,本地服务监听(%s:%u)",IP,wPort);
	return true;
}
//停止
bool CGatewayAttempterSink::StopServer()
{
	m_pITcpIOAttemperEngine->StopServer();
	OUT_INFO("停止多网关服务器完成"); 
	return true;
}
#define BUILD_STREAMID(p,id) DWORD dwRoomID = 0,dwUserID = 0;memcpy(&dwRoomID,p,4);memcpy(&dwUserID,p + 4,4);\
	dwRoomID = ntohl(dwRoomID);dwUserID = ntohl(dwUserID);id = MAKE_STREAM_IDENTITY(dwRoomID,dwUserID)
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//网络连接消息
bool CGatewayAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//超过最大连接数
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("网关服务器服务端超过最大连接数%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u,断开该客户端连接",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("网关服务器服务端:客户端连接IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u",
		pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 

	m_pConnectInfo[pAcceptEvent->dwIndexID].dwRoundID = pAcceptEvent->dwRoundID;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//网络读取消息
bool CGatewayAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//服务器登录
	case SYS_LOGIN_REQ:
		return OnReqSvrLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//上传负载信息
	case TRANS_DATA_REQ:
		return OnReqUpMediaLoad(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//用户登录
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//用户请求分配服务器
	case USER_ASK_FOR_SERVER_REQ:
		return OnReqSvrAllocate(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//心跳
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//网络关闭消息
bool CGatewayAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("媒体服务端：客户端连接断开id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	m_pConnectInfo[pCloseEvent->dwIndexID].dwRoundID = INVALID_DWORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pCloseEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//心跳
bool CGatewayAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,NULL,0);
}
//服务器登录
bool CGatewayAttempterSink::OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= 8);
	if(pData == NULL || wDataSize < 8) 
	{
		OUT_WARNEX("收到非法的服务器登录数据(size >= 8),size %u",wDataSize);
		return false;
	}

	UNDWORD udwID = 0;
	memcpy(&udwID,pData,8);
	udwID = ntohl64(udwID); 

	OUT_DEBUGEX("服务器登录:ID %llu",udwID); 
	m_pConnectInfo[dwIndexID].udwID = udwID;
	m_pConnectInfo[dwIndexID].wClientType = ST_DATA_TRANS_SVR;
	m_pConnectInfo[dwIndexID].wStatus = 1;

	int nRet = 0;
	command.dwCmd = SYS_LOGIN_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//客户端登陆
bool CGatewayAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("收到非法的用户登录数据(size >= 4),size %u",wDataSize);
		return false;
	}
	 
	DWORD dwUserID = 0; 

	char *pBuffer = pData;
	memcpy(&dwUserID,pBuffer,4);
	pBuffer += 4;
	dwUserID = ntohl(dwUserID);
	 
	OUT_DEBUGEX("用户登陆:%llu",dwUserID); 

	m_pConnectInfo[dwIndexID].udwID = dwUserID;
	m_pConnectInfo[dwIndexID].wClientType = ST_USER_CLIENT;
	m_pConnectInfo[dwIndexID].wStatus = 1;
	 
	int nRet = 0;
	command.dwCmd = USER_LOGIN_SVR_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4); 
}
//验证登录
inline bool CGatewayAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("连接index:%u roundID:%u 操作命令字0x%x，请求非法，连接未登录",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//服务器上传负载
bool CGatewayAttempterSink::OnReqUpMediaLoad(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("收到非法的Media服务器负载数据(size >= 4),size %u",wDataSize);
		return false;
	}
	 
	DWORD dwIP = 0; 
	WORD wdPort = 0;
	DWORD dwLoad = 0;

	char *pBuffer = pData;
	memcpy(&dwIP,pBuffer,4);
	pBuffer += 4;
	dwIP = ntohl(dwIP);
	memcpy(&wdPort,pBuffer,2);
	wdPort = ntohs(wdPort);
	pBuffer += 2;
	memcpy(&dwLoad,pBuffer,4);
	dwLoad = ntohl(dwLoad);
	 
	struct in_addr client;
	client.s_addr = dwIP;
	strncpy(m_mediaLoadMap[dwIndexID].ip,inet_ntoa(client),IP_LEN);
	m_mediaLoadMap[dwIndexID].port = wdPort;	
	m_mediaLoadMap[dwIndexID].load = dwLoad;	
	OUT_DEBUGEX("收到服务器(%s:%u)上报负载:%u",inet_ntoa(client),wdPort,dwLoad); 

	unsigned char nRet = 0;
	command.dwCmd = TRANS_DATA_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,1); 
}
//用户请求分配服务器
bool CGatewayAttempterSink::OnReqSvrAllocate(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("收到非法的请求分配服务器数据(size >= 4),size %u",wDataSize);
		return false;
	}
	
	DWORD dwIP = 0; 
	WORD wdPort = 0;
	DWORD dwLoad = 0XFFFFFFFF;
	unsigned char ret = 0;
 
	if(m_mediaLoadMap.empty())
	{
		OUT_WARNEX("没有可以分配的服务器,ret=%u",ret);
	}
	else
	{
		std::map<DWORD,MEDIA_SERVER_INFO>::const_iterator cit = m_mediaLoadMap.begin();
		for(; cit != m_mediaLoadMap.end(); ++cit)
		{
			if(cit->second.load < dwLoad) 
			{
				dwLoad = cit->second.load;
				dwIP = inet_addr(cit->second.ip);
				wdPort = cit->second.port;
				ret = 1;
			}
		}
	}
	
	char szBuffer[11];
	memcpy(szBuffer,&ret,1);
	memcpy(szBuffer + 1,&dwIP,4);
	memcpy(szBuffer + 5,&wdPort,2);
	DWORD dwNum = htonl(dwLoad);
	memcpy(szBuffer + 7,&dwNum,4);
	 
	struct in_addr client;
	client.s_addr = dwIP;
	OUT_DEBUGEX("收到分配服务器请求，分配%s,服务器(%s:%u),负载 %u",(ret==0)?"失败":"成功",inet_ntoa(client),wdPort,dwLoad); 

	command.dwCmd = USER_ASK_FOR_SERVER_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,szBuffer,11); 
}
