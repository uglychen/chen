////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：StreamInfo.cpp
// Project Code ：流对象，存储流ID和play该流的用户列表
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 07 28, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "StreamInfo.h"
#include <sys/time.h>

////////////////////////////////////////////////////////////////////////////
CStreamItem::CStreamItem()
{
	Release();
}
CStreamItem::~CStreamItem()
{
	Release();
	m_publishAudioClientMap.clear();
	m_publishVideoClientMap.clear();
}
//重置
void CStreamItem::Release()
{
	//音频数据重置
	m_bIsPublishAudio = false;				//是否在直播音频
	m_wAudioStreamType = INVALID_WORD;		//音频是否本地直播，为用户类型时，为本地直播
	m_wAudioHeaderSize = 0;					//音频头大小
	m_dwPublishAudioIndexID = INVALID_DWORD;//音频直播的SOCKET index，验证用户上传用
	m_dwPublishAudioRoundID = INVALID_DWORD;//音频直播的SOCKET round，验证用户上传用
	//m_playAudioUserMap.clear();				//音频房间的用户列表
	m_dwLastAudioTime = 0;
	memset(m_audioUserListSize,0,sizeof(int) * USER_PLAY_LIST_NUM);
	m_nCurrentAudioUserListMaxNum = 0;
	m_nCurrentAudioUserListMaxNumIndex = 0;
	m_nTotalAudioUserNum = 0;

	//视频数据重置
	m_bIsPublishVideo = false;				//是否在直播视频
	m_wVideoStreamType = INVALID_WORD;		//视频是否本地直播，为用户类型时，为本地直播
	m_wVideoHeaderSize = 0;					//视频头大小
	m_dwPublishVideoIndexID = INVALID_DWORD;//视频直播的SOCKET index，验证用户上传用
	m_dwPublishVideoRoundID = INVALID_DWORD;//视频直播的SOCKET round，验证用户上传用
	//m_playVideoUserMap.clear();
	m_dwLastVideoTime = 0;
	memset(m_videoUserListSize,0,sizeof(int) * USER_PLAY_LIST_NUM);
	m_nCurrentVideoUserListMaxNum = 0;
	m_nCurrentVideoUserListMaxNumIndex = 0;
	m_nTotalVideoUserNum = 0;
	//公共数据
	m_udwStreamID = (UNDWORD)-1;			//流ID
} 
//设置流信息，如果m_bIsPublish == true && (dwIndexID != m_dwPublishIndexID || dwRoundID != m_dwPublishRoundID) return false;
bool CStreamItem::SetPublishAudioInfo(WORD wUserType,DWORD dwIndexID,DWORD dwRoundID,UNDWORD udwStreamID)
{
	//已经有用户上传数据了，但是重复了提交命令
	ASSERT(m_udwStreamID == (UNDWORD)-1);
	ASSERT(m_udwStreamID != udwStreamID);

	if(m_bIsPublishAudio == true)
		return false;
	
	CThreadLockHandle lockHandle(&m_lastAudioTimeLock);
	m_dwLastAudioTime = GetTickCount();
	lockHandle.UnLock();

	m_bIsPublishAudio = true;
	m_wAudioStreamType = wUserType;
	m_dwPublishAudioIndexID = dwIndexID;
	m_dwPublishAudioRoundID = dwRoundID;
	m_udwStreamID = udwStreamID;
	return true;
}
//设置音频头数据
bool CStreamItem::SetAudioHeaderData(char *pData,WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize > 0 && wDataSize < AUDIO_HEADER_BUFFER_SIZE);
	if(pData == NULL) return false;
	if(wDataSize >= AUDIO_HEADER_BUFFER_SIZE) return false;

	CThreadLockHandle lockHandle(&m_audioHeaderLock);
	memcpy(m_cbAudioHeader,pData,wDataSize);
	m_wAudioHeaderSize = wDataSize;
	return true;
}
//获取音频头
bool CStreamItem::GetAudioHeaderData(char *pData,WORD &wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize < AUDIO_HEADER_BUFFER_SIZE);
	
	if(m_wAudioHeaderSize == 0) return false;

	CThreadLockHandle lockHandle(&m_audioHeaderLock);
	memcpy(pData,m_cbAudioHeader,m_wAudioHeaderSize);
	wDataSize = m_wAudioHeaderSize;
	return true;
}
//获取音频时间戳与当前时间的间隔
DWORD CStreamItem::GetCurrentAudioTimeInterval()
{
	CThreadLockHandle lockHandle(&m_lastAudioTimeLock);
	DWORD dwAudioTime = m_dwLastAudioTime;
	lockHandle.UnLock();
	return GetTickCount() - dwAudioTime;
}
//添加用户
void CStreamItem::AddPlayAudioUser(DWORD dwIndexID,DWORD dwRoundID)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	ASSERT(dwRoundID != INVALID_WORD);

	CThreadLockHandle lockHandle(&m_audioUserMapLock);
	m_nTotalAudioUserNum++;
	for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
	{
		if(m_audioUserListSize[i] < 10)
		{
			(m_playAudioUserMap[i])[dwIndexID] = dwRoundID;
			m_audioUserListSize[i] = m_playAudioUserMap[i].size();
			if(m_nCurrentAudioUserListMaxNum < m_audioUserListSize[i])
			{
				m_nCurrentAudioUserListMaxNum = m_audioUserListSize[i];
				m_nCurrentAudioUserListMaxNumIndex = i;
			}
			break;
		}
		else if(m_audioUserListSize[i] < m_nCurrentAudioUserListMaxNum)
		{
			(m_playAudioUserMap[i])[dwIndexID] = dwRoundID;
			m_audioUserListSize[i] = m_playAudioUserMap[i].size();
			if(m_nCurrentAudioUserListMaxNum < m_audioUserListSize[i])
			{
				m_nCurrentAudioUserListMaxNum = m_audioUserListSize[i];
				m_nCurrentAudioUserListMaxNumIndex = i;
			}
			break;
		}		
		else if(m_audioUserListSize[i] == m_nCurrentAudioUserListMaxNum && i != m_nCurrentAudioUserListMaxNumIndex)
		{
			(m_playAudioUserMap[i])[dwIndexID] = dwRoundID;
			m_audioUserListSize[i] = m_playAudioUserMap[i].size();
			if(m_nCurrentAudioUserListMaxNum < m_audioUserListSize[i])
			{
				m_nCurrentAudioUserListMaxNum = m_audioUserListSize[i];
				m_nCurrentAudioUserListMaxNumIndex = i;
			}
			break;
		}
		else if(m_audioUserListSize[i] > m_nCurrentAudioUserListMaxNum)
		{
			(m_playAudioUserMap[i])[dwIndexID] = dwRoundID;
			m_audioUserListSize[i] = m_playAudioUserMap[i].size();
			if(m_nCurrentAudioUserListMaxNum < m_audioUserListSize[i])
			{
				m_nCurrentAudioUserListMaxNum = m_audioUserListSize[i];
				m_nCurrentAudioUserListMaxNumIndex = i;
			}
			break;
		}
	}
	//m_playAudioUserMap[dwIndexID] = dwRoundID;
	//return m_playAudioUserMap.size();
}
//删除用户
int CStreamItem::RemoveAudioUser(DWORD dwIndexID)
{
	CThreadLockHandle lockHandle(&m_audioUserMapLock);
	m_nTotalAudioUserNum--;
	for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
	{
		if(m_playAudioUserMap[i].find(dwIndexID) != m_playAudioUserMap[i].end())
		{
			m_playAudioUserMap[i].erase(dwIndexID);
			m_audioUserListSize[i] = m_playAudioUserMap[i].size();
			m_nCurrentAudioUserListMaxNum--;
		}
	}
	//m_playAudioUserMap.erase(dwIndexID); 
	//nCount = m_playAudioUserMap.size();
	lockHandle.UnLock();
	return m_nTotalAudioUserNum;
}
//设置音频时间
void CStreamItem::SetAudioTime()
{
	CThreadLockHandle lockHandle1(&m_lastAudioTimeLock);
	m_dwLastAudioTime = GetTickCount();
	lockHandle1.UnLock();
}
//获取用户列表
void CStreamItem::GetPlayAudioUserList(int index,CPlayUserMap *pUserMap)
{
	ASSERT(index > -1 && index < USER_PLAY_LIST_NUM);
	if(index < 0 || index >= USER_PLAY_LIST_NUM) return;

	CThreadLockHandle lockHandle(&m_audioUserMapLock);
	if(m_playAudioUserMap[index].size() > 0)
		pUserMap->insert(m_playAudioUserMap[index].begin(),m_playAudioUserMap[index].end());
	lockHandle.UnLock();
}

