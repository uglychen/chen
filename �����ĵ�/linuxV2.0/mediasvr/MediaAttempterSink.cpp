////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：MediaAttempterSink.cpp
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
#include "ReadIniFile.h"
#include "MediaAttempterSink.h"
#include "mediasvr.h"
////////////////////////////////////////////////////////////////////////////
enum emBufferCode
{
	EM_BUFFER_CLUSTER				= -1,				//集群缓冲
	EM_BUFFER_MEDIA_DATA			= 0,				//下载的数据源缓冲
};
CMediaAttempterSink::CMediaAttempterSink()
	:m_udpAttempterSink(this),
	m_dwGatewayIndex(0),
	m_dwGatewayRound(0),
	m_dwTeacherNum(0),
	m_dwStudentNum(0)
{
	m_pITcpAttemperEngine = NULL;
	m_pConnectInfo = NULL;
	m_pIClusterEngine = NULL;
	m_pIClusterData = NULL;
	m_dwMaxConnectNum = 0;
}
CMediaAttempterSink::~CMediaAttempterSink()
{
	if(m_pConnectInfo != NULL)
	{
		delete [] m_pConnectInfo;
		m_pConnectInfo = NULL;
	}
	ClearFreePublishInfo();
}
//接口查询
void *  CMediaAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IClusterEvent,dwQueryVer);
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE(IClientAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//添加发布音频状态，如果已经存在，表明已经有连接发布该服务器的音频了，
//不需要在重新发布,默认为true，发布成功，主要防止重复发布
bool CMediaAttempterSink::AddPublishAudio(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_audioDstSvrPublishStateMapLock);
	if(m_audioDstSvrPublishStateMap.find(udwSvrID) != m_audioDstSvrPublishStateMap.end())
		return false;
	m_audioDstSvrPublishStateMap[udwSvrID] = true;
	return true;
}
//删除音频发布状态
bool CMediaAttempterSink::RemovePublishAudio(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_audioDstSvrPublishStateMapLock);
	m_audioDstSvrPublishStateMap.erase(udwSvrID);
	return true;
}
//添加发布事频状态，如果已经存在，表明已经有连接发布该服务器的视频了，
//不需要在重新发布,默认为true，发布成功，主要防止重复发布
bool CMediaAttempterSink::AddPublishVideo(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_videoDstSvrPublishStateMapLock);
	if(m_videoDstSvrPublishStateMap.find(udwSvrID) != m_videoDstSvrPublishStateMap.end())
		return false;
	m_videoDstSvrPublishStateMap[udwSvrID] = true;
	return true;
}
//删除视频发布状态
bool CMediaAttempterSink::RemovePublishVideo(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_videoDstSvrPublishStateMapLock);
	m_videoDstSvrPublishStateMap.erase(udwSvrID);
	return true;
}
//获取空闲的信息结构,没有的话new新的
PublishInfo* CMediaAttempterSink::GetFreePublishInfo()
{
	PublishInfo *pPublishInfo = NULL;
	CThreadLockHandle lockHandle(&m_freePublishInfoListLock);
	if(m_freePublishInfoList.size() > 0)
	{
		pPublishInfo = m_freePublishInfoList.front();
		m_freePublishInfoList.pop_front();
	}
	lockHandle.UnLock();
	if(pPublishInfo == NULL) pPublishInfo = new PublishInfo();
	return pPublishInfo;
}
//释放推送的信息结构
void CMediaAttempterSink::FreePublishInfo(PublishInfo* pPublishInfo)
{
	ASSERT(pPublishInfo != NULL);
	if(pPublishInfo == NULL) return;
	CThreadLockHandle lockHandle(&m_freePublishInfoListLock);
	m_freePublishInfoList.push_back(pPublishInfo);
}
//删除所有空闲的信息结构
void CMediaAttempterSink::ClearFreePublishInfo()
{
	PublishInfo *pPublishInfo = NULL;
	CThreadLockHandle lockHandle(&m_freePublishInfoListLock);
	for(CPublishInfoListIt it = m_freePublishInfoList.begin(); it != m_freePublishInfoList.end();it++)
	{
		pPublishInfo = (*it);
		if(pPublishInfo != NULL) delete pPublishInfo;
	}
	m_freePublishInfoList.clear();
}
//初始化
bool CMediaAttempterSink::InitServer(DWORD dwMaxConnectNum /* = 500000*/)
{
	if(m_clusterHelper.CreateInstance() == false)
	{
		 OUT_ERROREX("创建媒体服务器集群组件失败:%s",m_clusterHelper.GetErrorMessage());
		 return false;
	 }
	 m_pIClusterEngine = m_clusterHelper.GetInterface();
	 IClusterEvent *pIClusterEvent = (IClusterEvent*)this->QueryInterface(VER_IClusterEvent);
	 m_pIClusterEngine->SetIClusterEventNotify(pIClusterEvent);
	 m_pIClusterData = (IClusterData*)m_pIClusterEngine->QueryInterface(VER_IClusterData);
	 if((m_ipPortID.udwIpPortID = m_pIClusterEngine->InitCluster()) == ((UNDWORD)-1))
	 {
		 OUT_ERROR("初始化集群组件失败");
		 return false;
	 }
	 OUT_INFOEX("初始化媒体服务器集群完成，ID:0x%llx  IP:0x%x PORT:%u",m_ipPortID.udwIpPortID,m_ipPortID.dwIp,m_ipPortID.wPort);
		//初始化集群网络服务端 
	if(m_tcpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("创建媒体服务器服务端失败，错误描述：%s",m_tcpNetworkHelper.GetErrorMessage()); 
		return false;
	}
	m_pITcpAttemperEngine = m_tcpNetworkHelper.GetInterface(); 
	m_pITcpAttemperEngine->SetAttemperEngineSink((IAttemperEngineSink*)this);
	m_pITcpAttemperEngine->SetClientAttemperEngineSink((IClientAttemperEngineSink*)this);
	
	m_dwMaxConnectNum = dwMaxConnectNum;

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
	//初始化
	m_pITcpAttemperEngine->InitServer(&attempParam);

	OUT_INFOEX("初始化媒体服务器完成,服务线程%u,队列线程%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);

	//初始化UDP服务
	if(m_udpAttempterSink.InitServer(&m_streamManager,m_pIClusterData,m_ipPortID.udwIpPortID,dwMaxConnectNum) == false)
		return false;

	if(m_transAVDataEngine.InitTransAVDataEngine(m_udpAttempterSink.GetIUdpAttemperEngine(),m_pITcpAttemperEngine,&m_streamManager,sysconf(_SC_NPROCESSORS_ONLN)) == false)
	{
		OUT_ERROR("初始化多媒体数据转发队列失败");
	}

	
	return true;
}
//启动
bool CMediaAttempterSink::StartServer(const char *pszIp)
{
	ASSERT(m_pIClusterEngine != NULL);
	if(m_pIClusterEngine->StartCluster() == false)
	{
		OUT_ERROR("启动媒体服务器集群失败"); 
		StopServer();
		return false; 
	}

	DWORD dwPort = m_ipPortID.wPort + 10000;
	if(dwPort > 0xFFFE)
	{
		OUT_ERROREX("启动媒体服务器失败，本地服务端口非法%u",dwPort);
		StopServer();
		return false;
	}

	if(m_transAVDataEngine.StartTransAVDataEngine() == false)
	{
		OUT_ERROR("启动媒体转发队列失败"); 
		StopServer();
		return false;
	}

	WORD wPort = (WORD)dwPort;
	ASSERT(m_pITcpAttemperEngine != NULL);
	if(m_pITcpAttemperEngine->StartServer(wPort,TEXT("0.0.0.0")) == false)
	{
		OUT_ERROREX("启动媒体服务器服务端失败，监听地址:0.0.0.0,PORT：%u",wPort); 
		StopServer();
		return false;
	}
	if(m_pITcpAttemperEngine->StartClientService() == false)
	{
		OUT_ERROR("启动媒体服务器客户端服务失败"); 
		StopServer();
		return false;
	}

	//读取配置文件
	//创建服务器间连接
	TCHAR szConfigPath[] = TEXT("./gate.ini");
	TCHAR szApp[] = TEXT("SYSCONFIG");

	CReadIniFile readFile;
	if(readFile.SetFileName(szConfigPath) == false)
	{
		OUT_ERROREX("读取配置文件:%s失败",szConfigPath);
		return false;
	}

	char IP[IP_LEN+1] = {0};
	strncpy(IP,"0.0.0.0",strlen("0.0.0.0"));
	DWORD dwLength = readFile.GetStringValue(szApp,TEXT("BIND_IP"),TEXT("0.0.0.0"),IP,IP_LEN);
	WORD wdPort = readFile.GetIntValue(szApp,TEXT("BIND_PORT"),-1);
	if(!dwLength || !wdPort)
	{
		OUT_ERROR("读取网关BIND的IP和端口错误");
		return false;
	}

	PublishInfo pPublishInfo;
	pPublishInfo.wSrcServerType = MEDIA_SERVER;
	pPublishInfo.wDstServerType = GATEWAY_SERVER;

	DWORD ip = (ntohl)(inet_addr(IP));
	if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)&pPublishInfo,ip,wdPort) == false)
	{
		OUT_ERROR("连接到网关服务器失败");
		StopServer();
		return false;
	}
	OUT_INFOEX("启动媒体服务器完成,集群端口%u,本地服务端口%u",m_ipPortID.wPort,wPort);
	//启动UDP服务
	if(m_udpAttempterSink.StartServer(pszIp) == false)
		return false;
	return true;
}
//停止
bool CMediaAttempterSink::StopServer()
{
	m_transAVDataEngine.StopTransAVDataEngine();
	m_udpAttempterSink.StopServer();
	m_pIClusterEngine->StopCluster();
	m_pITcpAttemperEngine->StopServer();
	OUT_INFO("停止多媒体服务器完成"); 
	return true;
}
//清理资源
void CMediaAttempterSink::RecycleResource()
{
	m_streamManager.ClearStreamResource();
}
//集群事件通知
void CMediaAttempterSink::ClusterEventNotify(tagClusterEvent *pClusterEvent)
{
	ASSERT(pClusterEvent != NULL);
	if(pClusterEvent == NULL) return;

	OUT_INFOEX("集群事件通知：type(1,推送服务器信息;2,推送完成;3,服务器变更通知;4,加入集群通知;5,集群退出通知):%d code:%d ADDR:0x%llx",pClusterEvent->type,pClusterEvent->nCode,pClusterEvent->udwAddrID);
	switch(pClusterEvent->type)
	{
	case EM_CLUSTER_PUSH_ADDR_NOTIFY:
		{ 
			OUT_INFOEX("集群转发消息：集群服务器列表推送:0x%llx",pClusterEvent->udwAddrID);
			break;
		}
	case EM_CLUSTER_PUSH_ADDR_OVER_NOTIFY:
		{
			//清理状态为0的服务器
			OUT_INFO("集群转发消息：集群服务器列表推送完成");
			break;
		}
	case EM_CLUSTER_SVR_CHANAGE_NOTIFY:
		{
			OUT_INFOEX("集群转发消息：集群服务器 0x%llx 变更状态 %d",pClusterEvent->udwAddrID,pClusterEvent->nCode);
			break; 
		}
	case EM_CLUSTER_JOINE_NOTIFY:
		{
			OUT_INFO("集群转发消息：本服务器加入集群成功");
			break; 
		}
	case EM_CLUSTER_EXIT_NOTIFY:
		{
			if(pClusterEvent->nCode == 0)
				OUT_INFO("集群转发消息：服务器退出集群成功");
			else
				OUT_WARNEX("集群转发消息：警告：服务器已退出了集群，描述：%s",pClusterEvent->szBuffer);
			break;
		}
	default:
		OUT_WARNEX("集群转发消息：集群事件类型无效:type %d  code %d",pClusterEvent->type,pClusterEvent->nCode);
		break;
	}
}
//数据转发,集群抛到上层的数据直接是用户指令，直接处理数据
void CMediaAttempterSink::OnClusterTransDataEvent(COMMAND command,char *pData,WORD wDataSize)
{
	switch(command.dwCmd)
	{ 
		//需要下载音频数据
	case USER_PUBLISH_AUDIO_REQ:
		OnClusterPublishAudio(command,pData,wDataSize);
		//需要下载视频数据
	case USER_PUBLISH_VIDEO_REQ:
		OnClusterPublishVideo(command,pData,wDataSize);
	case USER_PLAY_AUDIO_REQ:
		//如果数据源是在本机，且是初始数据源，启动客户端连接，建立资源对接
		OnClusterPlayAudio(command,pData,wDataSize);
	case USER_PLAY_VIDEO_REQ:
		OnClusterPlayVideo(command,pData,wDataSize);
		//音频资源停止发布
	case USER_STOP_PUBLISH_AUDIO_REQ:
		OnClusterStopPublishAudio(command,pData,wDataSize);
		//视频资源停止发布
	case USER_STOP_PUBLISH_VIDEO_REQ:
		OnClusterStopPublishVideo(command,pData,wDataSize);
	}
}
//////////////////////////////////集群数据处理////////////////////////////////////////////
//集群转发过来的流的数据源所在的服务器位置,如果当前状态是没有推送，则请求推送
bool CMediaAttempterSink::OnClusterPlayAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("集群转发消息：播放音频流请求信息数据非法(size >= 8),size %u",wDataSize);
		return true;
	}
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);

	//请求播放的服务器
	UNDWORD udwIpPort = 0;
	memcpy(&udwIpPort,pBuffer + 8,8);
	udwIpPort = ntohl64(udwIpPort);

	//是否是本地的视频数据
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL || pStreamItem->GetAudioResourceType() != ST_USER_CLIENT)
	{
		OUT_DEBUGEX("集群转发消息：服务器 %llu 要求播放音频流 %llu,本服务器不是原始数据源，不转发",udwIpPort,udwStreamID);	
		return true;
	} 
	OUT_INFOEX("集群转发消息：服务器 %llu 要求播放音频流 %llu",udwIpPort,udwStreamID);	

	//推送
	if(AddPublishAudio(udwIpPort) == true)
	{
		//获取空闲推送信息
		PublishInfo *pPublishInfo = GetFreePublishInfo();
		pPublishInfo->mediaType = EM_MEDIA_TYPE_AUDIO;
		pPublishInfo->udwStreamID = udwStreamID;
		pPublishInfo->udwSvrID = udwIpPort;
		IPPORTID ipPort;
		ipPort.udwIpPortID = udwIpPort;
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,ipPort.dwIp,ipPort.wPort + 10000) == false)
		{
			OUT_WARNEX("集群转发：服务器 %llu 要求播放音频流 %llu,创建客户端连接失败",udwIpPort,udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishAudio(udwIpPort);
		}
	}
	else
	{
		OUT_WARNEX("集群转发：服务器 %llu 要求播放音频流 %llu,已经推送",udwIpPort,udwStreamID);
	}
	
	return true;
}
//集群转发过来的流的数据源所在的服务器位置,如果当前状态是没有推送，则请求推送
bool CMediaAttempterSink::OnClusterPlayVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("集群转发消息：播放视频流请求信息数据非法(size >= 8),size %u",wDataSize);
		return true;
	}

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);

	//要求推送的服务器
	UNDWORD udwIpPort = 0;
	memcpy(&udwIpPort,pBuffer + 8,8);
	udwIpPort = ntohl64(udwIpPort);

	//是否是本地的视频数据
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL || pStreamItem->GetVideoResourceType() != ST_USER_CLIENT)
	{
		OUT_DEBUGEX("集群转发消息：服务器 %llu 要求播放视频流 %llu,本服务器不是原始数据源，不转发",udwIpPort,udwStreamID);	
		return true;
	} 
	OUT_INFOEX("集群转发消息：服务器 %llu 要求播放视频流 %llu",udwIpPort,udwStreamID);	
	//推送
	if(AddPublishVideo(udwIpPort) == true)
	{
		//获取空闲推送信息
		PublishInfo *pPublishInfo = GetFreePublishInfo();
		pPublishInfo->mediaType = EM_MEDIA_TYPE_VIDEO;
		pPublishInfo->udwStreamID = udwStreamID;
		pPublishInfo->udwSvrID = udwIpPort;
		IPPORTID ipPort;
		ipPort.udwIpPortID = udwIpPort;
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,ipPort.dwIp,ipPort.wPort + 10000) == false)
		{
			OUT_WARNEX("集群转发：服务器 %llu 要求播放视频流 %llu,创建客户端连接失败",udwIpPort,udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishVideo(udwIpPort);
		}
	}
	else 
	{
		OUT_WARNEX("集群转发：服务器 %llu 要求播放视频流 %llu,已经推送",udwIpPort,udwStreamID);
	}
	
	return true;
}
//集群发过来的发布视频,如果本机请求该音频的用户数大于0要求推送数据
bool CMediaAttempterSink::OnClusterPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("集群转发消息：发布音频流请求信息数据非法(size >= 8),size %u",wDataSize);
		return true;
	}
	//数据内容：4字节房间ID，4字节发布人ID，8字节服务器ID
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);
	 
	//上传数据的服务器
	UNDWORD udwSvrID = 0;
	memcpy(&udwSvrID,pBuffer + 8,8);
	udwSvrID = ntohl64(udwSvrID);

	//该为本服务器需要下载视频
	UNDWORD udwDstSvr = htonl64(m_ipPortID.udwIpPortID);
	memcpy(pBuffer + 8,&udwDstSvr,8);
	OUT_INFOEX("集群转发消息：用户 %u 发布音频流 %llu",dwUserID,udwStreamID);	

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	//服务器先收到的请求播放的PLAY
	if(pStreamItem != NULL && (pStreamItem->IsPublishAudio() == false || pStreamItem->GetCurrentAudioTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL) && pStreamItem->GetStreamAudioUserCount() > 0)
	{
		//向源服务器要数据
		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_AUDIO_REQ;
		playCmd.dwSequence = 0;
	
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransDstSvr(udwSvrID,playCmd,pBuffer,wDataSize);

		OUT_DEBUGEX("集群转发消息：服务器 %llu 通过集群向源服务器 %llu 下载音频流 %llu 数据",m_ipPortID.udwIpPortID,udwSvrID,udwStreamID);
	}
	return true;
}
//集群发过来的发布音频,如果本机请求视频的用户数大于0要求推送视频数据
bool CMediaAttempterSink::OnClusterPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("集群转发消息：发布视频流请求信息数据非法(size >= 8),size %u",wDataSize);
		return true;
	}
	//数据内容：4字节房间ID，4字节发布人ID，8字节服务器ID
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);
	 
	UNDWORD udwSvrID = 0;
	memcpy(&udwSvrID,pBuffer + 8,8);
	udwSvrID = ntohl64(udwSvrID);

	UNDWORD udwDstSvr = htonl64(m_ipPortID.udwIpPortID);
	memcpy(pBuffer + 8,&udwDstSvr,8);
	OUT_INFOEX("集群转发消息：用户 %u 发布视频流 %llu",dwUserID,udwStreamID);	

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	//服务器先收到的请求播放的PLAY
	if(pStreamItem != NULL && (pStreamItem->IsPublishVideo() == false || pStreamItem->GetCurrentVideoTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL) && pStreamItem->GetStreamVideoUserCount() > 0)
	{
		//向源服务器要数据
		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_VIDEO_REQ;
		playCmd.dwSequence = 0;
	
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransDstSvr(udwSvrID,playCmd,pBuffer,wDataSize);

		OUT_INFOEX("集群转发消息：服务器 %llu 通过集群向源服务器 %llu 下载视频流 %llu 数据",m_ipPortID.udwIpPortID,udwSvrID,udwStreamID);
	}
	return true;
}
//音频资源停止发布当老师停止上传或者同意个流资源有新的用户发布的时候都会有该消息
bool CMediaAttempterSink::OnClusterStopPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(wDataSize >= 16);
	if(wDataSize < 16) return true;

	UNDWORD udwSrcSvr = 0,udwStreamID = 0;
	memcpy(&udwSrcSvr,pBuffer,8);
	memcpy(&udwStreamID,pBuffer + 8,8);

	udwSrcSvr = ntohl64(udwSrcSvr);
	udwStreamID = ntohl64(udwStreamID);

	OUT_WARNEX("音频流:%llu 在服务器0x%llx上发布，本机0x%llx停止推送数据流",udwStreamID,udwSrcSvr,m_ipPortID.udwIpPortID);
	CStreamItem *pStream = m_streamManager.GetStreamItem(udwStreamID);
	if(pStream == NULL) return true;

	//停止音频推送的流
	pStream->SetStopPublishAudio();
	CPublishClientMap pcmap;
	pStream->GetPublishAudioClientList(&pcmap);
	pStream->ClearAudioPublishClient();
	DWORD dwIndexID = INVALID_DWORD,dwRoundID = INVALID_DWORD;
	for(CPublishClientMapIt it = pcmap.begin();it != pcmap.end();it++)
	{
		dwIndexID = (*it).first;
		dwRoundID = (*it).second;
		m_pITcpAttemperEngine->CloseClientConnection(dwRoundID,dwIndexID,false);
	}

	int nUserCount = pStream->GetStreamAudioUserCount() + pStream->GetStreamVideoUserCount();
	//如果房间音频没有发布，视频也没有发布，房间的用户数为0，清理掉房间
	if(pStream->IsPublishAudio() == false && pStream->IsPublishVideo() == false && nUserCount == 0)
	{  
		m_streamManager.RemoveStream(udwStreamID);
		OUT_WARNEX("集群通知音频流停止了发布，清理流数据，流ID：%llu ,视频用户数:%d,音频用户:%d,推送音频客户端数:%d,推送视频客户端数:%d",
			udwStreamID,pStream->GetStreamVideoUserCount(),pStream->GetStreamAudioUserCount(),
			pStream->GetPublishAudioClientCount(),pStream->GetPublishVideoClientCount());
	}
	
	return true;
}
//视频资源停止发布当老师停止上传或者同意个流资源有新的用户发布的时候都会有该消息
bool CMediaAttempterSink::OnClusterStopPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(wDataSize >= 16);
	if(wDataSize < 16) return true;

	UNDWORD udwSrcSvr = 0,udwStreamID = 0;
	memcpy(&udwSrcSvr,pBuffer,8);
	memcpy(&udwStreamID,pBuffer + 8,8);

	udwSrcSvr = ntohl64(udwSrcSvr);
	udwStreamID = ntohl64(udwStreamID);

	OUT_WARNEX("视频流:%llu 在服务器0x%llx上发布，本机0x%llx停止推送数据流",udwStreamID,udwSrcSvr,m_ipPortID.udwIpPortID);
	CStreamItem *pStream = m_streamManager.GetStreamItem(udwStreamID);
	if(pStream == NULL) return true;

	//停止视频推送的流
	pStream->SetStopPublishVideo();
	CPublishClientMap pcmap;
	pStream->GetPublishVideoClientList(&pcmap);
	pStream->ClearVideoPublishClient();
	DWORD dwIndexID = INVALID_DWORD,dwRoundID = INVALID_DWORD;
	for(CPublishClientMapIt it = pcmap.begin();it != pcmap.end();it++)
	{
		dwIndexID = (*it).first;
		dwRoundID = (*it).second;
		m_pITcpAttemperEngine->CloseClientConnection(dwRoundID,dwIndexID,false);
	}

	int nUserCount = pStream->GetStreamVideoUserCount() + pStream->GetStreamVideoUserCount();
	//如果房间音频没有发布，视频也没有发布，房间的用户数为0，清理掉房间
	if(pStream->IsPublishAudio() == false && pStream->IsPublishVideo() == false && nUserCount == 0)
	{  
		m_streamManager.RemoveStream(udwStreamID);
		OUT_WARNEX("集群通知音频流停止了发布，清理流数据，流ID：%llu ,视频用户数:%d,音频用户:%d,推送音频客户端数:%d,推送视频客户端数:%d",
			udwStreamID,pStream->GetStreamVideoUserCount(),pStream->GetStreamAudioUserCount(),
			pStream->GetPublishAudioClientCount(),pStream->GetPublishVideoClientCount());
	}
	return true;
}
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//网络连接消息
bool CMediaAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//超过最大连接数
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("媒体服务器服务端超过最大连接数%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u,断开该客户端连接",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("媒体服务器服务端:客户端连接IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,端口:%u",
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
bool CMediaAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//服务器登录
	case SYS_LOGIN_REQ:
		return OnReqSvrLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//用户登录
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//发布流
	case USER_PUBLISH_VIDEO_REQ:
		return OnReqPublishVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//发布流
	case USER_PUBLISH_AUDIO_REQ:
		return OnReqPublishAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//播放流
	case USER_PLAY_VIDEO_REQ:
		return OnReqPlayVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//播放流
	case USER_PLAY_AUDIO_REQ:
		return OnReqPlayAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//音频头
	case USER_AUDIO_HEADER_REQ:
		return OnReqAudioHeaderData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);;
		//视频头
	case USER_VIDEO_HEADER_REQ:
		return OnReqVideoHeaderData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize); 
		//音频数据
	case USER_AUDIO_DATA_REQ:
		return OnReqAudioData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);;
		//视频数据
	case USER_VIDEO_DATA_REQ:
		return OnReqVideoData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize); 
		//心跳
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//网络关闭消息
bool CMediaAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("媒体服务端：客户端连接断开id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	bool bIsClearStream = true;
	//清理流数据,如果是老师发布，只需要修改状态为没有发布，如果是学生，删除房间中的学生，在看用户数是否为0，为0清理房间
	if(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID != (UNDWORD)-1)
	{
		CStreamItem *pStreamItem = m_streamManager.GetStreamItem(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
		if(pStreamItem != NULL)
		{
			//当前是上传数据
			int nUserCount = 0;
			if(m_pConnectInfo[pCloseEvent->dwIndexID].bIsPublish == true)
			{ 

				//上传的是音频,设置音频状态为没有上传
				if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
				{
					//如果当前断开的连接和正在发布的连接一致，才清理数据
					if(pStreamItem->GetPublishAudioIndexID() == pCloseEvent->dwIndexID && pStreamItem->GetPublishAudioRoundID() == pCloseEvent->dwRoundID)
					{
						//推送到所有服务器
						if(m_pConnectInfo[pCloseEvent->dwIndexID].wClientType == ST_USER_CENTER) 
						{
							//通过集群通知原推送服务器
							COMMAND cmd; 
							cmd.dwCmd = USER_STOP_PUBLISH_AUDIO_REQ; 
							char cbBuffer[24];
							UNDWORD udwLocl = htonl64(m_ipPortID.udwIpPortID);
							memcpy(cbBuffer,&udwLocl,8);
							UNDWORD udwStreamTmp = htonl64(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
							memcpy(cbBuffer + 8,&udwStreamTmp,8);
							m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,cmd,cbBuffer,16);
						}

						pStreamItem->SetStopPublishAudio();
						//清理推送连接
						CPublishClientMap pcmap;
						pStreamItem->GetPublishAudioClientList(&pcmap);
						pStreamItem->ClearAudioPublishClient();
						DWORD dwIndexID = INVALID_DWORD,dwRoundID = INVALID_DWORD;
						for(CPublishClientMapIt it = pcmap.begin();it != pcmap.end();it++)
						{
							dwIndexID = (*it).first;
							dwRoundID = (*it).second;
							m_pITcpAttemperEngine->CloseClientConnection(dwRoundID,dwIndexID,false);
						}
						OUT_INFOEX("老师%llu停止了上传音频数据，流ID：%llu ,音频用户数:%d,推送音频客户端数:%d",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamAudioUserCount(),
							pStreamItem->GetPublishAudioClientCount());
					}
					else
					{
						OUT_WARNEX("老师%llu停止了上传音频数据，但是当前断开连接索引和正在上传的索引不一致，所以不清理数据。流ID：%llu ,音频用户数:%d,推送音频客户端数:%d cindex:%u cround:%u pindex:%u pround:%u",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamAudioUserCount(),
							pStreamItem->GetPublishAudioClientCount(),pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pStreamItem->GetPublishAudioIndexID(),pStreamItem->GetPublishAudioRoundID());
						bIsClearStream = false;
					}
				}
				else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //视频
				{
					if(pStreamItem->GetPublishVideoIndexID() == pCloseEvent->dwIndexID && pStreamItem->GetPublishVideoRoundID() == pCloseEvent->dwRoundID)
					{
						//推送到所有服务器
						if(m_pConnectInfo[pCloseEvent->dwIndexID].wClientType == ST_USER_CENTER) 
						{
							//通过集群通知原推送服务器
							COMMAND cmd;
							cmd.dwCmd = USER_STOP_PUBLISH_VIDEO_REQ;
							char cbBuffer[24];
							UNDWORD udwLocl = htonl64(m_ipPortID.udwIpPortID);
							memcpy(cbBuffer,&udwLocl,8);
							UNDWORD udwStreamTmp = htonl64(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
							memcpy(cbBuffer + 8,&udwStreamTmp,8);
							m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,cmd,cbBuffer,16);
						}
						pStreamItem->SetStopPublishVideo();
						//清理推送连接
						CPublishClientMap pcmap;
						pStreamItem->GetPublishVideoClientList(&pcmap);
						pStreamItem->ClearVideoPublishClient();
						DWORD dwIndexID = INVALID_DWORD,dwRoundID = INVALID_DWORD;
						for(CPublishClientMapIt it = pcmap.begin();it != pcmap.end();it++)
						{
							dwIndexID = (*it).first;
							dwRoundID = (*it).second;
							m_pITcpAttemperEngine->CloseClientConnection(dwRoundID,dwIndexID,false);
						}
						OUT_INFOEX("老师%llu停止了上传视频数据，流ID：%llu ,视频用户数:%d,推送视频客户端数:%d",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),
							pStreamItem->GetPublishVideoClientCount());
					}
					else
					{
						OUT_WARNEX("老师%llu停止了上传视频数据，但是当前断开连接索引和正在上传的索引不一致，所以不清理数据。流ID：%llu ,视频用户数:%d,推送视频客户端数:%d cindex:%u cround:%u pindex:%u pround:%u",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),
							pStreamItem->GetPublishVideoClientCount(),pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pStreamItem->GetPublishVideoIndexID(),pStreamItem->GetPublishVideoRoundID());
						bIsClearStream = false;
					}
				}
				nUserCount = pStreamItem->GetPublishAudioClientCount();//房间还剩多少人
				nUserCount += pStreamItem->GetPublishVideoClientCount();//房间还剩多少人
			}
			else 
			{
				//请求播放的是音频还是视频
				if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
				{
					nUserCount = pStreamItem->RemoveAudioUser(pCloseEvent->dwIndexID);
					OUT_DEBUGEX("学生%llu停止了播放音频数据，流ID：%llu ,音频用户数:%d,推送音频客户端数:%d",
						m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetPublishAudioClientCount());
					nUserCount += pStreamItem->GetPublishVideoClientCount();//房间还剩多少人
				}
				else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //视频
				{
					nUserCount = pStreamItem->RemoveVideoUser(pCloseEvent->dwIndexID);
					OUT_DEBUGEX("学生%llu停止了播放视频数据，流ID：%llu ,视频用户数:%d,推送视频客户端数:%d",
						m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,
						pStreamItem->GetPublishVideoClientCount());
					nUserCount += pStreamItem->GetPublishAudioClientCount();//房间还剩多少人
				}
			}

			//如果房间音频没有发布，视频也没有发布，房间的用户数为0，清理掉房间
			if(bIsClearStream && pStreamItem->IsPublishAudio() == false && pStreamItem->IsPublishVideo() == false && nUserCount == 0)
			{  
				m_streamManager.RemoveStream(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
				OUT_INFOEX("清理流数据，流ID：%llu ,视频用户数:%d,音频用户:%d,推送音频客户端数:%d,推送视频客户端数:%d",
					pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount());
			}
			else
			{
				OUT_INFOEX("清理流数据(不清理)，流ID：%llu ,用户数：%u,视频用户数:%d,音频用户:%d,推送音频客户端数:%d,推送视频客户端数:%d,音频发布状态:%d 视频发布状态:%d",
					pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount(),pStreamItem->IsPublishAudio(),pStreamItem->IsPublishVideo());
			}
		}
	}

	//TCP上传走TCP,如果在登录情况下断线，则老师负载减1
	if(EM_MEDIA_TYPE_NO != m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType
			&& ST_USER_CLIENT == m_pConnectInfo[pCloseEvent->dwIndexID].wClientType
			&& 1 == m_pConnectInfo[pCloseEvent->dwIndexID].wStatus)
	{
		if(m_dwTeacherNum)	{
			--m_dwTeacherNum;
			SendGatewayServerLoad();
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
bool CMediaAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,NULL,0);
}
//服务器登录
bool CMediaAttempterSink::OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	OUT_INFOEX("服务器登陆:ID %llu",udwID); 
	m_pConnectInfo[dwIndexID].udwID = udwID;
	m_pConnectInfo[dwIndexID].wClientType = ST_DATA_TRANS_SVR;
	m_pConnectInfo[dwIndexID].wStatus = 1;

	int nRet = 0;
	command.dwCmd = SYS_LOGIN_RESP;
	ASSERT(m_pITcpAttemperEngine != NULL);
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//客户端登陆
bool CMediaAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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
	 
	++m_dwTeacherNum;
	SendGatewayServerLoad();

	int nRet = 0;
	command.dwCmd = USER_LOGIN_SVR_RESP;
	ASSERT(m_pITcpAttemperEngine != NULL);
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4); 
}
//用户发布视频数据
bool CMediaAttempterSink::OnReqPublishVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;

	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//如果是用户上传数据，告诉其他所有服务器，先清理该流的资源
	if(m_pConnectInfo[dwIndexID].wClientType == ST_USER_CLIENT)
	{
		COMMAND cmd;
		cmd.dwCmd = USER_STOP_PUBLISH_VIDEO_REQ;
		char cbBuffer[24];
		UNDWORD udwLocl = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer,&udwLocl,8);
		UNDWORD udwStreamTmp = htonl64(udwStreamID);
		memcpy(cbBuffer + 8,&udwStreamTmp,8);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,cmd,cbBuffer,16);
	}

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
	//判断当前视频发布的状态为true，但是最后一次的视频数据时间已经超过了2秒
	DWORD dwTimeInterval = 0;
	if(pStreamItem->IsPublishVideo() == true && (dwTimeInterval = pStreamItem->GetCurrentVideoTimeInterval()) > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		//判断是否是本地视频数据服
		DWORD dwPublishIndex = pStreamItem->GetPublishVideoIndexID();
		//是用户发布，不是推送的服务器连接
		if(m_pConnectInfo[dwPublishIndex].wClientType == ST_USER_CLIENT)
		{
			//使用户发布，但是不是同一个连接
			if(dwIndexID == dwPublishIndex && dwRoundID == pStreamItem->GetPublishVideoRoundID())
			{
				OUT_WARNEX("普通用户%llu 重复发布同一视频资源流:%llu，上一视频数据连接最后视频数据为%u秒前,连接i:%u r:%u",m_pConnectInfo[dwIndexID].udwID,
					udwStreamID,dwTimeInterval,dwIndexID,dwRoundID);
			}
			else if(m_pConnectInfo[dwPublishIndex].dwRoundID != pStreamItem->GetPublishVideoRoundID())
			{
				OUT_WARNEX("严重警告流:%llu，SOCKET连接已经被复用，但是视频发布资源的连接还是复用前的连接，资源没有及时清理,ni:%u nr:%u oi:%u or:%u",
					udwStreamID,dwIndexID,dwRoundID,dwPublishIndex,pStreamItem->GetPublishVideoRoundID());
			}
		}
		pStreamItem->SetStopPublishVideo();
	}


	ASSERT(pStreamItem != NULL);
	if(pStreamItem->SetPublishVideoInfo(m_pConnectInfo[dwIndexID].wClientType,dwIndexID,dwRoundID,udwStreamID) == false)
	{
		OUT_WARNEX("用户%llu:%u 类型%u 发布%llu视频数据失败，该流资源已经有用户发布",m_pConnectInfo[dwIndexID].udwID,dwUserID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
		int nRet = htonl((int)1);
		command.dwCmd = USER_PUBLISH_VIDEO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	//用户类型是发布
	m_pConnectInfo[dwIndexID].bIsPublish = true;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_VIDEO;	//发布类型是视频
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 
	//通知其他服务器,客户端登陆，说明是源数据
	if(m_pConnectInfo[dwIndexID].wClientType == ST_USER_CLIENT)
	{
		char cbBuffer[24];
		memcpy(cbBuffer,pData,8);
		UNDWORD udwSvrID = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer + 8,&udwSvrID,8);

		COMMAND publishCmd;
		publishCmd.dwCmd = USER_PUBLISH_VIDEO_REQ;
		publishCmd.dwSequence = 0;
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,publishCmd,cbBuffer,16);
		OUT_INFOEX("用户%llu 类型%u 发布%llu视频数据,通知其他服务器",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
	}
	else 
		OUT_INFOEX("用户%llu 类型%u 发布%llu视频数据",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

	int nRet = 0;
	command.dwCmd = USER_PUBLISH_VIDEO_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//用户发布音频数据
bool CMediaAttempterSink::OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;

	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//如果是用户上传数据，告诉其他所有服务器，先清理该流的资源
	if(m_pConnectInfo[dwIndexID].wClientType == ST_USER_CLIENT)
	{
		COMMAND cmd;
		cmd.dwCmd = USER_STOP_PUBLISH_AUDIO_REQ;
		char cbBuffer[24];
		UNDWORD udwLocl = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer,&udwLocl,8);
		UNDWORD udwStreamTmp = htonl64(udwStreamID);
		memcpy(cbBuffer + 8,&udwStreamTmp,8);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,cmd,cbBuffer,16);
	}

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
	ASSERT(pStreamItem != NULL);
	//判断当前视频发布的状态为true，但是最后一次的视频数据时间已经超过了2秒
	DWORD dwTimeInterval = 0;
	if(pStreamItem->IsPublishAudio() == true && (dwTimeInterval = pStreamItem->GetCurrentAudioTimeInterval()) > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		//判断是否是本地音频数据服
		DWORD dwPublishIndex = pStreamItem->GetPublishAudioIndexID();
		//是用户发布，不是推送的服务器连接
		if(m_pConnectInfo[dwPublishIndex].wClientType == ST_USER_CLIENT)
		{
			//使用户发布，但是不是同一个连接
			if(dwIndexID == dwPublishIndex && dwRoundID == pStreamItem->GetPublishAudioRoundID())
			{
				OUT_WARNEX("普通用户%llu 重复发布同一音频资源流:%llu，上一视频数据连接最后视频数据为%u秒前,连接i:%u r:%u",m_pConnectInfo[dwIndexID].udwID,
					udwStreamID,dwTimeInterval,dwIndexID,dwRoundID);
			}
			else if(m_pConnectInfo[dwPublishIndex].dwRoundID != pStreamItem->GetPublishAudioRoundID())
			{
				OUT_WARNEX("严重警告流:%llu，SOCKET连接已经被复用，但是音频发布资源的连接还是复用前的连接，资源没有及时清理,ni:%u nr:%u oi:%u or:%u",
					udwStreamID,dwIndexID,dwRoundID,dwPublishIndex,pStreamItem->GetPublishAudioRoundID());
			}
		}
		pStreamItem->SetStopPublishAudio();
	}


	if(pStreamItem->SetPublishAudioInfo(m_pConnectInfo[dwIndexID].wClientType,dwIndexID,dwRoundID,udwStreamID) == false)
	{
		OUT_WARNEX("用户%llu:%u 类型%u 发布%llu音频数据失败，该流资源已经有用户发布",m_pConnectInfo[dwIndexID].udwID,dwUserID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
		int nRet = htonl((int)1);
		command.dwCmd = USER_PUBLISH_AUDIO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	//用户类型是发布
	m_pConnectInfo[dwIndexID].bIsPublish = true;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_AUDIO;	//发布类型是音频
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 
	//通知其他服务器,客户端登陆，说明是源数据
	if(m_pConnectInfo[dwIndexID].wClientType == ST_USER_CLIENT)
	{
		char cbBuffer[24];
		memcpy(cbBuffer,pData,8);
		UNDWORD udwSvrID = htonl64(m_ipPortID.udwIpPortID);
		memcpy(cbBuffer + 8,&udwSvrID,8);

		COMMAND publishCmd;
		publishCmd.dwCmd = USER_PUBLISH_AUDIO_REQ;
		publishCmd.dwSequence = 0;
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransExceptIDSvr(m_ipPortID.udwIpPortID,publishCmd,cbBuffer,16);
		OUT_INFOEX("用户%llu 类型%u 发布%llu音频数据,通知其他服务器",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
	}
	else 
		OUT_INFOEX("用户%llu 类型%u 发布%llu音频数据",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

	int nRet = 0;
	command.dwCmd = USER_PUBLISH_AUDIO_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//用户下载视频数据
bool CMediaAttempterSink::OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
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
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_VIDEO_RESP;
	m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu视频数据,当前请求用户数:%u,本机数据源状态:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamVideoUserCount(),pStreamItem->IsPublishVideo());
	
	//如果有音视频头数据，发送音视频头
	COMMAND headerCmd;
	char cbAvHeader[256];
	WORD wSize = 256;
	if(pStreamItem->GetVideoHeaderData(cbAvHeader,wSize) == true)
	{
		headerCmd.dwCmd = USER_VIDEO_HEADER_REQ;
		m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,headerCmd,cbAvHeader,wSize);
	}
	return true;
}
//用户下载音频数据
bool CMediaAttempterSink::OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
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
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_AUDIO_RESP;
	m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("用户%llu 类型%u 请求播放%llu音频数据,当前请求音频用户:%u,本机数据源状态:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamAudioUserCount(),pStreamItem->IsPublishAudio());
	
	//如果有音视频头数据，发送音视频头
	COMMAND headerCmd;
	char cbAvHeader[256];
	WORD wSize = 256;
	if(pStreamItem->GetAudioHeaderData(cbAvHeader,wSize) == true)
	{
		headerCmd.dwCmd = USER_AUDIO_HEADER_REQ;
		m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,headerCmd,cbAvHeader,wSize);
	}
	return true;
}
//视频头数据
bool CMediaAttempterSink::OnReqVideoHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(CheckUploadData(dwIndexID,command.dwCmd,pStreamItem) == false)
		return false;
  
	//保存视频头数据
	pStreamItem->SetVideoHeaderData(pData,wDataSize);
	return TransData(pStreamItem,command,pData,wDataSize);
}
//音频头数据
bool CMediaAttempterSink::OnReqAudioHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;
	
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(CheckUploadData(dwIndexID,command.dwCmd,pStreamItem) == false)
		return false;
	OUT_INFOEX("收到流 %llu 音频头数据,大小:%u",udwStreamID,wDataSize);
	//保存视频头数据
	pStreamItem->SetAudioHeaderData(pData,wDataSize);
	return TransData(pStreamItem,command,pData,wDataSize);
}
//视频数据
bool CMediaAttempterSink::OnReqVideoData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	//printf(__FILE__" : %d : %s  111\r\n",__LINE__,__FUNCTION__);
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	//printf(__FILE__" : %d : %s  222\r\n",__LINE__,__FUNCTION__);
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;
	//printf(__FILE__" : %d : %s  333\r\n",__LINE__,__FUNCTION__);
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(CheckUploadData(dwIndexID,command.dwCmd,pStreamItem) == false)
		return false;
	//printf(__FILE__" : %d : %s  444\r\n",__LINE__,__FUNCTION__);
	//printf("-------dwIndexID:%u-----recv video data,stream id:%llu ,data size:%u\r\n",dwIndexID,udwStreamID,wDataSize);
	return TransData(pStreamItem,command,pData,wDataSize);
}
//音频数据
bool CMediaAttempterSink::OnReqAudioData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;
	
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(CheckUploadData(dwIndexID,command.dwCmd,pStreamItem) == false)
		return false;
  
	return TransData(pStreamItem,command,pData,wDataSize);
}

