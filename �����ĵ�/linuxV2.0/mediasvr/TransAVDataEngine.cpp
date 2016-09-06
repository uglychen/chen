////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：TransAVDataEngine.cpp
// Project Code ：服务端回调，主要是客户端连接（用户）
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 10 28, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "TransAVDataEngine.h"
////////////////////////////////////////////////////////////////////////////
CTransAVDataEngine::CTransAVDataEngine()
{
	sem_init(&m_semT,0,0);
	m_pTransAVDataThread = NULL;
	m_pIUdpAttemperEngine = NULL;
	m_pITcpAttemperEngine = NULL;
	m_wTransAVDataThreadNum = TRANS_AV_DATA_THREAD_NUM;
}
CTransAVDataEngine::~CTransAVDataEngine()
{
	sem_destroy(&m_semT);
	//m_avDataMem.ClearAll();
	for(CAVDataListIt it = m_freeList.begin(); it != m_freeList.end();it++)
	{
		AVDATA *pData = (*it);
		if(pData != NULL)
		{
			delete pData;
			pData = NULL;
		}
	}
	m_freeList.clear();
}
//获取内存
AVDATA* CTransAVDataEngine::GetFreeMem()
{
	AVDATA *pAVData = NULL;
	CThreadLockHandle lockHandle(&m_freeAVDataListLock);
	if(m_freeList.size() > 0)
	{
		pAVData = m_freeList.front();
		m_freeList.pop_front();
	}
	lockHandle.UnLock();
	if(pAVData == NULL) pAVData = new AVDATA();
	return pAVData;
}
//释放内存
void CTransAVDataEngine::FreeAVData(AVDATA * pAVData)
{
	ASSERT(pAVData != NULL);
	if(pAVData == NULL) return;

	pAVData->wDataSize = 0;
	pAVData->clientMap.clear();

	bool isDelete = true;
	CThreadLockHandle lockHandle(&m_freeAVDataListLock);
	if(m_freeList.size() < 2000)
	{
		m_freeList.push_back(pAVData);
		isDelete = false;
	}
	lockHandle.UnLock();
	if(isDelete == true)
	{
		delete pAVData;
		pAVData = NULL;
	}
}
//初始化
bool CTransAVDataEngine::InitTransAVDataEngine(IUdpAttemperEngine *pIUdpAttemperEngine,ITcpIOAttemperEngine *pITcpAttemperEngine,CStreamManage *pStreamManage,WORD wThreadNum)
{
	m_pIUdpAttemperEngine = pIUdpAttemperEngine;
	m_pITcpAttemperEngine = pITcpAttemperEngine;
	m_pStreamManager = pStreamManage;
	m_wTransAVDataThreadNum = (wThreadNum == 0)?TRANS_AV_DATA_THREAD_NUM:wThreadNum;
	m_pTransAVDataThread = new CTransAVDataThread[m_wTransAVDataThreadNum];
	for(int i = 0;i<m_wTransAVDataThreadNum;i++)
	{
		m_pTransAVDataThread[i].SetTransAVDataEngine(this);
	}
	return true;
}
//启动
bool CTransAVDataEngine::StartTransAVDataEngine()
{
	for(int i = 0;i<m_wTransAVDataThreadNum;i++)
	{
		m_pTransAVDataThread[i].StartTransAVDataThread();
	}
	return true;
}
//停止
bool CTransAVDataEngine::StopTransAVDataEngine()
{
	for(int i = 0;i<m_wTransAVDataThreadNum;i++)
	{
		m_pTransAVDataThread[i].SetThreadStop();
	}
	for(int i = 0;i<m_wTransAVDataThreadNum;i++)
	{
		sem_post(&m_semT);
	}
	for(int i = 0;i<m_wTransAVDataThreadNum;i++)
	{
		m_pTransAVDataThread[i].StopTransAVDataThread();
	}
	return true;
}
//放入数据
void CTransAVDataEngine::PutData(COMMAND command,char *pBuffer,WORD wDataSize,ECT clientType)
{
	ASSERT(pBuffer != NULL && wDataSize > 0);
	if(pBuffer == NULL || wDataSize == 0) return;

	ASSERT(wDataSize < MAX_TASK_BUFFER_NUM);
	if(wDataSize >= MAX_TASK_BUFFER_NUM) 
	{
		printf("-----------error 音视频数据大小非法:%u\r\n",wDataSize);
		return;
	}

	AVDATA *pAVData = GetFreeMem();
	ASSERT(pAVData != NULL);
	if(pAVData == NULL) return;

	memcpy(pAVData->cbBuffer,pBuffer,wDataSize);
	pAVData->wDataSize = wDataSize;
	pAVData->clientMap.clear();
	pAVData->clientType = clientType;
	pAVData->msgType = EM_MSG_TRANS;
	pAVData->cmd.dwCmd = command.dwCmd;
	pAVData->cmd.dwSequence = command.dwSequence;

	CThreadLockHandle lockHandle(&m_avDataListLock);
	m_avDataList.push_back(pAVData);
	lockHandle.UnLock();

	sem_post(&m_semT);
}
//放入数据
void CTransAVDataEngine::PutData(COMMAND cmd,char *pBuffer,WORD wDataSize,ECT clientType,map<DWORD,DWORD> userIndexMap)
{
	ASSERT(pBuffer != NULL && wDataSize > 0);
	if(pBuffer == NULL || wDataSize == 0) return;

	ASSERT(wDataSize < MAX_TASK_BUFFER_NUM);
	if(wDataSize >= MAX_TASK_BUFFER_NUM) 
	{
		printf("-----------error 音视频数据大小非法:%u\r\n",wDataSize);
		return;
	}

	AVDATA *pAVData = GetFreeMem();
	ASSERT(pAVData != NULL);
	if(pAVData == NULL) return;

	memcpy(pAVData->cbBuffer,pBuffer,wDataSize);
	pAVData->wDataSize = wDataSize;
	pAVData->clientMap.insert(userIndexMap.begin(),userIndexMap.end());
	pAVData->clientType = clientType;
	pAVData->msgType = EM_MSG_SEND;
	pAVData->cmd.dwCmd = cmd.dwCmd;
	pAVData->cmd.dwSequence = cmd.dwSequence;

	CThreadLockHandle lockHandle(&m_avDataListLock);
	m_avDataList.push_back(pAVData);
	lockHandle.UnLock();

	sem_post(&m_semT);
}
//获取数据
AVDATA* CTransAVDataEngine::GetAVData()
{
	AVDATA *pAVData = NULL;
	CThreadLockHandle lockHandle(&m_avDataListLock);
	if(m_avDataList.size() > 0)
	{
		pAVData = m_avDataList.front();
		m_avDataList.pop_front();
	}
	return pAVData;
}
//执行任务
bool CTransAVDataEngine::ExecTask()
{
	sem_wait(&m_semT);
	AVDATA *pAVData = NULL;
	if((pAVData = GetAVData()) == NULL)
		return true;

	if(pAVData->msgType == EM_MSG_TRANS)
	{
		CPublishClientMap clientMap;
		UNDWORD udwStreamID = 0;
		BUILD_STREAMID(pAVData->cbBuffer,udwStreamID);
		CStreamItem *pStreamItem = m_pStreamManager->GetStreamItem(udwStreamID);
		if(pStreamItem != NULL)
		{
			CPlayUserMap userMap;
			int nCount = 0,nNum = 0;
			if(pAVData->cmd.dwCmd == USER_VIDEO_DATA_REQ || pAVData->cmd.dwCmd == USER_VIDEO_HEADER_REQ)
			{
				pStreamItem->GetPublishVideoClientList(&clientMap);
				if((nNum = pStreamItem->GetStreamVideoUserCount()) > 0)
				{
					for(int i = 0;i<USER_PLAY_LIST_NUM && nCount < nNum;i++)
					{
						userMap.clear();
						pStreamItem->GetPlayVideoUserList(i,&userMap);
						//printf("--------video-----------index:%d  user:%d--------------\r\n",i,userMap.size());
						if(userMap.size() > 0)
						{
							nCount += userMap.size();
							PutData(pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize,pAVData->clientType,userMap);
						}
					} 
				}
			}
			else if(pAVData->cmd.dwCmd == USER_AUDIO_DATA_REQ || pAVData->cmd.dwCmd == USER_AUDIO_HEADER_REQ)
			{
				pStreamItem->GetPublishAudioClientList(&clientMap);
				if((nNum = pStreamItem->GetStreamAudioUserCount()) > 0)
				{
					for(int i = 0;i<USER_PLAY_LIST_NUM && nCount < nNum;i++)
					{
						userMap.clear();
						pStreamItem->GetPlayAudioUserList(i,&userMap);
						//printf("--------audio-----------index:%d  user:%d--------------\r\n",i,userMap.size());
						if(userMap.size() > 0)
						{
							nCount += userMap.size();
							PutData(pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize,pAVData->clientType,userMap);
						}
					}
				}
			}

			
			for(CPublishClientMapIt it = clientMap.begin();it != clientMap.end();it++)
			{
				m_pITcpAttemperEngine->SendData((*it).second,(*it).first,pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize);
			}
		}
		FreeAVData(pAVData);
		return true;
	}


	if(pAVData->clientType == EM_CLIENT_TYPE_UDP && m_pIUdpAttemperEngine != NULL && (pAVData->cmd.dwCmd == USER_VIDEO_HEADER_REQ || 
		pAVData->cmd.dwCmd == USER_AUDIO_HEADER_REQ || pAVData->cmd.dwCmd == USER_AUDIO_DATA_REQ))
	{
		for(map<DWORD,DWORD>::iterator it = pAVData->clientMap.begin(); it != pAVData->clientMap.end(); it++)
			m_pIUdpAttemperEngine->SendDataCtrlCmd((*it).second,(*it).first,pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize);
	}
	else if(pAVData->clientType == EM_CLIENT_TYPE_UDP && m_pIUdpAttemperEngine != NULL)
	{
		for(map<DWORD,DWORD>::iterator it = pAVData->clientMap.begin(); it != pAVData->clientMap.end(); it++)
			m_pIUdpAttemperEngine->SendData((*it).second,(*it).first,pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize);
	}
	else if(pAVData->clientType == EM_CLIENT_TYPE_TCP && m_pITcpAttemperEngine != NULL)
	{
		for(map<DWORD,DWORD>::iterator it = pAVData->clientMap.begin(); it != pAVData->clientMap.end(); it++)
			m_pITcpAttemperEngine->SendData((*it).second,(*it).first,pAVData->cmd,pAVData->cbBuffer,pAVData->wDataSize);
	}
	FreeAVData(pAVData);
	return true;
}

CTransAVDataThread::CTransAVDataThread()
{
	m_pTransAVDataEngine = NULL;
}
CTransAVDataThread::~CTransAVDataThread()
{

}
//设置
void CTransAVDataThread::SetTransAVDataEngine(CTransAVDataEngine *pTransAVDataEngine)
{
	m_pTransAVDataEngine = pTransAVDataEngine;
}
//启动
bool CTransAVDataThread::StartTransAVDataThread()
{
	return CServiceThread::StartThread();
}
//停止
bool CTransAVDataThread::StopTransAVDataThread()
{
	m_bRun = false;
	return CServiceThread::StopThread();
}
bool CTransAVDataThread::RepetitionRun()
{
	if(m_pTransAVDataEngine != NULL)
	{
		m_pTransAVDataEngine->ExecTask();
	}
	else
	{
		sleep(1);
	}
	return m_bRun;
}