//设置流信息，如果m_bIsPublish == true && (dwIndexID != m_dwPublishIndexID || dwRoundID != m_dwPublishRoundID) return false;
bool CStreamItem::SetPublishVideoInfo(WORD wUserType,DWORD dwIndexID,DWORD dwRoundID,UNDWORD udwStreamID)
{
	//已经有用户上传数据了，但是重复了提交命令
	ASSERT(m_udwStreamID == (UNDWORD)-1);
	ASSERT(m_udwStreamID != udwStreamID);

	if(m_bIsPublishVideo == true)
		return false;
	
	CThreadLockHandle lockHandle(&m_lastVideoTimeLock);
	m_dwLastVideoTime = GetTickCount();
	lockHandle.UnLock();

	m_bIsPublishVideo = true;
	m_wVideoStreamType = wUserType;
	m_dwPublishVideoIndexID = dwIndexID;
	m_dwPublishVideoRoundID = dwRoundID;
	m_udwStreamID = udwStreamID;
	return true;
}
//设置视频头数据
bool CStreamItem::SetVideoHeaderData(char *pData,WORD wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize > 0 && wDataSize < VIDEO_HEADER_BUFFER_SIZE);
	if(pData == NULL) return false;
	if(wDataSize < 1 || wDataSize > VIDEO_HEADER_BUFFER_SIZE) return false;

	CThreadLockHandle lockHandle(&m_videoHeaderLock);
	memcpy(m_cbVideoHeader,pData,wDataSize);
	m_wVideoHeaderSize = wDataSize;
	lockHandle.UnLock(); 

	CThreadLockHandle lockHandle1(&m_lastVideoTimeLock);
	m_dwLastVideoTime = GetTickCount();
	lockHandle1.UnLock();
	return true;
}
//获取视频头数据
bool CStreamItem::GetVideoHeaderData(char *pData,WORD &wDataSize)
{
	ASSERT(pData != NULL);
	ASSERT(wDataSize >= VIDEO_HEADER_BUFFER_SIZE);
	
	if(m_wVideoHeaderSize == 0) return false;

	CThreadLockHandle lockHandle(&m_videoHeaderLock);
	memcpy(pData,m_cbVideoHeader,m_wVideoHeaderSize);
	wDataSize = m_wVideoHeaderSize;
	return true;
}
//获取视频时间戳与当前时间戳的间隔
DWORD CStreamItem::GetCurrentVideoTimeInterval()
{
	CThreadLockHandle lockHandle(&m_lastVideoTimeLock);
	DWORD dwVideoTime = m_dwLastVideoTime;
	lockHandle.UnLock();
	return GetTickCount() - dwVideoTime;
}
//添加用户
int CStreamItem::AddPlayVideoUser(DWORD dwIndexID,DWORD dwRoundID)
{
	ASSERT(dwIndexID != INVALID_DWORD);
	ASSERT(dwRoundID != INVALID_WORD);

	CThreadLockHandle lockHandle(&m_videoUserMapLock);
	m_nTotalVideoUserNum++;
	for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
	{
		if(m_videoUserListSize[i] < 10)
		{
			(m_playVideoUserMap[i])[dwIndexID] = dwRoundID;
			m_videoUserListSize[i] = m_playVideoUserMap[i].size();
			if(m_nCurrentVideoUserListMaxNum < m_videoUserListSize[i])
			{
				m_nCurrentVideoUserListMaxNum = m_videoUserListSize[i];
				m_nCurrentVideoUserListMaxNumIndex = i;
			}
			break;
		}
		else if(m_videoUserListSize[i] < m_nCurrentVideoUserListMaxNum)
		{
			(m_playVideoUserMap[i])[dwIndexID] = dwRoundID;
			m_videoUserListSize[i] = m_playVideoUserMap[i].size();
			if(m_nCurrentVideoUserListMaxNum < m_videoUserListSize[i])
			{
				m_nCurrentVideoUserListMaxNum = m_videoUserListSize[i];
				m_nCurrentVideoUserListMaxNumIndex = i;
			}
			break;
		}		
		else if(m_videoUserListSize[i] == m_nCurrentVideoUserListMaxNum && i != m_nCurrentVideoUserListMaxNumIndex)
		{
			(m_playVideoUserMap[i])[dwIndexID] = dwRoundID;
			m_videoUserListSize[i] = m_playVideoUserMap[i].size();
			if(m_nCurrentVideoUserListMaxNum < m_videoUserListSize[i])
			{
				m_nCurrentVideoUserListMaxNum = m_videoUserListSize[i];
				m_nCurrentVideoUserListMaxNumIndex = i;
			}
			break;
		}
		else if(m_videoUserListSize[i] > m_nCurrentVideoUserListMaxNum)
		{
			(m_playVideoUserMap[i])[dwIndexID] = dwRoundID;
			m_videoUserListSize[i] = m_playVideoUserMap[i].size();
			if(m_nCurrentVideoUserListMaxNum < m_videoUserListSize[i])
			{
				m_nCurrentVideoUserListMaxNum = m_videoUserListSize[i];
				m_nCurrentVideoUserListMaxNumIndex = i;
			}
			break;
		}
	}
	//m_playVideoUserMap[dwIndexID] = dwRoundID;
	//return m_playVideoUserMap.size();
	return m_nTotalVideoUserNum;
}