//验证登录
inline bool CMediaAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("连接index:%u roundID:%u 操作命令字0x%x，请求非法，连接未登录",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//验证数据
inline bool CMediaAttempterSink::CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= n);
	if(pData == NULL || wDataSize < n)
	{
		OUT_WARNEX("连接index:%u roundID:%u 登录类型:%u 命令字：0x%x数据非法 %u >= %u",dwIndexID,dwRoundID,m_pConnectInfo[dwIndexID].wClientType,dwCmd,wDataSize,n);
		return false;
	}
	return true;
}
//验证上传权限
inline bool CMediaAttempterSink::CheckUploadData(DWORD dwIndexID,DWORD dwCmd,CStreamItem *pStreamItem)
{
	if((dwCmd == USER_PUBLISH_VIDEO_REQ || dwCmd == USER_VIDEO_HEADER_REQ || dwCmd == USER_VIDEO_DATA_REQ) && 
		(m_pConnectInfo[dwIndexID].bIsPublish == false || m_pConnectInfo[dwIndexID].wMediaType != EM_MEDIA_TYPE_VIDEO) && 
		(pStreamItem == NULL || pStreamItem->IsPublishVideo() == false || m_pConnectInfo[dwIndexID].udwPublishStreamID != pStreamItem->GetStreamID() ||
		(m_pConnectInfo[dwIndexID].dwRoundID != pStreamItem->GetPublishVideoRoundID() && dwIndexID != pStreamItem->GetPublishVideoIndexID()))
		)
	{
		//bool bIsPublish = (pStreamItem == NULL)?false:pStreamItem->IsPublishVideo();
		//UNDWORD udwStream = (pStreamItem == NULL)?0:pStreamItem->GetStreamID();
		//DWORD dwPublishIndex = (pStreamItem == NULL)?INVALID_DWORD:pStreamItem->GetPublishVideoIndexID();
		//DWORD dwPublishRound = (pStreamItem == NULL)?INVALID_DWORD:pStreamItem->GetPublishVideoRoundID();
		//printf("0x%x,isPublish:%u,PublishStreamID:%llu != streamID:%llu,indexID:0x%x != publishIndex:0x%x,PublisRound:0x%x != roundID:0x%x",
		//	pStreamItem,bIsPublish,m_pConnectInfo[dwIndexID].udwPublishStreamID,udwStream,
		//	dwIndexID,dwPublishIndex,dwPublishRound,m_pConnectInfo[dwIndexID].dwRoundID);
		OUT_WARNEX("用户 %llu 操作 0x%x 非法，不具备上传视频数据的权限",m_pConnectInfo[dwIndexID].udwID,dwCmd);
		return false;
	} 
	if((dwCmd == USER_PUBLISH_AUDIO_REQ || dwCmd == USER_AUDIO_HEADER_REQ || dwCmd == USER_AUDIO_DATA_REQ) && 
		(m_pConnectInfo[dwIndexID].bIsPublish == false || m_pConnectInfo[dwIndexID].wMediaType != EM_MEDIA_TYPE_AUDIO) && 
		(pStreamItem == NULL || pStreamItem->IsPublishAudio() == false || m_pConnectInfo[dwIndexID].udwPublishStreamID != pStreamItem->GetStreamID() ||
		(m_pConnectInfo[dwIndexID].dwRoundID != pStreamItem->GetPublishAudioRoundID() && dwIndexID != pStreamItem->GetPublishAudioIndexID()))
		)
	{
		//bool bIsPublish = (pStreamItem == NULL)?false:pStreamItem->IsPublishAudio();
		//UNDWORD udwStream = (pStreamItem == NULL)?0:pStreamItem->GetStreamID();
		//DWORD dwPublishIndex = (pStreamItem == NULL)?INVALID_DWORD:pStreamItem->GetPublishAudioIndexID();
		//DWORD dwPublishRound = (pStreamItem == NULL)?INVALID_DWORD:pStreamItem->GetPublishAudioRoundID();
		//printf("0x%x,isPublish:%u,PublishStreamID:%llu != streamID:%llu,indexID:0x%x != publishIndex:0x%x,PublisRound:0x%x != roundID:0x%x",
		//	pStreamItem,bIsPublish,m_pConnectInfo[dwIndexID].udwPublishStreamID,udwStream,
		//	dwIndexID,dwPublishIndex,dwPublishRound,m_pConnectInfo[dwIndexID].dwRoundID);
		OUT_WARNEX("用户 %llu 操作 0x%x 非法，不具备上传音频数据的权限",m_pConnectInfo[dwIndexID].udwID,dwCmd);
		return false;
	} 
	return true;
}
//转发数据
inline bool CMediaAttempterSink::TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pStreamItem != NULL);
	if(pStreamItem == NULL) return true;

	//先添加用户数据任务
	//CPlayUserMap userMap;
	//转发数据
	//CPublishClientMap clientMap;
	if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
	{
		pStreamItem->SetVideoTime();
		//printf(__FILE__" : %d : v user:%d p client:%d\r\n",__LINE__,pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetPublishVideoClientCount());
		if(pStreamItem->GetStreamVideoUserCount() > 0 || pStreamItem->GetPublishVideoClientCount() > 0)
		{
			m_transAVDataEngine.PutData(command,pData,wDataSize,EM_CLIENT_TYPE_UDP);
		}

		//for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
		//{
		//	userMap.clear();
		//	pStreamItem->GetPlayVideoUserList(i,&userMap);
		//	if(userMap.size() > 0)
		//	{
		//		m_transAVDataEngine.PutData(command,pData,wDataSize,EM_CLIENT_TYPE_UDP,userMap);
		//	}
		//} 

		//printf(__FILE__" : %d : %s  444\r\n",__LINE__,__FUNCTION__);
		//pStreamItem->GetPublishVideoClientList(&clientMap);
		//printf(__FILE__" : %d : %s  555\r\n",__LINE__,__FUNCTION__);
	}
	else if(command.dwCmd == USER_AUDIO_DATA_REQ	 || command.dwCmd == USER_AUDIO_HEADER_REQ)
	{
		pStreamItem->SetAudioTime();
		//printf(__FILE__" : %d : a user:%d p client:%d\r\n",__LINE__,pStreamItem->GetStreamAudioUserCount(),pStreamItem->GetPublishAudioClientCount());
		if(pStreamItem->GetStreamAudioUserCount() > 0 || pStreamItem->GetPublishAudioClientCount() > 0)
		{
			m_transAVDataEngine.PutData(command,pData,wDataSize,EM_CLIENT_TYPE_UDP);
		}
		//for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
		//{
		//	userMap.clear();
		//	pStreamItem->GetPlayAudioUserList(i,&userMap);
		//	if(userMap.size() > 0)
		//	{
		//		m_transAVDataEngine.PutData(command,pData,wDataSize,EM_CLIENT_TYPE_UDP,userMap);
		//	}
		//} 

		//pStreamItem->GetPublishAudioClientList(&clientMap);

	}
	
	//CPublishStreamClientSink *pClient = NULL;
	//for(CPublishClientMapIt it = clientMap.begin();it != clientMap.end();it++)
	//{
	//	pClient = (*it).second;
	//	if(pClient == NULL) continue;
	//	if(command.dwCmd == USER_VIDEO_HEADER_REQ || command.dwCmd == USER_AUDIO_HEADER_REQ)
	//		pClient->SetMediaHeaderData(pData,wDataSize);
	//	pClient->SendData(command,pData,wDataSize);
	//}
	//if(command.dwCmd == USER_VIDEO_DATA_REQ || command.dwCmd == USER_VIDEO_HEADER_REQ)
	//	printf(__FILE__" : %d : %s  %u 666\r\n",__LINE__,__FUNCTION__,command.dwSequence);
	//m_udpAttempterSink.TransData(pStreamItem,command,pData,wDataSize);
	return true;
}
bool CMediaAttempterSink::OnClientConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	ASSERT(pAcceptEvent != NULL);
	PublishInfo *pPublishInfo = (PublishInfo*)(pAcceptEvent->wParam);
	ASSERT(pPublishInfo != NULL);
	//连接成功
	if(pAcceptEvent->nErrCode == 0)
	{
		pPublishInfo->wRepeatConnectionCounter = 0;
		pPublishInfo->nSleepTime = 1;

		//发送登录
		ReqPublishLogin(pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID);
		return true;
	}
	pPublishInfo->wRepeatConnectionCounter++;
	pPublishInfo->nSleepTime *= 2;
	pPublishInfo->nSleepTime = (pPublishInfo->nSleepTime > 60)?60:pPublishInfo->nSleepTime;
	OUT_WARNEX("发布流客户端连接 %u 次服务器失败，类型:%d,%u秒后重新连接，目标 %llu，流 %llu 描述：%s",pPublishInfo->wRepeatConnectionCounter,pPublishInfo->mediaType,
		pPublishInfo->nSleepTime,pPublishInfo->udwSvrID,pPublishInfo->udwStreamID,pAcceptEvent->szMsg);
	if(pPublishInfo->wRepeatConnectionCounter <= 30)
	{
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,pAcceptEvent->ulIpAddr,pAcceptEvent->usPort,pPublishInfo->nSleepTime) == false)
		{
			OUT_WARNEX("集群转发：服务器 %llu 要求播放音频流 %llu,创建客户端连接失败",pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishAudio(pPublishInfo->udwSvrID);
			return false;
		}
	}
	else
	{
		OUT_WARNEX("发布流客户端连接 %u 次服务器失败，类型:%d,%u秒后重新连接，目标 %llu，流 %llu 描述：%s  已经超过30次连接，不在执行连接",pPublishInfo->wRepeatConnectionCounter,pPublishInfo->mediaType,
			pPublishInfo->nSleepTime,pPublishInfo->udwSvrID,pPublishInfo->udwStreamID,pAcceptEvent->szMsg);
		FreePublishInfo(pPublishInfo);
		RemovePublishAudio(pPublishInfo->udwSvrID);
	}
	return true;
}
bool CMediaAttempterSink::OnClientRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		case SYS_LOGIN_RESP:
			return OnRespPublishLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize,pRecvEvent->wParam);
		case USER_PUBLISH_VIDEO_RESP:
		case USER_PUBLISH_AUDIO_RESP:
			return OnRespPublishPublish(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize,pRecvEvent->wParam);
	}
	return true;
}
bool CMediaAttempterSink::OnClientCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	ASSERT(pCloseEvent != NULL);
	PublishInfo *pPublishInfo = (PublishInfo*)(pCloseEvent->wParam);
	ASSERT(pPublishInfo != NULL);
	//删除音视频的连接
	if(pPublishInfo->mediaType == EM_MEDIA_TYPE_AUDIO)
	{
		CStreamItem *pStream = m_streamManager.GetStreamItem(pPublishInfo->udwStreamID);
		if(pStream != NULL) pStream->RemovePublishAudioClient(pCloseEvent->dwIndexID);
	}
	else if(pPublishInfo->mediaType == EM_MEDIA_TYPE_VIDEO)
	{
		CStreamItem *pStream = m_streamManager.GetStreamItem(pPublishInfo->udwStreamID);
		if(pStream != NULL) pStream->RemovePublishAudioClient(pCloseEvent->dwIndexID);
	}

	OUT_WARNEX("发布流客户端与服务器的连接断开，类型:%d,目标 %llu，流 %llu,即将重新连接",pPublishInfo->mediaType,pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);
	if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,pCloseEvent->dwIpAddr,pCloseEvent->usPort,pPublishInfo->nSleepTime) == false)
	{
		OUT_WARNEX("集群转发：服务器 %llu 要求播放音频流 %llu,创建客户端连接失败",pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);	
		CStreamItem *pStream = m_streamManager.GetStreamItem(pPublishInfo->udwStreamID);
		if(pStream != NULL)
		{
			if(pPublishInfo->mediaType == EM_MEDIA_TYPE_AUDIO)
				pStream->RemovePublishAudioClient(pCloseEvent->dwIndexID);
			else if(pPublishInfo->mediaType == EM_MEDIA_TYPE_VIDEO)
				pStream->RemovePublishVideoClient(pCloseEvent->dwIndexID);
		}
		FreePublishInfo(pPublishInfo);
		RemovePublishAudio(pPublishInfo->udwSvrID);
		return false;
	}
	return true;
}
//登录到服务器,ID UNDWORD ,TYPE WORD(类型为业务连接)
bool CMediaAttempterSink::ReqPublishLogin(DWORD dwIndexID,DWORD dwRoundID)
{
	UNDWORD udwSvrID = htonl64(m_ipPortID.udwIpPortID);
	char szBuffer[12];
	memcpy(szBuffer,&udwSvrID,8);
	WORD srcServerType = htonl(MEDIA_SERVER);
	WORD dstServerType = htonl(GATEWAY_SERVER);
	memcpy(szBuffer+8,&srcServerType,2);
	memcpy(szBuffer+10,&dstServerType,2);

	COMMAND cmd;
	cmd.dwCmd = SYS_LOGIN_REQ;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,cmd,(char*)szBuffer,12);
}
//登录响应
bool CMediaAttempterSink::OnRespPublishLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize == 4);
	if(wDataSize < 4) return false;
	ASSERT(*((int*)pBuffer) == 0);

	UNDWORD udwSvrID = 0;
	WORD srcServerType = NONE_SERVER;
	WORD dstServerType = NONE_SERVER;

	udwSvrID = ntohl64(*(UNDWORD*)(pBuffer));
	srcServerType = ntohl(*(WORD*)(pBuffer+8));
	dstServerType = ntohl(*(WORD*)(pBuffer+10));

	if (MEDIA_SERVER == srcServerType && GATEWAY_SERVER == dstServerType)
	{
		m_dwGatewayIndex = dwIndexID;
		m_dwGatewayRound = dwRoundID;
	
		OUT_INFOEX("收到网关服务器(ID:%llu,Type:%u)登录成功回复消息",udwSvrID,dstServerType);
		return SendGatewayServerLoad();
	}
	
	return ReqPublishPublish(dwIndexID,dwRoundID,wParam);
}
//发送publish
bool CMediaAttempterSink::ReqPublishPublish(DWORD dwIndexID,DWORD dwRoundID,WPARAM wParam)
{
	PublishInfo *pPublishInfo = (PublishInfo*)wParam;
	ASSERT(pPublishInfo != NULL);

	char cbBuffer[16];
	DWORD dwUserID = pPublishInfo->udwStreamID & 0xFFFFFFFF;
	DWORD dwRoomID = pPublishInfo->udwStreamID >> 32;
	dwUserID = htonl(dwUserID);
	dwRoomID = htonl(dwRoomID);

	memcpy(cbBuffer,&dwRoomID,4);
	memcpy(cbBuffer + 4,&dwUserID,4);
	COMMAND cmd;
	cmd.dwCmd = (pPublishInfo->mediaType == EM_MEDIA_TYPE_AUDIO)?USER_PUBLISH_AUDIO_REQ:USER_PUBLISH_VIDEO_REQ;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,cmd,cbBuffer,16);
}
//publish响应
bool CMediaAttempterSink::OnRespPublishPublish(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize == 4);
	if(wDataSize < 4) return false;
	ASSERT(*((int*)pBuffer) == 0);

	PublishInfo *pPublishInfo = (PublishInfo*)wParam;
	ASSERT(pPublishInfo != NULL);

	OUT_INFOEX("收到服务器 %llu 的发布流RESPONSE,类型:%d,流ID %llu",m_ipPortID.udwIpPortID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
	DWORD dwUserID = pPublishInfo->udwStreamID & 0xFFFFFFFF;
	DWORD dwRoomID = pPublishInfo->udwStreamID >> 32;
	dwUserID = htonl(dwUserID);
	dwRoomID = htonl(dwRoomID);

	//memcpy(cbBuffer,&dwRoomID,4);
	//memcpy(cbBuffer + 4,&dwUserID,4);

	CStreamItem *pStream = m_streamManager.GetStreamItem(pPublishInfo->udwStreamID);
	ASSERT(pStream != NULL);

	COMMAND cmd;
	char cbBuffer[128];
	WORD wSize = 128;
	if(pPublishInfo->mediaType == EM_MEDIA_TYPE_AUDIO && pStream->GetAudioHeaderData(cbBuffer,wSize) == true)
	{
		cmd.dwCmd = USER_AUDIO_HEADER_REQ;
		m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,cmd,cbBuffer,wSize);
		pStream->AddPublisAudioClient(dwIndexID,dwRoundID);
		OUT_INFOEX("发送缓存的音频头数据到服服务器 %llu 类型:%d,流ID %llu",pPublishInfo->udwSvrID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
	}
	else if(pPublishInfo->mediaType == EM_MEDIA_TYPE_VIDEO && pStream->GetVideoHeaderData(cbBuffer,wSize) == true)
	{
		cmd.dwCmd = USER_VIDEO_HEADER_REQ;
		m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,cmd,cbBuffer,wSize);
		pStream->AddPublisVideoClient(dwIndexID,dwRoundID);
		OUT_INFOEX("发送缓存的视频头数据到服服务器 %llu 类型:%d,流ID %llu",pPublishInfo->udwSvrID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
	}
	return true;
}
//上报总负载
bool CMediaAttempterSink::SendGatewayServerLoad()
{
	if(0 == m_dwGatewayIndex) return false;

	DWORD dwLoad = this->GetServerLoadNum();
	char szBuffer[10];
	DWORD dwIP = htonl(inet_addr("0.0.0.0"));
	WORD wdPort = htons(3500);
	memcpy(szBuffer,&dwIP,4);
	memcpy(szBuffer + 4,&wdPort,2);
	DWORD dwNum = htonl(dwLoad);
	memcpy(szBuffer + 6,&dwNum,4);

	COMMAND cmd;
	cmd.dwCmd = TRANS_DATA_REQ;
	return m_pITcpAttemperEngine->SendData(m_dwGatewayRound,m_dwGatewayIndex,cmd,szBuffer,10);
}
