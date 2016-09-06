////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��GatewayAttempterSink.cpp
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
//�ӿڲ�ѯ
void *  CGatewayAttempterSink::QueryInterface(DWORD dwQueryVer)
{
	QUERYINTERFACE(IAttemperEngineSink,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAttemperEngineSink,dwQueryVer);
	return NULL;
}
//��ʼ��
bool CGatewayAttempterSink::InitServer(DWORD dwMaxConnectNum, char* ip, WORD wdPort)
{
	
	//��ʼ����Ⱥ�������� 
	if(m_tcpNetworkHelper.CreateInstance() == false)
	{
		OUT_ERROREX("�������ط����������ʧ�ܣ�����������%s",m_tcpNetworkHelper.GetErrorMessage()); 
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
	 
	//DWORD		dwInitMemSize;				// DataStorage ���ڴ��ʼ����С - �䳤���ݴ洢 
	//DWORD		dwIocpThreadCount;			// IOCP�Ĺ����̸߳��� : < SystemInfo.dwNumberOfProcessors * 5
	//DWORD       dwRevcQueueServerNum;		// ���ն��и���
	//DWORD		dwInitSendMemSize;			// ÿ���ͻ��˷���ĳ�ʼ���ͻ�������С(��λ��KB��
	//DWORD		dwMaxSendMemSize;			// �ͻ��˷��ͻ��������ֵ�������Ͽ�����(��λ���ֽڣ�
	//DWORD		dwMaxAcceptCount;			// �����������Ĭ��1024
	AttemperParam attempParam;
	attempParam.dwInitMemSize = 1;
	attempParam.dwIocpThreadCount = sysconf(_SC_NPROCESSORS_ONLN);
	attempParam.dwRevcQueueServerNum = attempParam.dwIocpThreadCount;// / 2;
	attempParam.dwMaxSendMemSize = 8192000;
	m_pITcpIOAttemperEngine->InitServer(&attempParam);
	
	OUT_INFOEX("��ʼ�����ط��������,�����߳�%u,�����߳�%u",attempParam.dwIocpThreadCount,attempParam.dwRevcQueueServerNum);
	m_ipPortID.wPort = wdPort;
	//m_ipPortID.dwIp = inet_addr(IP);
	if(NULL==ip || !wdPort)
	{
	}
	return true;
}
//����
bool CGatewayAttempterSink::StartServer(char *IP)
{
	DWORD dwPort = m_ipPortID.wPort;
	if(dwPort > 0xFFFE || !IP)
	{
		OUT_ERROREX("�������ط�����ʧ�ܣ����ط���˿ڷǷ�%u",dwPort);
		StopServer();
		return false;
	}
	WORD wPort = (WORD)dwPort;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	if(m_pITcpIOAttemperEngine->StartServer(wPort,IP) == false)
	{
		OUT_ERROREX("�������ط����������ʧ�ܣ�������ַ(%s:%u)",IP,wPort); 
		StopServer();
		return false;
	}
	OUT_INFOEX("�������ط��������,���ط������(%s:%u)",IP,wPort);
	return true;
}
//ֹͣ
bool CGatewayAttempterSink::StopServer()
{
	m_pITcpIOAttemperEngine->StopServer();
	OUT_INFO("ֹͣ�����ط��������"); 
	return true;
}
#define BUILD_STREAMID(p,id) DWORD dwRoomID = 0,dwUserID = 0;memcpy(&dwRoomID,p,4);memcpy(&dwUserID,p + 4,4);\
	dwRoomID = ntohl(dwRoomID);dwUserID = ntohl(dwUserID);id = MAKE_STREAM_IDENTITY(dwRoomID,dwUserID)
/////////////////////////////////////////////////from IAttemperEngineSink///////////////////////////////////////////// 
//����������Ϣ
bool CGatewayAttempterSink::OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent)
{
	//�������������
	if(pAcceptEvent->dwIndexID >= m_dwMaxConnectNum)
	{
		unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
		OUT_WARNEX("���ط���������˳������������%u,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u,�Ͽ��ÿͻ�������",
			m_dwMaxConnectNum,pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 
		return false;
	}
	unsigned char *pIp = (unsigned char*)&pAcceptEvent->ulIpAddr;
	OUT_DEBUGEX("���ط����������:�ͻ�������IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u,�˿�:%u",
		pAcceptEvent->dwIndexID,pAcceptEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0],pAcceptEvent->usPort); 

	m_pConnectInfo[pAcceptEvent->dwIndexID].dwRoundID = pAcceptEvent->dwRoundID;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pAcceptEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pAcceptEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//�����ȡ��Ϣ
bool CGatewayAttempterSink::OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize)
{
	switch(command.dwCmd)
	{
		//��������¼
	case SYS_LOGIN_REQ:
		return OnReqSvrLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//�ϴ�������Ϣ
	case TRANS_DATA_REQ:
		return OnReqUpMediaLoad(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//�û���¼
	case USER_LOGIN_SVR_REQ:
		return OnReqUserLogin(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//�û�������������
	case USER_ASK_FOR_SERVER_REQ:
		return OnReqSvrAllocate(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command,pData,wDataSize);
		//����
	case SYS_HEART_REQ:
		return OnActive(pRecvEvent->dwIndexID,pRecvEvent->dwRoundID,command); 
	}
	return true;
}
//����ر���Ϣ
bool CGatewayAttempterSink::OnCloseEvent(NTY_IOCloseEvent *pCloseEvent)
{
	unsigned char *pIp = (unsigned char*)&pCloseEvent->dwIpAddr;
	OUT_DEBUGEX("ý�����ˣ��ͻ������ӶϿ�id:%llu,IndexID:%u,RoundID:%u,IP:%u.%u.%u.%u",
		m_pConnectInfo[pCloseEvent->dwIndexID].udwID,pCloseEvent->dwIndexID,pCloseEvent->dwRoundID,pIp[3],pIp[2],pIp[1],pIp[0]); 

	m_pConnectInfo[pCloseEvent->dwIndexID].dwRoundID = INVALID_DWORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwID = (UNDWORD)-1;
	m_pConnectInfo[pCloseEvent->dwIndexID].wClientType = INVALID_WORD;
	m_pConnectInfo[pCloseEvent->dwIndexID].wStatus = 0;
	m_pConnectInfo[pCloseEvent->dwIndexID].udwPublishStreamID = (UNDWORD)-1;
	return true;
}
//����
bool CGatewayAttempterSink::OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command)
{
	command.dwCmd = SYS_HEART_RESP;
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,NULL,0);
}
//��������¼
bool CGatewayAttempterSink::OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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

	OUT_DEBUGEX("��������¼:ID %llu",udwID); 
	m_pConnectInfo[dwIndexID].udwID = udwID;
	m_pConnectInfo[dwIndexID].wClientType = ST_DATA_TRANS_SVR;
	m_pConnectInfo[dwIndexID].wStatus = 1;

	int nRet = 0;
	command.dwCmd = SYS_LOGIN_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4);
}
//�ͻ��˵�½
bool CGatewayAttempterSink::OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
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
	 
	int nRet = 0;
	command.dwCmd = USER_LOGIN_SVR_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,4); 
}
//��֤��¼
inline bool CGatewayAttempterSink::CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	if(m_pConnectInfo[dwIndexID].dwRoundID != dwRoundID || m_pConnectInfo[dwIndexID].wStatus == 0) 
	{
		OUT_WARNEX("����index:%u roundID:%u ����������0x%x������Ƿ�������δ��¼",dwIndexID,dwRoundID,dwCmd);
		return false;
	}
	return true;
}
//�������ϴ�����
bool CGatewayAttempterSink::OnReqUpMediaLoad(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("�յ��Ƿ���Media��������������(size >= 4),size %u",wDataSize);
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
	OUT_DEBUGEX("�յ�������(%s:%u)�ϱ�����:%u",inet_ntoa(client),wdPort,dwLoad); 

	unsigned char nRet = 0;
	command.dwCmd = TRANS_DATA_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,(char*)&nRet,1); 
}
//�û�������������
bool CGatewayAttempterSink::OnReqSvrAllocate(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize == 4);
	if(pData == NULL || wDataSize < 4) 
	{
		OUT_WARNEX("�յ��Ƿ�������������������(size >= 4),size %u",wDataSize);
		return false;
	}
	
	DWORD dwIP = 0; 
	WORD wdPort = 0;
	DWORD dwLoad = 0XFFFFFFFF;
	unsigned char ret = 0;
 
	if(m_mediaLoadMap.empty())
	{
		OUT_WARNEX("û�п��Է���ķ�����,ret=%u",ret);
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
	OUT_DEBUGEX("�յ�������������󣬷���%s,������(%s:%u),���� %u",(ret==0)?"ʧ��":"�ɹ�",inet_ntoa(client),wdPort,dwLoad); 

	command.dwCmd = USER_ASK_FOR_SERVER_RESP;
	ASSERT(m_pITcpIOAttemperEngine != NULL);
	return m_pITcpIOAttemperEngine->SendData(dwRoundID,dwIndexID,command,szBuffer,11); 
}