//删除用户
int CStreamItem::RemoveVideoUser(DWORD dwIndexID)
{
	int nCount = 0;
	CThreadLockHandle lockHandleVideo(&m_videoUserMapLock);
	m_nTotalVideoUserNum--;
	for(int i = 0;i<USER_PLAY_LIST_NUM;i++)
	{
		if(m_playVideoUserMap[i].find(dwIndexID) != m_playVideoUserMap[i].end())
		{
			m_playVideoUserMap[i].erase(dwIndexID);
			m_videoUserListSize[i] = m_playVideoUserMap[i].size();
			m_nCurrentVideoUserListMaxNum--;
			break;
		}
	}
	//m_playVideoUserMap.erase(dwIndexID);
	//nCount = m_playVideoUserMap.size();
	lockHandleVideo.UnLock();
	return nCount;
}
//设置音频时间
void CStreamItem::SetVideoTime()
{
	CThreadLockHandle lockHandle1(&m_lastVideoTimeLock);
	m_dwLastVideoTime = GetTickCount();
	lockHandle1.UnLock();
}
//获取用户列表
void CStreamItem::GetPlayVideoUserList(int index,CPlayUserMap *pUserMap)
{
	CThreadLockHandle lockHandle(&m_videoUserMapLock);
	pUserMap->insert(m_playVideoUserMap[index].begin(),m_playVideoUserMap[index].end());
	lockHandle.UnLock();
}
//增加发布流客户端
void CStreamItem::AddPublisAudioClient(DWORD dwIndexID,DWORD dwRoundID)
{
	CThreadLockHandle lockHandle(&m_publishAudioClientMapLock);
	m_publishAudioClientMap[dwIndexID] = dwRoundID;
}
//增加发布流客户端
void CStreamItem::AddPublisVideoClient(DWORD dwIndexID,DWORD dwRoundID)
{
	CThreadLockHandle lockHandle(&m_publishVideoClientMapLock);
	m_publishVideoClientMap[dwIndexID] = dwRoundID;
}
//删除发布流客户端
void CStreamItem::RemovePublishAudioClient(DWORD dwIndexID)
{
	CThreadLockHandle lockHandle(&m_publishAudioClientMapLock);
	m_publishAudioClientMap.erase(dwIndexID);
}
//删除发布流客户端
void CStreamItem::RemovePublishVideoClient(DWORD dwIndexID)
{
	CThreadLockHandle lockHandle(&m_publishVideoClientMapLock);
	m_publishVideoClientMap.erase(dwIndexID);
}
//获取发布流列表
bool CStreamItem::GetPublishAudioClientList(CPublishClientMap *pMap)
{
	ASSERT(pMap != NULL);
	if(pMap == NULL) return false;

	CThreadLockHandle lockHandle(&m_publishAudioClientMapLock);
	pMap->insert(m_publishAudioClientMap.begin(),m_publishAudioClientMap.end());
	return pMap->size();
}
//获取发布流列表
bool CStreamItem::GetPublishVideoClientList(CPublishClientMap *pMap)
{
	ASSERT(pMap != NULL);
	if(pMap == NULL) return false;

	CThreadLockHandle lockHandle(&m_publishVideoClientMapLock);
	pMap->insert(m_publishVideoClientMap.begin(),m_publishVideoClientMap.end());
	return pMap->size();
}
//清理音频推送连接
void CStreamItem::ClearAudioPublishClient()
{
	CThreadLockHandle lockHandle(&m_publishAudioClientMapLock);
	m_publishAudioClientMap.clear();
}
//清理视频推送连接
void CStreamItem::ClearVideoPublishClient()
{
	CThreadLockHandle lockHandle(&m_publishVideoClientMapLock);
	m_publishVideoClientMap.clear();
}
////////////////////////////////////////////////////////////////////////////

