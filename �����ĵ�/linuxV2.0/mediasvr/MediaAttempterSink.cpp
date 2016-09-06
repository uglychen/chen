////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��MediaAttempterSink.cpp
// Project Code ������˻ص�����Ҫ�ǿͻ������ӣ��û���
// Abstract     ��
// Reference    ��
//
// Version      ��1.0
// Author       ��LiuJun
// Accomplished date �� 07 28, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "ReadIniFile.h"
#include "MediaAttempterSink.h"
#include "mediasvr.h"
////////////////////////////////////////////////////////////////////////////
enum emBufferCode
{
	EM_BUFFER_CLUSTER				= -1,				//��Ⱥ����
	EM_BUFFER_MEDIA_DATA			= 0,				//���ص�����Դ����
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
//�ӿڲ�ѯ
void *  CMediaAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IClusterEvent,dwQueryVer);
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE(IClientAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//��ӷ�����Ƶ״̬������Ѿ����ڣ������Ѿ������ӷ����÷���������Ƶ�ˣ�
//����Ҫ�����·���,Ĭ��Ϊtrue�������ɹ�����Ҫ��ֹ�ظ�����
bool CMediaAttempterSink::AddPublishAudio(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_audioDstSvrPublishStateMapLock);
	if(m_audioDstSvrPublishStateMap.find(udwSvrID) != m_audioDstSvrPublishStateMap.end())
		return false;
	m_audioDstSvrPublishStateMap[udwSvrID] = true;
	return true;
}
//ɾ����Ƶ����״̬
bool CMediaAttempterSink::RemovePublishAudio(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_audioDstSvrPublishStateMapLock);
	m_audioDstSvrPublishStateMap.erase(udwSvrID);
	return true;
}
//��ӷ�����Ƶ״̬������Ѿ����ڣ������Ѿ������ӷ����÷���������Ƶ�ˣ�
//����Ҫ�����·���,Ĭ��Ϊtrue�������ɹ�����Ҫ��ֹ�ظ�����
bool CMediaAttempterSink::AddPublishVideo(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_videoDstSvrPublishStateMapLock);
	if(m_videoDstSvrPublishStateMap.find(udwSvrID) != m_videoDstSvrPublishStateMap.end())
		return false;
	m_videoDstSvrPublishStateMap[udwSvrID] = true;
	return true;
}
//ɾ����Ƶ����״̬
bool CMediaAttempterSink::RemovePublishVideo(UNDWORD udwSvrID)
{
	CThreadLockHandle lockHandle(&m_videoDstSvrPublishStateMapLock);
	m_videoDstSvrPublishStateMap.erase(udwSvrID);
	return true;
}
//��ȡ���е���Ϣ�ṹ,û�еĻ�new�µ�
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
//�ͷ����͵���Ϣ�ṹ
void CMediaAttempterSink::FreePublishInfo(PublishInfo* pPublishInfo)
{
	ASSERT(pPublishInfo != NULL);
	if(pPublishInfo == NULL) return;
	CThreadLockHandle lockHandle(&m_freePublishInfoListLock);
	m_freePublishInfoList.push_back(pPublishInfo);
}
//ɾ�����п��е���Ϣ�ṹ
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
//��ʼ��
bool CMediaAttempterSink::InitServer(DWORD dwMaxConnectNum /* = 500000*/)
{
	if(m_clusterHelper.CreateInstance() == false)
	{
		 OUT_ERROREX("����ý���������Ⱥ���ʧ��:%s",m_clusterHelper.GetErrorMessage());
		 return false;
	 }
	 m_pIClusterEngine = m_clusterHelper.GetInterface();
	 IClusterEvent *pIClusterEvent = (IClusterEvent*)this->QueryInterface(VER_IClusterEvent);
	 m_pIClusterEngine->SetIClusterEventNotify(pIClusterEvent);
	 m_pIClusterData = (IClusterData*)m_pIClusterEngine->QueryInterface(VER_IClusterData);
	 if((m_ipPortID.udwIpPortID = m_pIClusterEngine->InitCluster()) == ((UNDWORD)-1))
	 {
		 OUT_ERROR("��ʼ����Ⱥ���ʧ��");
		 return false;
	 }
	 OUT_INFOEX("��ʼ��ý���������Ⱥ��ɣ�ID:0x%llx  IP:0x%x PORT:%u",m_ipPortID.udwIpPortID,m_ipPortID.dwIp,m_ipPortID.wPort);
		//��ʼ����Ⱥ�������� 
	if(m_tcpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("����ý������������ʧ�ܣ�����������%s",m_tcpNetworkHelper.GetErrorMessage()); 
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
	 
	//DWORD		dwInitMemSize;				// DataStorage ���ڴ��ʼ����С - �䳤���ݴ洢 �� [1, 100] ��λM��Ĭ��1M
	//DWORD		dwIocpThreadCount;			// IOCP�Ĺ����̸߳��� : ������cpu������* 5
	//DWORD       dwRevcQueueServerNum;		// ���ն��и���,Ĭ��ΪdwIocpThreadCount
	//DWORD		dwRecvQueuePkgMaxNum;		// ���ն���������ݰ��������ƣ�Ĭ��600��(������Ƶ���ݣ���Ƶ10֡/S * 15pkg/֡ * 4s)�����ÿ�����ж������˸ô�С��
	//										// ��̬���Ӷ��к��߳��������µ����ӣ������ӵĶ��к��̲߳��������û����߸��صļ��ٶ����٣���һֱ����
	//DWORD		dwMaxSendMemSize;			// �ͻ��˷��ͻ��������ֵĬ��102400,��λ�ֽڣ�Ĭ��Ϊ��DWORD��-1�����������õ�ֵ���������δ���͵�����

	AttemperParam attempParam;
	attempParam.dwInitMemSize = 1;
	attempParam.dwIocpThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRevcQueueServerNum = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRecvQueuePkgMaxNum = 600;
	attempParam.dwMaxSendMemSize = 102400;
	//��ʼ��
	m_pITcpAttemperEngine->InitServer(&attempParam);

	OUT_INFOEX("��ʼ��ý����������,�����߳�%u,�����߳�%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);

	//��ʼ��UDP����
	if(m_udpAttempterSink.InitServer(&m_streamManager,m_pIClusterData,m_ipPortID.udwIpPortID,dwMaxConnectNum) == false)
		return false;

	if(m_transAVDataEngine.InitTransAVDataEngine(m_udpAttempterSink.GetIUdpAttemperEngine(),m_pITcpAttemperEngine,&m_streamManager,sysconf(_SC_NPROCESSORS_ONLN)) == false)
	{
		OUT_ERROR("��ʼ����ý������ת������ʧ��");
	}

	
	return true;
}
//����
bool CMediaAttempterSink::StartServer(const char *pszIp)
{
	ASSERT(m_pIClusterEngine != NULL);
	if(m_pIClusterEngine->StartCluster() == false)
	{
		OUT_ERROR("����ý���������Ⱥʧ��"); 
		StopServer();
		return false; 
	}

	DWORD dwPort = m_ipPortID.wPort + 10000;
	if(dwPort > 0xFFFE)
	{
		OUT_ERROREX("����ý�������ʧ�ܣ����ط���˿ڷǷ�%u",dwPort);
		StopServer();
		return false;
	}

	if(m_transAVDataEngine.StartTransAVDataEngine() == false)
	{
		OUT_ERROR("����ý��ת������ʧ��"); 
		StopServer();
		return false;
	}

	WORD wPort = (WORD)dwPort;
	ASSERT(m_pITcpAttemperEngine != NULL);
	if(m_pITcpAttemperEngine->StartServer(wPort,TEXT("0.0.0.0")) == false)
	{
		OUT_ERROREX("����ý������������ʧ�ܣ�������ַ:0.0.0.0,PORT��%u",wPort); 
		StopServer();
		return false;
	}
	if(m_pITcpAttemperEngine->StartClientService() == false)
	{
		OUT_ERROR("����ý��������ͻ��˷���ʧ��"); 
		StopServer();
		return false;
	}

	//��ȡ�����ļ�
	//����������������
	TCHAR szConfigPath[] = TEXT("./gate.ini");
	TCHAR szApp[] = TEXT("SYSCONFIG");

	CReadIniFile readFile;
	if(readFile.SetFileName(szConfigPath) == false)
	{
		OUT_ERROREX("��ȡ�����ļ�:%sʧ��",szConfigPath);
		return false;
	}

	char IP[IP_LEN+1] = {0};
	strncpy(IP,"0.0.0.0",strlen("0.0.0.0"));
	DWORD dwLength = readFile.GetStringValue(szApp,TEXT("BIND_IP"),TEXT("0.0.0.0"),IP,IP_LEN);
	WORD wdPort = readFile.GetIntValue(szApp,TEXT("BIND_PORT"),-1);
	if(!dwLength || !wdPort)
	{
		OUT_ERROR("��ȡ����BIND��IP�Ͷ˿ڴ���");
		return false;
	}

	PublishInfo pPublishInfo;
	pPublishInfo.wSrcServerType = MEDIA_SERVER;
	pPublishInfo.wDstServerType = GATEWAY_SERVER;

	DWORD ip = (ntohl)(inet_addr(IP));
	if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)&pPublishInfo,ip,wdPort) == false)
	{
		OUT_ERROR("���ӵ����ط�����ʧ��");
		StopServer();
		return false;
	}
	OUT_INFOEX("����ý����������,��Ⱥ�˿�%u,���ط���˿�%u",m_ipPortID.wPort,wPort);
	//����UDP����
	if(m_udpAttempterSink.StartServer(pszIp) == false)
		return false;
	return true;
}
//ֹͣ
bool CMediaAttempterSink::StopServer()
{
	m_transAVDataEngine.StopTransAVDataEngine();
	m_udpAttempterSink.StopServer();
	m_pIClusterEngine->StopCluster();
	m_pITcpAttemperEngine->StopServer();
	OUT_INFO("ֹͣ��ý����������"); 
	return true;
}
//������Դ
void CMediaAttempterSink::RecycleResource()
{
	m_streamManager.ClearStreamResource();
}
//��Ⱥ�¼�֪ͨ
void CMediaAttempterSink::ClusterEventNotify(tagClusterEvent *pClusterEvent)
{
	ASSERT(pClusterEvent != NULL);
	if(pClusterEvent == NULL) return;

	OUT_INFOEX("��Ⱥ�¼�֪ͨ��type(1,���ͷ�������Ϣ;2,�������;3,���������֪ͨ;4,���뼯Ⱥ֪ͨ;5,��Ⱥ�˳�֪ͨ):%d code:%d ADDR:0x%llx",pClusterEvent->type,pClusterEvent->nCode,pClusterEvent->udwAddrID);
	switch(pClusterEvent->type)
	{
	case EM_CLUSTER_PUSH_ADDR_NOTIFY:
		{ 
			OUT_INFOEX("��Ⱥת����Ϣ����Ⱥ�������б�����:0x%llx",pClusterEvent->udwAddrID);
			break;
		}
	case EM_CLUSTER_PUSH_ADDR_OVER_NOTIFY:
		{
			//����״̬Ϊ0�ķ�����
			OUT_INFO("��Ⱥת����Ϣ����Ⱥ�������б��������");
			break;
		}
	case EM_CLUSTER_SVR_CHANAGE_NOTIFY:
		{
			OUT_INFOEX("��Ⱥת����Ϣ����Ⱥ������ 0x%llx ���״̬ %d",pClusterEvent->udwAddrID,pClusterEvent->nCode);
			break; 
		}
	case EM_CLUSTER_JOINE_NOTIFY:
		{
			OUT_INFO("��Ⱥת����Ϣ�������������뼯Ⱥ�ɹ�");
			break; 
		}
	case EM_CLUSTER_EXIT_NOTIFY:
		{
			if(pClusterEvent->nCode == 0)
				OUT_INFO("��Ⱥת����Ϣ���������˳���Ⱥ�ɹ�");
			else
				OUT_WARNEX("��Ⱥת����Ϣ�����棺���������˳��˼�Ⱥ��������%s",pClusterEvent->szBuffer);
			break;
		}
	default:
		OUT_WARNEX("��Ⱥת����Ϣ����Ⱥ�¼�������Ч:type %d  code %d",pClusterEvent->type,pClusterEvent->nCode);
		break;
	}
}
//����ת��,��Ⱥ�׵��ϲ������ֱ�����û�ָ�ֱ�Ӵ�������
void CMediaAttempterSink::OnClusterTransDataEvent(COMMAND command,char *pData,WORD wDataSize)
{
	switch(command.dwCmd)
	{ 
		//��Ҫ������Ƶ����
	case USER_PUBLISH_AUDIO_REQ:
		OnClusterPublishAudio(command,pData,wDataSize);
		//��Ҫ������Ƶ����
	case USER_PUBLISH_VIDEO_REQ:
		OnClusterPublishVideo(command,pData,wDataSize);
	case USER_PLAY_AUDIO_REQ:
		//�������Դ���ڱ��������ǳ�ʼ����Դ�������ͻ������ӣ�������Դ�Խ�
		OnClusterPlayAudio(command,pData,wDataSize);
	case USER_PLAY_VIDEO_REQ:
		OnClusterPlayVideo(command,pData,wDataSize);
		//��Ƶ��Դֹͣ����
	case USER_STOP_PUBLISH_AUDIO_REQ:
		OnClusterStopPublishAudio(command,pData,wDataSize);
		//��Ƶ��Դֹͣ����
	case USER_STOP_PUBLISH_VIDEO_REQ:
		OnClusterStopPublishVideo(command,pData,wDataSize);
	}
}
//////////////////////////////////��Ⱥ���ݴ���////////////////////////////////////////////
//��Ⱥת����������������Դ���ڵķ�����λ��,�����ǰ״̬��û�����ͣ�����������
bool CMediaAttempterSink::OnClusterPlayAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("��Ⱥת����Ϣ��������Ƶ��������Ϣ���ݷǷ�(size >= 8),size %u",wDataSize);
		return true;
	}
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);

	//���󲥷ŵķ�����
	UNDWORD udwIpPort = 0;
	memcpy(&udwIpPort,pBuffer + 8,8);
	udwIpPort = ntohl64(udwIpPort);

	//�Ƿ��Ǳ��ص���Ƶ����
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL || pStreamItem->GetAudioResourceType() != ST_USER_CLIENT)
	{
		OUT_DEBUGEX("��Ⱥת����Ϣ�������� %llu Ҫ�󲥷���Ƶ�� %llu,������������ԭʼ����Դ����ת��",udwIpPort,udwStreamID);	
		return true;
	} 
	OUT_INFOEX("��Ⱥת����Ϣ�������� %llu Ҫ�󲥷���Ƶ�� %llu",udwIpPort,udwStreamID);	

	//����
	if(AddPublishAudio(udwIpPort) == true)
	{
		//��ȡ����������Ϣ
		PublishInfo *pPublishInfo = GetFreePublishInfo();
		pPublishInfo->mediaType = EM_MEDIA_TYPE_AUDIO;
		pPublishInfo->udwStreamID = udwStreamID;
		pPublishInfo->udwSvrID = udwIpPort;
		IPPORTID ipPort;
		ipPort.udwIpPortID = udwIpPort;
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,ipPort.dwIp,ipPort.wPort + 10000) == false)
		{
			OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�����ͻ�������ʧ��",udwIpPort,udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishAudio(udwIpPort);
		}
	}
	else
	{
		OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�Ѿ�����",udwIpPort,udwStreamID);
	}
	
	return true;
}
//��Ⱥת����������������Դ���ڵķ�����λ��,�����ǰ״̬��û�����ͣ�����������
bool CMediaAttempterSink::OnClusterPlayVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("��Ⱥת����Ϣ��������Ƶ��������Ϣ���ݷǷ�(size >= 8),size %u",wDataSize);
		return true;
	}

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);

	//Ҫ�����͵ķ�����
	UNDWORD udwIpPort = 0;
	memcpy(&udwIpPort,pBuffer + 8,8);
	udwIpPort = ntohl64(udwIpPort);

	//�Ƿ��Ǳ��ص���Ƶ����
	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL || pStreamItem->GetVideoResourceType() != ST_USER_CLIENT)
	{
		OUT_DEBUGEX("��Ⱥת����Ϣ�������� %llu Ҫ�󲥷���Ƶ�� %llu,������������ԭʼ����Դ����ת��",udwIpPort,udwStreamID);	
		return true;
	} 
	OUT_INFOEX("��Ⱥת����Ϣ�������� %llu Ҫ�󲥷���Ƶ�� %llu",udwIpPort,udwStreamID);	
	//����
	if(AddPublishVideo(udwIpPort) == true)
	{
		//��ȡ����������Ϣ
		PublishInfo *pPublishInfo = GetFreePublishInfo();
		pPublishInfo->mediaType = EM_MEDIA_TYPE_VIDEO;
		pPublishInfo->udwStreamID = udwStreamID;
		pPublishInfo->udwSvrID = udwIpPort;
		IPPORTID ipPort;
		ipPort.udwIpPortID = udwIpPort;
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,ipPort.dwIp,ipPort.wPort + 10000) == false)
		{
			OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�����ͻ�������ʧ��",udwIpPort,udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishVideo(udwIpPort);
		}
	}
	else 
	{
		OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�Ѿ�����",udwIpPort,udwStreamID);
	}
	
	return true;
}
//��Ⱥ�������ķ�����Ƶ,��������������Ƶ���û�������0Ҫ����������
bool CMediaAttempterSink::OnClusterPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("��Ⱥת����Ϣ��������Ƶ��������Ϣ���ݷǷ�(size >= 8),size %u",wDataSize);
		return true;
	}
	//�������ݣ�4�ֽڷ���ID��4�ֽڷ�����ID��8�ֽڷ�����ID
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);
	 
	//�ϴ����ݵķ�����
	UNDWORD udwSvrID = 0;
	memcpy(&udwSvrID,pBuffer + 8,8);
	udwSvrID = ntohl64(udwSvrID);

	//��Ϊ����������Ҫ������Ƶ
	UNDWORD udwDstSvr = htonl64(m_ipPortID.udwIpPortID);
	memcpy(pBuffer + 8,&udwDstSvr,8);
	OUT_INFOEX("��Ⱥת����Ϣ���û� %u ������Ƶ�� %llu",dwUserID,udwStreamID);	

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	//���������յ������󲥷ŵ�PLAY
	if(pStreamItem != NULL && (pStreamItem->IsPublishAudio() == false || pStreamItem->GetCurrentAudioTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL) && pStreamItem->GetStreamAudioUserCount() > 0)
	{
		//��Դ������Ҫ����
		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_AUDIO_REQ;
		playCmd.dwSequence = 0;
	
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransDstSvr(udwSvrID,playCmd,pBuffer,wDataSize);

		OUT_DEBUGEX("��Ⱥת����Ϣ�������� %llu ͨ����Ⱥ��Դ������ %llu ������Ƶ�� %llu ����",m_ipPortID.udwIpPortID,udwSvrID,udwStreamID);
	}
	return true;
}
//��Ⱥ�������ķ�����Ƶ,�������������Ƶ���û�������0Ҫ��������Ƶ����
bool CMediaAttempterSink::OnClusterPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize >= 16);
	if(pBuffer == NULL || wDataSize < 16) 
	{
		OUT_WARNEX("��Ⱥת����Ϣ��������Ƶ��������Ϣ���ݷǷ�(size >= 8),size %u",wDataSize);
		return true;
	}
	//�������ݣ�4�ֽڷ���ID��4�ֽڷ�����ID��8�ֽڷ�����ID
	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pBuffer,udwStreamID);
	 
	UNDWORD udwSvrID = 0;
	memcpy(&udwSvrID,pBuffer + 8,8);
	udwSvrID = ntohl64(udwSvrID);

	UNDWORD udwDstSvr = htonl64(m_ipPortID.udwIpPortID);
	memcpy(pBuffer + 8,&udwDstSvr,8);
	OUT_INFOEX("��Ⱥת����Ϣ���û� %u ������Ƶ�� %llu",dwUserID,udwStreamID);	

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	//���������յ������󲥷ŵ�PLAY
	if(pStreamItem != NULL && (pStreamItem->IsPublishVideo() == false || pStreamItem->GetCurrentVideoTimeInterval() > MEDIA_DATA_MAX_SECONDS_INTERNVL) && pStreamItem->GetStreamVideoUserCount() > 0)
	{
		//��Դ������Ҫ����
		COMMAND playCmd;
		playCmd.dwCmd = USER_PLAY_VIDEO_REQ;
		playCmd.dwSequence = 0;
	
		ASSERT(m_pIClusterData != NULL);
		m_pIClusterData->TransDstSvr(udwSvrID,playCmd,pBuffer,wDataSize);

		OUT_INFOEX("��Ⱥת����Ϣ�������� %llu ͨ����Ⱥ��Դ������ %llu ������Ƶ�� %llu ����",m_ipPortID.udwIpPortID,udwSvrID,udwStreamID);
	}
	return true;
}
//��Ƶ��Դֹͣ��������ʦֹͣ�ϴ�����ͬ�������Դ���µ��û�������ʱ�򶼻��и���Ϣ
bool CMediaAttempterSink::OnClusterStopPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(wDataSize >= 16);
	if(wDataSize < 16) return true;

	UNDWORD udwSrcSvr = 0,udwStreamID = 0;
	memcpy(&udwSrcSvr,pBuffer,8);
	memcpy(&udwStreamID,pBuffer + 8,8);

	udwSrcSvr = ntohl64(udwSrcSvr);
	udwStreamID = ntohl64(udwStreamID);

	OUT_WARNEX("��Ƶ��:%llu �ڷ�����0x%llx�Ϸ���������0x%llxֹͣ����������",udwStreamID,udwSrcSvr,m_ipPortID.udwIpPortID);
	CStreamItem *pStream = m_streamManager.GetStreamItem(udwStreamID);
	if(pStream == NULL) return true;

	//ֹͣ��Ƶ���͵���
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
	//���������Ƶû�з�������ƵҲû�з�����������û���Ϊ0�����������
	if(pStream->IsPublishAudio() == false && pStream->IsPublishVideo() == false && nUserCount == 0)
	{  
		m_streamManager.RemoveStream(udwStreamID);
		OUT_WARNEX("��Ⱥ֪ͨ��Ƶ��ֹͣ�˷��������������ݣ���ID��%llu ,��Ƶ�û���:%d,��Ƶ�û�:%d,������Ƶ�ͻ�����:%d,������Ƶ�ͻ�����:%d",
			udwStreamID,pStream->GetStreamVideoUserCount(),pStream->GetStreamAudioUserCount(),
			pStream->GetPublishAudioClientCount(),pStream->GetPublishVideoClientCount());
	}
	
	return true;
}
//��Ƶ��Դֹͣ��������ʦֹͣ�ϴ�����ͬ�������Դ���µ��û�������ʱ�򶼻��и���Ϣ
bool CMediaAttempterSink::OnClusterStopPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize)
{
	ASSERT(wDataSize >= 16);
	if(wDataSize < 16) return true;

	UNDWORD udwSrcSvr = 0,udwStreamID = 0;
	memcpy(&udwSrcSvr,pBuffer,8);
	memcpy(&udwStreamID,pBuffer + 8,8);

	udwSrcSvr = ntohl64(udwSrcSvr);
	udwStreamID = ntohl64(udwStreamID);

	OUT_WARNEX("��Ƶ��:%llu �ڷ�����0x%llx�Ϸ���������0x%llxֹͣ����������",udwStreamID,udwSrcSvr,m_ipPortID.udwIpPortID);
	CStreamItem *pStream = m_streamManager.GetStreamItem(udwStreamID);
	if(pStream == NULL) return true;

	//ֹͣ��Ƶ���͵���
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
	//���������Ƶû�з�������ƵҲû�з�����������û���Ϊ0�����������
	if(pStream->IsPublishAudio() == false && pStream->IsPublishVideo() == false && nUserCount == 0)
	{  
		m_streamManager.RemoveStream(udwStreamID);
		OUT_WARNEX("��Ⱥ֪ͨ��Ƶ��ֹͣ�˷��������������ݣ���ID��%llu ,��Ƶ�û���:%d,��Ƶ�û�:%d,������Ƶ�ͻ�����:%d,������Ƶ�ͻ�����:%d",
			udwStreamID,pStream->GetStreamVideoUserCount(),pStream->GetStreamAudioUserCount(),
			pStream->GetPublishAudioClientCount(),pStream->GetPublishVideoClientCount());
	}
	return true;
}
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//����������Ϣ
bool CMediaAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//�������������
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("ý�����������˳������������%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u,�Ͽ��ÿͻ�������",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("ý������������:�ͻ�������IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u",
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
//�����ȡ��Ϣ
bool CMediaAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//��������¼
	case SYS_LOGIN_REQ:
		return OnReqSvrLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//�û���¼
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PUBLISH_VIDEO_REQ:
		return OnReqPublishVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PUBLISH_AUDIO_REQ:
		return OnReqPublishAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PLAY_VIDEO_REQ:
		return OnReqPlayVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PLAY_AUDIO_REQ:
		return OnReqPlayAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//��Ƶͷ
	case USER_AUDIO_HEADER_REQ:
		return OnReqAudioHeaderData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);;
		//��Ƶͷ
	case USER_VIDEO_HEADER_REQ:
		return OnReqVideoHeaderData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize); 
		//��Ƶ����
	case USER_AUDIO_DATA_REQ:
		return OnReqAudioData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);;
		//��Ƶ����
	case USER_VIDEO_DATA_REQ:
		return OnReqVideoData(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize); 
		//����
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//����ر���Ϣ
bool CMediaAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("ý�����ˣ��ͻ������ӶϿ�id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	bool bIsClearStream = true;
	//����������,�������ʦ������ֻ��Ҫ�޸�״̬Ϊû�з����������ѧ����ɾ�������е�ѧ�����ڿ��û����Ƿ�Ϊ0��Ϊ0������
	if(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID != (UNDWORD)-1)
	{
		CStreamItem *pStreamItem = m_streamManager.GetStreamItem(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
		if(pStreamItem != NULL)
		{
			//��ǰ���ϴ�����
			int nUserCount = 0;
			if(m_pConnectInfo[pCloseEvent->dwIndexID].bIsPublish == true)
			{ 

				//�ϴ�������Ƶ,������Ƶ״̬Ϊû���ϴ�
				if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
				{
					//�����ǰ�Ͽ������Ӻ����ڷ���������һ�£�����������
					if(pStreamItem->GetPublishAudioIndexID() == pCloseEvent->dwIndexID && pStreamItem->GetPublishAudioRoundID() == pCloseEvent->dwRoundID)
					{
						//���͵����з�����
						if(m_pConnectInfo[pCloseEvent->dwIndexID].wClientType == ST_USER_CENTER) 
						{
							//ͨ����Ⱥ֪ͨԭ���ͷ�����
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
						//������������
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
						OUT_INFOEX("��ʦ%lluֹͣ���ϴ���Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamAudioUserCount(),
							pStreamItem->GetPublishAudioClientCount());
					}
					else
					{
						OUT_WARNEX("��ʦ%lluֹͣ���ϴ���Ƶ���ݣ����ǵ�ǰ�Ͽ����������������ϴ���������һ�£����Բ��������ݡ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d cindex:%u cround:%u pindex:%u pround:%u",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamAudioUserCount(),
							pStreamItem->GetPublishAudioClientCount(),pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pStreamItem->GetPublishAudioIndexID(),pStreamItem->GetPublishAudioRoundID());
						bIsClearStream = false;
					}
				}
				else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //��Ƶ
				{
					if(pStreamItem->GetPublishVideoIndexID() == pCloseEvent->dwIndexID && pStreamItem->GetPublishVideoRoundID() == pCloseEvent->dwRoundID)
					{
						//���͵����з�����
						if(m_pConnectInfo[pCloseEvent->dwIndexID].wClientType == ST_USER_CENTER) 
						{
							//ͨ����Ⱥ֪ͨԭ���ͷ�����
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
						//������������
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
						OUT_INFOEX("��ʦ%lluֹͣ���ϴ���Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),
							pStreamItem->GetPublishVideoClientCount());
					}
					else
					{
						OUT_WARNEX("��ʦ%lluֹͣ���ϴ���Ƶ���ݣ����ǵ�ǰ�Ͽ����������������ϴ���������һ�£����Բ��������ݡ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d cindex:%u cround:%u pindex:%u pround:%u",
							m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),
							pStreamItem->GetPublishVideoClientCount(),pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pStreamItem->GetPublishVideoIndexID(),pStreamItem->GetPublishVideoRoundID());
						bIsClearStream = false;
					}
				}
				nUserCount = pStreamItem->GetPublishAudioClientCount();//���仹ʣ������
				nUserCount += pStreamItem->GetPublishVideoClientCount();//���仹ʣ������
			}
			else 
			{
				//���󲥷ŵ�����Ƶ������Ƶ
				if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
				{
					nUserCount = pStreamItem->RemoveAudioUser(pCloseEvent->dwIndexID);
					OUT_DEBUGEX("ѧ��%lluֹͣ�˲�����Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
						m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetPublishAudioClientCount());
					nUserCount += pStreamItem->GetPublishVideoClientCount();//���仹ʣ������
				}
				else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //��Ƶ
				{
					nUserCount = pStreamItem->RemoveVideoUser(pCloseEvent->dwIndexID);
					OUT_DEBUGEX("ѧ��%lluֹͣ�˲�����Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
						m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,
						pStreamItem->GetPublishVideoClientCount());
					nUserCount += pStreamItem->GetPublishAudioClientCount();//���仹ʣ������
				}
			}

			//���������Ƶû�з�������ƵҲû�з�����������û���Ϊ0�����������
			if(bIsClearStream && pStreamItem->IsPublishAudio() == false && pStreamItem->IsPublishVideo() == false && nUserCount == 0)
			{  
				m_streamManager.RemoveStream(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
				OUT_INFOEX("���������ݣ���ID��%llu ,��Ƶ�û���:%d,��Ƶ�û�:%d,������Ƶ�ͻ�����:%d,������Ƶ�ͻ�����:%d",
					pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount());
			}
			else
			{
				OUT_INFOEX("����������(������)����ID��%llu ,�û�����%u,��Ƶ�û���:%d,��Ƶ�û�:%d,������Ƶ�ͻ�����:%d,������Ƶ�ͻ�����:%d,��Ƶ����״̬:%d ��Ƶ����״̬:%d",
					pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount(),pStreamItem->IsPublishAudio(),pStreamItem->IsPublishVideo());
			}
		}
	}

	//TCP�ϴ���TCP,����ڵ�¼����¶��ߣ�����ʦ���ؼ�1
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
//����
bool CMediaAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,NULL,0);
}
//��������¼
bool CMediaAttempterSink::OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= 8);
	if(pData == NULL || wDataSize < 8) 
	{
		OUT_WARNEX("�յ��Ƿ��ķ�������¼����(size >= 8),size %u",wDataSize);
		return false;
	}

	UNDWORD udwID = 0;
	memcpy(&udwID,pData,8);
	udwID = ntohl64(udwID); 

	OUT_INFOEX("��������½:ID %llu",udwID); 
	m_pConnectInfo[dwIndexID].udwID = udwID;
	m_pConnectInfo[dwIndexID].wClientType = ST_DATA_TRANS_SVR;
	m_pConnectInfo[dwIndexID].wStatus = 1;

	int nRet = 0;
	command.dwCmd = SYS_LOGIN_RESP;
	ASSERT(m_pITcpAttemperEngine != NULL);
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//�ͻ��˵�½
bool CMediaAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("�յ��Ƿ����û���¼����(size >= 4),size %u",wDataSize);
		return false;
	}
	 
	DWORD dwUserID = 0; 

	char *pBuffer = pData;
	memcpy(&dwUserID,pBuffer,4);
	pBuffer += 4;
	dwUserID = ntohl(dwUserID);
	 
	OUT_DEBUGEX("�û���½:%llu",dwUserID); 

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
//�û�������Ƶ����
bool CMediaAttempterSink::OnReqPublishVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;

	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//������û��ϴ����ݣ������������з��������������������Դ
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
	//�жϵ�ǰ��Ƶ������״̬Ϊtrue���������һ�ε���Ƶ����ʱ���Ѿ�������2��
	DWORD dwTimeInterval = 0;
	if(pStreamItem->IsPublishVideo() == true && (dwTimeInterval = pStreamItem->GetCurrentVideoTimeInterval()) > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		//�ж��Ƿ��Ǳ�����Ƶ���ݷ�
		DWORD dwPublishIndex = pStreamItem->GetPublishVideoIndexID();
		//���û��������������͵ķ���������
		if(m_pConnectInfo[dwPublishIndex].wClientType == ST_USER_CLIENT)
		{
			//ʹ�û����������ǲ���ͬһ������
			if(dwIndexID == dwPublishIndex && dwRoundID == pStreamItem->GetPublishVideoRoundID())
			{
				OUT_WARNEX("��ͨ�û�%llu �ظ�����ͬһ��Ƶ��Դ��:%llu����һ��Ƶ�������������Ƶ����Ϊ%u��ǰ,����i:%u r:%u",m_pConnectInfo[dwIndexID].udwID,
					udwStreamID,dwTimeInterval,dwIndexID,dwRoundID);
			}
			else if(m_pConnectInfo[dwPublishIndex].dwRoundID != pStreamItem->GetPublishVideoRoundID())
			{
				OUT_WARNEX("���ؾ�����:%llu��SOCKET�����Ѿ������ã�������Ƶ������Դ�����ӻ��Ǹ���ǰ�����ӣ���Դû�м�ʱ����,ni:%u nr:%u oi:%u or:%u",
					udwStreamID,dwIndexID,dwRoundID,dwPublishIndex,pStreamItem->GetPublishVideoRoundID());
			}
		}
		pStreamItem->SetStopPublishVideo();
	}


	ASSERT(pStreamItem != NULL);
	if(pStreamItem->SetPublishVideoInfo(m_pConnectInfo[dwIndexID].wClientType,dwIndexID,dwRoundID,udwStreamID) == false)
	{
		OUT_WARNEX("�û�%llu:%u ����%u ����%llu��Ƶ����ʧ�ܣ�������Դ�Ѿ����û�����",m_pConnectInfo[dwIndexID].udwID,dwUserID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
		int nRet = htonl((int)1);
		command.dwCmd = USER_PUBLISH_VIDEO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	//�û������Ƿ���
	m_pConnectInfo[dwIndexID].bIsPublish = true;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_VIDEO;	//������������Ƶ
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 
	//֪ͨ����������,�ͻ��˵�½��˵����Դ����
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
		OUT_INFOEX("�û�%llu ����%u ����%llu��Ƶ����,֪ͨ����������",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
	}
	else 
		OUT_INFOEX("�û�%llu ����%u ����%llu��Ƶ����",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

	int nRet = 0;
	command.dwCmd = USER_PUBLISH_VIDEO_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//�û�������Ƶ����
bool CMediaAttempterSink::OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;

	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//������û��ϴ����ݣ������������з��������������������Դ
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
	//�жϵ�ǰ��Ƶ������״̬Ϊtrue���������һ�ε���Ƶ����ʱ���Ѿ�������2��
	DWORD dwTimeInterval = 0;
	if(pStreamItem->IsPublishAudio() == true && (dwTimeInterval = pStreamItem->GetCurrentAudioTimeInterval()) > MEDIA_DATA_MAX_SECONDS_INTERNVL)
	{
		//�ж��Ƿ��Ǳ�����Ƶ���ݷ�
		DWORD dwPublishIndex = pStreamItem->GetPublishAudioIndexID();
		//���û��������������͵ķ���������
		if(m_pConnectInfo[dwPublishIndex].wClientType == ST_USER_CLIENT)
		{
			//ʹ�û����������ǲ���ͬһ������
			if(dwIndexID == dwPublishIndex && dwRoundID == pStreamItem->GetPublishAudioRoundID())
			{
				OUT_WARNEX("��ͨ�û�%llu �ظ�����ͬһ��Ƶ��Դ��:%llu����һ��Ƶ�������������Ƶ����Ϊ%u��ǰ,����i:%u r:%u",m_pConnectInfo[dwIndexID].udwID,
					udwStreamID,dwTimeInterval,dwIndexID,dwRoundID);
			}
			else if(m_pConnectInfo[dwPublishIndex].dwRoundID != pStreamItem->GetPublishAudioRoundID())
			{
				OUT_WARNEX("���ؾ�����:%llu��SOCKET�����Ѿ������ã�������Ƶ������Դ�����ӻ��Ǹ���ǰ�����ӣ���Դû�м�ʱ����,ni:%u nr:%u oi:%u or:%u",
					udwStreamID,dwIndexID,dwRoundID,dwPublishIndex,pStreamItem->GetPublishAudioRoundID());
			}
		}
		pStreamItem->SetStopPublishAudio();
	}


	if(pStreamItem->SetPublishAudioInfo(m_pConnectInfo[dwIndexID].wClientType,dwIndexID,dwRoundID,udwStreamID) == false)
	{
		OUT_WARNEX("�û�%llu:%u ����%u ����%llu��Ƶ����ʧ�ܣ�������Դ�Ѿ����û�����",m_pConnectInfo[dwIndexID].udwID,dwUserID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
		int nRet = htonl((int)1);
		command.dwCmd = USER_PUBLISH_AUDIO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	//�û������Ƿ���
	m_pConnectInfo[dwIndexID].bIsPublish = true;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_AUDIO;	//������������Ƶ
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 
	//֪ͨ����������,�ͻ��˵�½��˵����Դ����
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
		OUT_INFOEX("�û�%llu ����%u ����%llu��Ƶ����,֪ͨ����������",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);
	}
	else 
		OUT_INFOEX("�û�%llu ����%u ����%llu��Ƶ����",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

	int nRet = 0;
	command.dwCmd = USER_PUBLISH_AUDIO_RESP;
	return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//�û�������Ƶ����
bool CMediaAttempterSink::OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//�û������ǲ���
	m_pConnectInfo[dwIndexID].bIsPublish = false;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_VIDEO;	//������������Ƶ
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
	ASSERT(pStreamItem != NULL);
	//��play���ȴ���������ȥ������������������
	//int nUserNum = 0;//���ܳ��ֶ����������������ת����Ƶ���ݣ��ڼ�Ⱥ��Ϣ�д���
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
		OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,ͨ����Ⱥ������������������",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

		int nRet = 0;
		command.dwCmd = USER_PLAY_VIDEO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_VIDEO_RESP;
	m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,��ǰ�����û���:%u,��������Դ״̬:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamVideoUserCount(),pStreamItem->IsPublishVideo());
	
	//���������Ƶͷ���ݣ���������Ƶͷ
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
//�û�������Ƶ����
bool CMediaAttempterSink::OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	if(CheckClientIsLogin(dwIndexID,dwRoundID,command.dwCmd) == false)
		return false;
	if(CheckClientData(dwIndexID,dwRoundID,command.dwCmd,pData,wDataSize,8) == false)
		return false;

	UNDWORD udwStreamID = 0;
	BUILD_STREAMID(pData,udwStreamID);

	//�û������ǲ���
	m_pConnectInfo[dwIndexID].bIsPublish = false;
	m_pConnectInfo[dwIndexID].wMediaType = EM_MEDIA_TYPE_AUDIO;	//������������Ƶ
	m_pConnectInfo[dwIndexID].udwPublishStreamID = udwStreamID; 

	CStreamItem *pStreamItem = m_streamManager.GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_streamManager.CreateSteamItem(udwStreamID);
	ASSERT(pStreamItem != NULL);
	//��play���ȴ���������ȥ������������������
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
		OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,ͨ����Ⱥ������������������",m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID);

		int nRet = 0;
		command.dwCmd = USER_PLAY_AUDIO_RESP;
		return m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_AUDIO_RESP;
	m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,��ǰ������Ƶ�û�:%u,��������Դ״̬:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamAudioUserCount(),pStreamItem->IsPublishAudio());
	
	//���������Ƶͷ���ݣ���������Ƶͷ
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
//��Ƶͷ����
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
  
	//������Ƶͷ����
	pStreamItem->SetVideoHeaderData(pData,wDataSize);
	return TransData(pStreamItem,command,pData,wDataSize);
}
//��Ƶͷ����
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
	OUT_INFOEX("�յ��� %llu ��Ƶͷ����,��С:%u",udwStreamID,wDataSize);
	//������Ƶͷ����
	pStreamItem->SetAudioHeaderData(pData,wDataSize);
	return TransData(pStreamItem,command,pData,wDataSize);
}
//��Ƶ����
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
//��Ƶ����
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

//��֤��¼
inline bool CMediaAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("����index:%u roundID:%u ����������0x%x������Ƿ�������δ��¼",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//��֤����
inline bool CMediaAttempterSink::CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= n);
	if(pData == NULL || wDataSize < n)
	{
		OUT_WARNEX("����index:%u roundID:%u ��¼����:%u �����֣�0x%x���ݷǷ� %u >= %u",dwIndexID,dwRoundID,m_pConnectInfo[dwIndexID].wClientType,dwCmd,wDataSize,n);
		return false;
	}
	return true;
}
//��֤�ϴ�Ȩ��
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
		OUT_WARNEX("�û� %llu ���� 0x%x �Ƿ������߱��ϴ���Ƶ���ݵ�Ȩ��",m_pConnectInfo[dwIndexID].udwID,dwCmd);
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
		OUT_WARNEX("�û� %llu ���� 0x%x �Ƿ������߱��ϴ���Ƶ���ݵ�Ȩ��",m_pConnectInfo[dwIndexID].udwID,dwCmd);
		return false;
	} 
	return true;
}
//ת������
inline bool CMediaAttempterSink::TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pStreamItem != NULL);
	if(pStreamItem == NULL) return true;

	//������û���������
	//CPlayUserMap userMap;
	//ת������
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
	//���ӳɹ�
	if(pAcceptEvent->nErrCode == 0)
	{
		pPublishInfo->wRepeatConnectionCounter = 0;
		pPublishInfo->nSleepTime = 1;

		//���͵�¼
		ReqPublishLogin(pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID);
		return true;
	}
	pPublishInfo->wRepeatConnectionCounter++;
	pPublishInfo->nSleepTime *= 2;
	pPublishInfo->nSleepTime = (pPublishInfo->nSleepTime > 60)?60:pPublishInfo->nSleepTime;
	OUT_WARNEX("�������ͻ������� %u �η�����ʧ�ܣ�����:%d,%u����������ӣ�Ŀ�� %llu���� %llu ������%s",pPublishInfo->wRepeatConnectionCounter,pPublishInfo->mediaType,
		pPublishInfo->nSleepTime,pPublishInfo->udwSvrID,pPublishInfo->udwStreamID,pAcceptEvent->szMsg);
	if(pPublishInfo->wRepeatConnectionCounter <= 30)
	{
		if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,pAcceptEvent->ulIpAddr,pAcceptEvent->usPort,pPublishInfo->nSleepTime) == false)
		{
			OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�����ͻ�������ʧ��",pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);	
			FreePublishInfo(pPublishInfo);
			RemovePublishAudio(pPublishInfo->udwSvrID);
			return false;
		}
	}
	else
	{
		OUT_WARNEX("�������ͻ������� %u �η�����ʧ�ܣ�����:%d,%u����������ӣ�Ŀ�� %llu���� %llu ������%s  �Ѿ�����30�����ӣ�����ִ������",pPublishInfo->wRepeatConnectionCounter,pPublishInfo->mediaType,
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
	//ɾ������Ƶ������
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

	OUT_WARNEX("�������ͻ���������������ӶϿ�������:%d,Ŀ�� %llu���� %llu,������������",pPublishInfo->mediaType,pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);
	if(m_pITcpAttemperEngine->ConnectToServer((WPARAM)pPublishInfo,pCloseEvent->dwIpAddr,pCloseEvent->usPort,pPublishInfo->nSleepTime) == false)
	{
		OUT_WARNEX("��Ⱥת���������� %llu Ҫ�󲥷���Ƶ�� %llu,�����ͻ�������ʧ��",pPublishInfo->udwSvrID,pPublishInfo->udwStreamID);	
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
//��¼��������,ID UNDWORD ,TYPE WORD(����Ϊҵ������)
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
//��¼��Ӧ
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
	
		OUT_INFOEX("�յ����ط�����(ID:%llu,Type:%u)��¼�ɹ��ظ���Ϣ",udwSvrID,dstServerType);
		return SendGatewayServerLoad();
	}
	
	return ReqPublishPublish(dwIndexID,dwRoundID,wParam);
}
//����publish
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
//publish��Ӧ
bool CMediaAttempterSink::OnRespPublishPublish(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam)
{
	ASSERT(pBuffer != NULL);
	ASSERT(wDataSize == 4);
	if(wDataSize < 4) return false;
	ASSERT(*((int*)pBuffer) == 0);

	PublishInfo *pPublishInfo = (PublishInfo*)wParam;
	ASSERT(pPublishInfo != NULL);

	OUT_INFOEX("�յ������� %llu �ķ�����RESPONSE,����:%d,��ID %llu",m_ipPortID.udwIpPortID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
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
		OUT_INFOEX("���ͻ������Ƶͷ���ݵ��������� %llu ����:%d,��ID %llu",pPublishInfo->udwSvrID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
	}
	else if(pPublishInfo->mediaType == EM_MEDIA_TYPE_VIDEO && pStream->GetVideoHeaderData(cbBuffer,wSize) == true)
	{
		cmd.dwCmd = USER_VIDEO_HEADER_REQ;
		m_pITcpAttemperEngine->SendData(dwRoundID,dwIndexID,cmd,cbBuffer,wSize);
		pStream->AddPublisVideoClient(dwIndexID,dwRoundID);
		OUT_INFOEX("���ͻ������Ƶͷ���ݵ��������� %llu ����:%d,��ID %llu",pPublishInfo->udwSvrID,pPublishInfo->mediaType,pPublishInfo->udwStreamID);
	}
	return true;
}
//�ϱ��ܸ���
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
