////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：MediaUdpAttempterSink.cpp
// Project Code ：服务端回调，主要是客户端连接（用户）
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 09 22, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "MediaUdpAttempterSink.h"
#include "MediaAttempterSink.h"
#include "mediasvr.h"
////////////////////////////////////////////////////////////////////////////
enum emBufferCode
{
	EM_BUFFER_CLUSTER				= -1,				//集群缓冲
	EM_BUFFER_MEDIA_DATA			= 0,				//下载的数据源缓冲
};
CMediaUdpAttempterSink::CMediaUdpAttempterSink(CMediaAttempterSink* pSvr)
{
	m_pIUdpAttemperEngine = NULL;
	m_pConnectInfo = NULL;
	m_pIClusterData = NULL;
	m_dwMaxConnectNum = 0;
	m_dwStudentNum = 0;
	m_pMediaSvr = pSvr;
}
CMediaUdpAttempterSink::~CMediaUdpAttempterSink()
{
	if(m_pConnectInfo != NULL)
	{
		delete [] m_pConnectInfo;
		m_pConnectInfo = NULL;
	}
}
//接口查询
void *  CMediaUdpAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//初始化
bool CMediaUdpAttempterSink::InitServer(CStreamManage *pStreamManage,IClusterData *pIClusterData,UNDWORD udwIpPort,DWORD dwMaxConnectNum /* = 500000*/)
{
	ASSERT(pStreamManage != NULL);
	ASSERT(pIClusterData != NULL);
	if(pStreamManage == NULL || pIClusterData == NULL)
		return false;

	m_pStreamManager = pStreamManage;
	m_pIClusterData = pIClusterData;
	m_ipPortID.udwIpPortID = udwIpPort;
	m_dwMaxConnectNum = dwMaxConnectNum;

	//初始化集群网络服务端 
	if(m_udpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("UDP 创建媒体服务器服务端失败，错误描述：%s",m_udpNetworkHelper.GetErrorMessage()); 
		return false;
	}
	m_pIUdpAttemperEngine = m_udpNetworkHelper.GetInterface(); 
	m_pIUdpAttemperEngine->SetAttemperEngineSink((IAttemperEngineSink*)this);
	
	ASSERT(m_pConnectInfo == NULL);
	if(m_pConnectInfo != NULL)
	{
		delete [] m_pConnectInfo;
		m_pConnectInfo = NULL;
	}

	m_pConnectInfo = new tagConnectInfo[m_dwMaxConnectNum];
	memset(m_pConnectInfo,-1,sizeof(tagConnectInfo) * m_dwMaxConnectNum);
	 
	//DWORD		dwInitMemSize;				// DataStorage 的内存初始化大小 - 变长数据存储 ： [1, 100] 单位M，默认1M
	//DWORD		dwIocpThreadCount;			// IOCP的工作线程个数 : 最大可用cpu核心数* 5
	//DWORD       dwRevcQueueServerNum;		// 接收队列个数,默认为dwIocpThreadCount
	//DWORD		dwRecvQueuePkgMaxNum;		// 接收队列最大数据包个数限制，默认600个(根据视频数据，视频10帧/S * 15pkg/帧 * 4s)，如果每个队列都超过了该大小，
	//										// 则动态增加队列和线程来处理新的连接，新增加的队列和线程不会随着用户或者负载的减少而销毁，会一直存在
	//DWORD		dwMaxSendMemSize;			// 客户端发送缓冲区最大值默认102400,单位字节，默认为（DWORD）-1，超出，设置的值，将清理掉未发送的数据

	AttemperParam attempParam;
	attempParam.dwInitMemSize = 1;
	attempParam.dwIocpThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRevcQueueServerNum = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRecvQueuePkgMaxNum = 600;
	attempParam.dwMaxSendMemSize = 102400;

	OUT_INFOEX("UDP 初始化媒体服务器完成,服务线程%u,队列线程%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);
	return true;
}
//启动
bool CMediaUdpAttempterSink::StartServer(const char *pszIp)
{
	ASSERT(pszIp != NULL);
	if(pszIp == NULL) return false;

	DWORD dwPort = m_ipPortID.wPort + 10000;
	if(dwPort > 0xFFFE)
	{
		OUT_ERROREX("UDP 启动媒体服务器失败，本地服务端口非法%u",dwPort);
		StopServer();
		return false;
	}
	WORD wPort = (WORD)dwPort;
	ASSERT(m_pIUdpAttemperEngine != NULL);
	if(m_pIUdpAttemperEngine->StartServer(wPort,pszIp) == false)
	{
		OUT_ERROREX("UDP 启动媒体服务器服务端失败，监听地址:%s,PORT：%u",pszIp,wPort); 
		StopServer();
		return false;
	}
	OUT_INFOEX("UDP 启动媒体服务器完成,集群端口%u,本地服务端口%u",m_ipPortID.wPort,wPort);
	return true;
}
//停止
bool CMediaUdpAttempterSink::StopServer()
{
	m_pIUdpAttemperEngine->StopServer();
	OUT_INFO("UDP 停止多媒体服务器完成"); 
	return true;
}
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//网络连接消息
bool CMediaUdpAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//超过最大连接数
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("UDP 媒体服务器服务端超过最大连接数%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u,断开该客户端连接",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("UDP 媒体服务器服务端:客户端连接IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u",
		pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 

	m_pConnectInfo[pAcceptEvent->dwIndexID].bIsPublish = false;
	m_pConnectInfo[pAcceptEvent->dwIndexID].dwRoundID = pAcceptEvent->dwRoundID;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wMediaType = EM_MEDIA_TYPE_NO;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//网络读取消息
bool CMediaUdpAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//用户登录
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//播放流
	case USER_PLAY_VIDEO_REQ:
		return OnReqPlayVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//播放流
	case USER_PLAY_AUDIO_REQ:
		return OnReqPlayAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//心跳
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//网络关闭消息
bool CMediaUdpAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("UDP 媒体服务端：客户端连接断开id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	//清理流数据,如果是老师发布，只需要修改状态为没有发布，如果是学生，删除房间中的学生，在看用户数是否为0，为0清理房间
	if(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID != (UNDWORD)-1)
	{
		CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
		if(pStreamItem != NULL)
		{
			//当前是上传数据
			int nUserCount = 0;
			//请求播放的是音频还是视频
			if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
			{
				nUserCount = pStreamItem->RemoveAudioUser(pCloseEvent->dwIndexID);
				OUT_DEBUGEX("UDP 学生%llu停止了播放音频数据，流ID：%llu ,音频用户数:%d,推送音频客户端数:%d",
					m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetPublishAudioClientCount());
				nUserCount += pStreamItem->GetPublishVideoClientCount();//房间还剩多少人
			}
			else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //视频
			{
				nUserCount = pStreamItem->RemoveVideoUser(pCloseEvent->dwIndexID);
				OUT_DEBUGEX("UDP 学生%llu停止了播放视频数据，流ID：%llu ,视频用户数:%d,推送视频客户端数:%d",
					m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,
					pStreamItem->GetPublishVideoClientCount());
				nUserCount += pStreamItem->GetPublishAudioClientCount();//房间还剩多少人
			}

			//如果房间音频没有发布，视频也没有发布，房间的用户数为0，清理掉房间
			if(pStreamItem->IsPublishAudio() == false && pStreamItem->IsPublishVideo() == false && nUserCount == 0)
			{  
				m_pStreamManager->RemoveStream(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
				OUT_DEBUGEX("UDP 清理流数据，流ID：%llu ,视频用户数:%d,音频用户:%d,推送音频客户端数:%d,推送视频客户端数:%d",
					pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount());
			}
		}
	}

	//如果在登录情况下断线，则负载减1
	if(EM_MEDIA_TYPE_NO != m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType
			&& ST_USER_CLIENT == m_pConnectInfo[pCloseEvent->dwIndexID].wClientType
			&& 1 == m_pConnectInfo[pCloseEvent->dwIndexID].wStatus)
	{
		if(m_dwStudentNum)	{
			--m_dwStudentNum;
			if(m_pMediaSvr)	m_pMediaSvr->SendGatewayServerLoad();
		}
	}

	m_pConnectInfo[pCloseEvent->dwIndexID].bIsPublish = false;
	m_pConnectInfo[pCloseEvent->dwIndexID].dwRoundID = INVALID_DWORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType = EM_MEDIA_TYPE_NO;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pCloseEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//心跳
bool CMediaUdpAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pIUdpAttemperEngine->SendDataCtrlCmd(dwRoundID,dwIndexID,command,NULL,0);
}
//客户端登陆
bool CMediaUdpAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("UDP 收到非法的用户登录数据(size >= 4),size %u",wDataSize);
		return false;
	}
	 
	DWORD dwUserID = 0; 

	char *pBuffer = pData;
	memcpy(&dwUserID,pBuffer,4);
	pBuffer += 4;
	dwUserID = ntohl(dwUserID);
	 
	OUT_DEBUGEX("UDP 用户登陆:%llu",dwUserID); 
	++m_dwStudentNum;
	if(m_pMediaSvr)	m_pMediaSvr->SendGatewayServerLoad();

	m_pConnectInfo[dwIndexID].udwID = dwUserID;
	m_pConnectInfo[dwIndexID].wClientType = ST_USER_CLIENT;
	m_pConnectInfo[dwIndexID].wStatus = 1;
	 
	int nRet = 0;
	command.dwCmd = USER_LOGIN_SVR_RESP;
	ASSERT(m_pIUdpAttemperEngine != NULL);
	return m_pIUdpAttemperEngine->SendDataCtrlCmd(dwRoundID,dwIndexID,command,(char*)&nRet,4); 
}
//用户下载视频数据
bool CMediaUdpAttempterSink::OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//用户类型是播放
	m_pConnectInfo[dwIndexID].bIsPublish = false;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_VIDEO;	//播放类型是视频
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 

	CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_pStreamManager->CreateSteamItem(udwStreamID);
	ASSERT(pStreamItem != NULL);
	//有play端先创建的流，去其他服务器请求数据
	//int nUserNum = 0;//可能出现多次请求其他服务器转发视频数据，在集群消息中处理
	pStreamItem->AddPlayVideoUser(dwIndexID,dwRoundID);
	if(pStreamItem->IsPublishVideo() == false || pStreamItem->GetCurrentVideoTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		char cbBuffer[24];
		memcpy(cbBuffer,pData,8);
		UNDWORD udwSvrID = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer + 8,&udwSvrID,8);
		
		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_VIDEO_REQ;
		playCmd.dwSequence = 0;
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,playCmd,cbBuffer,17);
		OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu视频数据,通过集群请求其他服务器数据",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

		int nRet = 0;
		command.dwCmd = USER_PLAY_VIDEO_RESP;
		return m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_VIDEO_RESP;
	m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu视频数据,当前请求用户数:%u,本机数据源状态:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamVideoUserCount(),pStreamItem->IsPublishVideo());
	
	//如果有音视频头数据，发送音视频头
	COMMAND headerCmd;
	char cbAvHeader[256];
	WORD wSize = 256;
	if(pStreamItem->GetVideoHeaderData(cbAvHeader,wSize) == true)
	{
		headerCmd.dwCmd = USER_VIDEO_HEADER_REQ;
		m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,headerCmd,cbAvHeader,wSize);
	}
	return true;
}
//用户下载音频数据
bool CMediaUdpAttempterSink::OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//用户类型是播放
	m_pConnectInfo[dwIndexID].bIsPublish = false;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_AUDIO;	//播放类型是音频
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 

	CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_pStreamManager->CreateSteamItem(udwStreamID);
	ASSERT(pStreamItem != NULL);
	//有play端先创建的流，去其他服务器请求数据
	pStreamItem->AddPlayAudioUser(dwIndexID,dwRoundID);
	//int nUserNum = 0;
	if(pStreamItem->IsPublishAudio() == false || pStreamItem->GetCurrentAudioTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		char cbBuffer[24];
		memcpy(cbBuffer,pData,8);
		UNDWORD udwSvrID = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer + 8,&udwSvrID,8);

		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_AUDIO_REQ;
		playCmd.dwSequence = 0;
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,playCmd,cbBuffer,17);
		OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu音频数据,通过集群请求其他服务器数据",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

		int nRet = 0;
		command.dwCmd = USER_PLAY_AUDIO_RESP;
		return m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_AUDIO_RESP;
	m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu音频数据,当前请求音频用户:%u,本机数据源状态:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamAudioUserCount(),pStreamItem->IsPublishAudio());
	
	//如果有音视频头数据，发送音视频头
	COMMAND headerCmd;
	char cbAvHeader[256];
	WORD wSize = 256;
	if(pStreamItem->GetAudioHeaderData(cbAvHeader,wSize) == true)
	{
		headerCmd.dwCmd = USER_AUDIO_HEADER_REQ;
		m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,headerCmd,cbAvHeader,wSize);
	}
	return true;
}
//验证登录
inline bool CMediaUdpAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("UDP 连接index:%u roundID:%u 操作命令字0x%x，请求非法，连接未登录",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//验证数据
inline bool CMediaUdpAttempterSink::CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= n);
	if(pData == NULL || wDataSize < n)
	{
		OUT_WARNEX("UDP 连接index:%u roundID:%u 登录类型:%u 命令字：0x%x数据非法 %u >= %u",dwIndexID,dwRoundID,m_pConnectInfo[dwIndexID].wClientType,dwCmd,wDataSize,n);
		return false;
	}
	return true;
}
//转发数据
static FILE *pFile = NULL;
bool CMediaUdpAttempterSink::TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pStreamItem != NULL);
	if(pStreamItem == NULL) return true;

	//转发数据
	CPlayUserMap userMap;
	if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
	{
		//printf(__FILE__" : %d : %s  777\r\n",__LINE__,__FUNCTION__);
		pStreamItem->GetPlayVideoUserList(0,&userMap);
		printf(__FILE__" : %d : %s  888\r\n",__LINE__,__FUNCTION__);
	}
	else if(command.dwCmd == USER_AUDIO_DATA_REQ	 || command.dwCmd == USER_AUDIO_HEADER_REQ)
	{
		pStreamItem->GetPlayAudioUserList(0,&userMap);
	}
	if(pFile == NULL)
	{
		pFile = fopen("./send.txt","a");
	}
	char szMsg[512];
	for(CPlayUserMapIt it = userMap.begin();it != userMap.end();it++)
	{
		if(pFile != NULL)
		{
			sprintf(szMsg,"USER:%llu CMD:0x%x size:%u INDEX:%u Round:%u \n",m_pConnectInfo[(*it).first].udwID,command.dwCmd,wDataSize,(*it).first,(*it).second);
			fwrite(szMsg,1,strlen(szMsg),pFile);
			fflush(pFile);
		}
		if(command.dwCmd == USER_VIDEO_HEADER_REQ || command.dwCmd == USER_AUDIO_HEADER_REQ || command.dwCmd == USER_AUDIO_DATA_REQ)
		{
			//if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
			//	printf(__FILE__" : %d : %s  999\r\n",__LINE__,__FUNCTION__);
			m_pIUdpAttemperEngine->SendDataCtrlCmd((*it).second,(*it).first,command,pData,wDataSize);
			if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
				printf(__FILE__" : %d : %s  AAA\r\n",__LINE__,__FUNCTION__);
		}
		else 
		{
			//if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
			//	printf(__FILE__" : %d : %s  999\r\n",__LINE__,__FUNCTION__);
			m_pIUdpAttemperEngine->SendData((*it).second,(*it).first,command,pData,wDataSize);
			if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
				printf(__FILE__" : %d : %s  AAA\r\n",__LINE__,__FUNCTION__);
		}
	}
	if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
		printf(__FILE__" : %d : %s  BBBB\r\n",__LINE__,__FUNCTION__);
	return true;
}