CStreamManage::CStreamManage()
{

}
CStreamManage::~CStreamManage()
{
	for(int i = 0;i<STREAM_QUEUE_NUM;i++)
	{
		for(CStreamMapIt it = m_streamMap[i].begin(); it != m_streamMap[i].end();it++)
		{
			if((*it).second != NULL)
				delete (*it).second;
		}
		m_streamMap[i].clear();
	}
	CStreamItem *pStream = NULL;
	for(CStreamMapIt it = m_freeItemMap.begin();it != m_freeItemMap.end();it++)
	{
		pStream = (*it).second;
		delete pStream;
	}
	m_freeItemMap.clear();
}
void CStreamManage::AddStreamItemToFreeQueue(CStreamItem *pStream)
{
	ASSERT(pStream != NULL);
	if(pStream == NULL) return;

	timeval tv;
	gettimeofday(&tv,NULL);
	UNDWORD udwTime = (((UNDWORD)tv.tv_sec) * 1000 * 1000) + ((UNDWORD)tv.tv_usec); //微妙
	m_freeStreamMapLock.Lock();
	m_freeItemMap[udwTime] = pStream;
	m_freeStreamMapLock.UnLock();
} 
void CStreamManage::ClearStreamResource()
{
	timeval tv;
	gettimeofday(&tv,NULL);
	UNDWORD udwTime = (((UNDWORD)tv.tv_sec) * 1000 * 1000) + ((UNDWORD)tv.tv_usec); //微妙
	CStreamItem *pStream = NULL;
	m_freeStreamMapLock.Lock();
	if(m_freeItemMap.size() > 0)
	{
		CStreamMapIt it = m_freeItemMap.begin();
		if((udwTime - (*it).first) > 30000000) //大于30秒
		{
			pStream = (*it).second;
			m_freeItemMap.erase(it);
		}
	}
	m_freeStreamMapLock.UnLock();

	if(pStream != NULL)
	{
		delete pStream;
		pStream = NULL;
	}
} 
//获取Stream
CStreamItem* CStreamManage::GetStreamItem(UNDWORD udwStreamID)
{
	WORD wIndex = STREAM_INDEX(udwStreamID); 
	CStreamItem *pSteamItem = NULL;
	CThreadLockHandle lockHandle(&m_lock[wIndex]);
	CStreamMapIt it = m_streamMap[wIndex].find(udwStreamID);
	if(it != m_streamMap[wIndex].end())
		pSteamItem = (*it).second;
	return pSteamItem;
}
//创建stream
CStreamItem* CStreamManage::CreateSteamItem(UNDWORD udwStreamID)
{ 
	WORD wIndex = STREAM_INDEX(udwStreamID); 
	CStreamItem *pSteamItem = NULL;

	CThreadLockHandle lockHandle(&m_lock[wIndex]);
	CStreamMapIt it = m_streamMap[wIndex].find(udwStreamID);
	if(it != m_streamMap[wIndex].end())
	{
		pSteamItem = (*it).second;
		return pSteamItem;
	}
	 
	if(pSteamItem == NULL) pSteamItem = new CStreamItem();
	ASSERT(pSteamItem != NULL);
	 
	(m_streamMap[wIndex])[udwStreamID] = pSteamItem;

	return pSteamItem;
}
//删除stream
void CStreamManage::RemoveStream(UNDWORD udwStreamId)
{
	WORD wIndex = STREAM_INDEX(udwStreamId); 
	CStreamItem *pSteamItem = NULL;

	CThreadLockHandle lockHandle(&m_lock[wIndex]);
	CStreamMapIt it = m_streamMap[wIndex].find(udwStreamId);
	if(it != m_streamMap[wIndex].end())
	{
		pSteamItem = (*it).second; 
		m_streamMap[wIndex].erase(it);
	} 
	lockHandle.UnLock();

	if(pSteamItem != NULL) AddStreamItemToFreeQueue(pSteamItem);
}