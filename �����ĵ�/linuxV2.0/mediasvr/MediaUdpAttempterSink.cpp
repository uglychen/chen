////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��MediaUdpAttempterSink.cpp
// Project Code ������˻ص�����Ҫ�ǿͻ������ӣ��û���
// Abstract     ��
// Reference    ��
//
// Version      ��1.0
// Author       ��LiuJun
// Accomplished date �� 09 22, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "MediaUdpAttempterSink.h"
#include "MediaAttempterSink.h"
#include "mediasvr.h"
////////////////////////////////////////////////////////////////////////////
enum emBufferCode
{
	EM_BUFFER_CLUSTER				= -1,				//��Ⱥ����
	EM_BUFFER_MEDIA_DATA			= 0,				//���ص�����Դ����
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
//�ӿڲ�ѯ
void *  CMediaUdpAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//��ʼ��
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

	//��ʼ����Ⱥ�������� 
	if(m_udpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("UDP ����ý������������ʧ�ܣ�����������%s",m_udpNetworkHelper.GetErrorMessage()); 
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

	OUT_INFOEX("UDP ��ʼ��ý����������,�����߳�%u,�����߳�%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);
	return true;
}
//����
bool CMediaUdpAttempterSink::StartServer(const char *pszIp)
{
	ASSERT(pszIp != NULL);
	if(pszIp == NULL) return false;

	DWORD dwPort = m_ipPortID.wPort + 10000;
	if(dwPort > 0xFFFE)
	{
		OUT_ERROREX("UDP ����ý�������ʧ�ܣ����ط���˿ڷǷ�%u",dwPort);
		StopServer();
		return false;
	}
	WORD wPort = (WORD)dwPort;
	ASSERT(m_pIUdpAttemperEngine != NULL);
	if(m_pIUdpAttemperEngine->StartServer(wPort,pszIp) == false)
	{
		OUT_ERROREX("UDP ����ý������������ʧ�ܣ�������ַ:%s,PORT��%u",pszIp,wPort); 
		StopServer();
		return false;
	}
	OUT_INFOEX("UDP ����ý����������,��Ⱥ�˿�%u,���ط���˿�%u",m_ipPortID.wPort,wPort);
	return true;
}
//ֹͣ
bool CMediaUdpAttempterSink::StopServer()
{
	m_pIUdpAttemperEngine->StopServer();
	OUT_INFO("UDP ֹͣ��ý����������"); 
	return true;
}
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//����������Ϣ
bool CMediaUdpAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//�������������
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("UDP ý�����������˳������������%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u,�Ͽ��ÿͻ�������",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("UDP ý������������:�ͻ�������IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u",
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
bool CMediaUdpAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//�û���¼
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PLAY_VIDEO_REQ:
		return OnReqPlayVideo(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//������
	case USER_PLAY_AUDIO_REQ:
		return OnReqPlayAudio(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//����
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//����ر���Ϣ
bool CMediaUdpAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("UDP ý�����ˣ��ͻ������ӶϿ�id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	//����������,�������ʦ������ֻ��Ҫ�޸�״̬Ϊû�з����������ѧ����ɾ�������е�ѧ�����ڿ��û����Ƿ�Ϊ0��Ϊ0������
	if(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID != (UNDWORD)-1)
	{
		CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
		if(pStreamItem != NULL)
		{
			//��ǰ���ϴ�����
			int nUserCount = 0;
			//���󲥷ŵ�����Ƶ������Ƶ
			if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_AUDIO)
			{
				nUserCount = pStreamItem->RemoveAudioUser(pCloseEvent->dwIndexID);
				OUT_DEBUGEX("UDP ѧ��%lluֹͣ�˲�����Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
					m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,pStreamItem->GetPublishAudioClientCount());
				nUserCount += pStreamItem->GetPublishVideoClientCount();//���仹ʣ������
			}
			else if(m_pConnectInfo[pCloseEvent->dwIndexID].wMediaType == EM_MEDIA_TYPE_VIDEO) //��Ƶ
			{
				nUserCount = pStreamItem->RemoveVideoUser(pCloseEvent->dwIndexID);
				OUT_DEBUGEX("UDP ѧ��%lluֹͣ�˲�����Ƶ���ݣ���ID��%llu ,��Ƶ�û���:%d,������Ƶ�ͻ�����:%d",
					m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pStreamItem->GetStreamID(),nUserCount,
					pStreamItem->GetPublishVideoClientCount());
				nUserCount += pStreamItem->GetPublishAudioClientCount();//���仹ʣ������
			}

			//���������Ƶû�з�������ƵҲû�з�����������û���Ϊ0�����������
			if(pStreamItem->IsPublishAudio() == false && pStreamItem->IsPublishVideo() == false && nUserCount == 0)
			{  
				m_pStreamManager->RemoveStream(m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID);
				OUT_DEBUGEX("UDP ���������ݣ���ID��%llu ,��Ƶ�û���:%d,��Ƶ�û�:%d,������Ƶ�ͻ�����:%d,������Ƶ�ͻ�����:%d",
					pStreamItem->GetStreamID(),pStreamItem->GetStreamVideoUserCount(),pStreamItem->GetStreamAudioUserCount(),
					pStreamItem->GetPublishAudioClientCount(),pStreamItem->GetPublishVideoClientCount());
			}
		}
	}

	//����ڵ�¼����¶��ߣ����ؼ�1
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
//����
bool CMediaUdpAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pIUdpAttemperEngine->SendDataCtrlCmd(dwRoundID,dwIndexID,command,NULL,0);
}
//�ͻ��˵�½
bool CMediaUdpAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("UDP �յ��Ƿ����û���¼����(size >= 4),size %u",wDataSize);
		return false;
	}
	 
	DWORD dwUserID = 0; 

	char *pBuffer = pData;
	memcpy(&dwUserID,pBuffer,4);
	pBuffer += 4;
	dwUserID = ntohl(dwUserID);
	 
	OUT_DEBUGEX("UDP �û���½:%llu",dwUserID); 
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
//�û�������Ƶ����
bool CMediaUdpAttempterSink::OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_pStreamManager->CreateSteamItem(udwStreamID);
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
		return m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_VIDEO_RESP;
	m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,��ǰ�����û���:%u,��������Դ״̬:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamVideoUserCount(),pStreamItem->IsPublishVideo());
	
	//���������Ƶͷ���ݣ���������Ƶͷ
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
//�û�������Ƶ����
bool CMediaUdpAttempterSink::OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(udwStreamID);
	if(pStreamItem == NULL) pStreamItem = m_pStreamManager->CreateSteamItem(udwStreamID);
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
		return m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
	}
	 
	int nRet = 0;
	command.dwCmd = USER_PLAY_AUDIO_RESP;
	m_pIUdpAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);

	OUT_DEBUGEX("�û�%llu ����%u ���󲥷�%llu��Ƶ����,��ǰ������Ƶ�û�:%u,��������Դ״̬:%u",
		m_pConnectInfo[dwIndexID].udwID,m_pConnectInfo[dwIndexID].wClientType,udwStreamID,pStreamItem->GetStreamAudioUserCount(),pStreamItem->IsPublishAudio());
	
	//���������Ƶͷ���ݣ���������Ƶͷ
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
//��֤��¼
inline bool CMediaUdpAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("UDP ����index:%u roundID:%u ����������0x%x������Ƿ�������δ��¼",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//��֤����
inline bool CMediaUdpAttempterSink::CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= n);
	if(pData == NULL || wDataSize < n)
	{
		OUT_WARNEX("UDP ����index:%u roundID:%u ��¼����:%u �����֣�0x%x���ݷǷ� %u >= %u",dwIndexID,dwRoundID,m_pConnectInfo[dwIndexID].wClientType,dwCmd,wDataSize,n);
		return false;
	}
	return true;
}
//ת������
static FILE *pFile = NULL;
bool CMediaUdpAttempterSink::TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pStreamItem != NULL);
	if(pStreamItem == NULL) return true;

	//ת������
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
