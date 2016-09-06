////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：StreamInfo.h
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
#ifndef __LIUJUN_20150728_STREAM_INFO_H
#define __LIUJUN_20150728_STREAM_INFO_H
#include "globaldef.h"
#include "ServiceThread.h"
#include "mediasvr.h"
#include <map>
#include <list>
using namespace std;

typedef map<DWORD,DWORD>							CPlayUserMap;
typedef CPlayUserMap::iterator						CPlayUserMapIt;
typedef CPlayUserMap								CPublishClientMap;
typedef CPublishClientMap::iterator					CPublishClientMapIt;

#ifndef AUDIO_HEADER_BUFFER_SIZE
#define AUDIO_HEADER_BUFFER_SIZE			128
#endif //AUDIO_HEADER_BUFFER_SIZE
#ifndef VIDEO_HEADER_BUFFER_SIZE
#define VIDEO_HEADER_BUFFER_SIZE			128
#endif //VIDEO_HEADER_BUFFER_SIZE

#define MEDIA_DATA_MAX_SECONDS_INTERNVL  2			//最后一次音视频与当前时间戳的最大间隔

//创建播放的用户列表
class CStreamItem
{
	//音频数据
private:
	bool					m_bIsPublishAudio;		//是否在直播音频
	WORD					m_wAudioStreamType;		//音频是否本地直播，为用户类型时，为本地直播
	WORD					m_wAudioHeaderSize;		//音频头大小
	DWORD					m_dwPublishAudioIndexID;//音频直播的SOCKET index，验证用户上传用
	DWORD					m_dwPublishAudioRoundID;//音频直播的SOCKET round，验证用户上传用
	CThreadLock				m_audioHeaderLock;		//音频头数据锁
	char					m_cbAudioHeader[AUDIO_HEADER_BUFFER_SIZE];	//音频头
	DWORD					m_dwLastAudioTime;		//最后一次音频时间戳
	CThreadLock				m_lastAudioTimeLock;	//锁

	//音频用户
private:
	CThreadLock				m_audioUserMapLock;		//音频房间用户锁
	CPlayUserMap			m_playAudioUserMap[USER_PLAY_LIST_NUM];		//音频房间的用户列表
	int						m_audioUserListSize[USER_PLAY_LIST_NUM];
	int						m_nCurrentAudioUserListMaxNum;	//当前最大的用户数
	int						m_nCurrentAudioUserListMaxNumIndex;
	int						m_nTotalAudioUserNum;

private:
	bool					m_bIsPublishVideo;		//是否在直播视频
	WORD					m_wVideoStreamType;		//视频是否本地直播，为用户类型时，为本地直播
	CThreadLock				m_videoHeaderLock;		//视频头数据锁
	WORD					m_wVideoHeaderSize;		//视频头大小
	char					m_cbVideoHeader[VIDEO_HEADER_BUFFER_SIZE];	//视频头
	DWORD					m_dwPublishVideoIndexID;//视频直播的SOCKET index，验证用户上传用
	DWORD					m_dwPublishVideoRoundID;//视频直播的SOCKET round，验证用户上传用
	DWORD					m_dwLastVideoTime;		//最后一次音频时间戳
	CThreadLock				m_lastVideoTimeLock;
	
	//视频用户
private:
	CThreadLock				m_videoUserMapLock;		//视频用户锁
	CPlayUserMap			m_playVideoUserMap[USER_PLAY_LIST_NUM];		//用户列表
	int						m_videoUserListSize[USER_PLAY_LIST_NUM];
	int						m_nCurrentVideoUserListMaxNum;	//当前最大的用户数
	int						m_nCurrentVideoUserListMaxNumIndex;
	int						m_nTotalVideoUserNum;

private:
	UNDWORD					m_udwStreamID;			//流ID

	//音频的客户端推送
private:
	CPublishClientMap		m_publishAudioClientMap;
	CThreadLock				m_publishAudioClientMapLock;

	//视频的客户端推送
private:
	CPublishClientMap		m_publishVideoClientMap;
	CThreadLock				m_publishVideoClientMapLock;


public:
	CStreamItem();
	~CStreamItem();

	//公共数据
public:
	//获取流ID
	UNDWORD GetStreamID() { return m_udwStreamID;}
	//重置
	void Release();

	//音频操作
public:
	//设置流信息，如果m_bIsPublish == true && （dwIndexID != m_dwPublishIndexID || dwRoundID != m_dwPublishRoundID) return false;
	bool SetPublishAudioInfo(WORD wUserType,DWORD dwIndexID,DWORD dwRoundID,UNDWORD udwStreamID);
	DWORD GetPublishAudioIndexID() { return m_dwPublishAudioIndexID;}
	DWORD GetPublishAudioRoundID() { return m_dwPublishAudioRoundID;}
	//获取发布人的类型
	WORD GetAudioResourceType() { return m_wAudioStreamType;}
	//获取状态
	bool IsPublishAudio(){return m_bIsPublishAudio;}
	//设置停止直播
	void SetStopPublishAudio() {m_bIsPublishAudio = false;}
	//设置音频头数据
	bool SetAudioHeaderData(char *pData,WORD wDataSize);
	//获取音频头
	bool GetAudioHeaderData(char *pData,WORD &wDataSize);
	//获取音频时间戳与当前时间的间隔
	DWORD GetCurrentAudioTimeInterval();
	//添加用户
	void AddPlayAudioUser(DWORD dwIndexID,DWORD dwRoundID);
	//删除用户
	int RemoveAudioUser(DWORD dwIndexID);
	//获取用户列表
	void GetPlayAudioUserList(int index,CPlayUserMap *pUserMap);
	//获取用户个数
	int GetStreamAudioUserCount() { return m_nTotalAudioUserNum;}
	//设置音频时间
	void SetAudioTime();

	//视频操作
public:
	//设置流信息，如果m_bIsPublish == true && （dwIndexID != m_dwPublishIndexID || dwRoundID != m_dwPublishRoundID) return false;
	bool SetPublishVideoInfo(WORD wUserType,DWORD dwIndexID,DWORD dwRoundID,UNDWORD udwStreamID);
	//获取发布人的索引
	DWORD GetPublishVideoIndexID() { return m_dwPublishVideoIndexID;}
	DWORD GetPublishVideoRoundID() { return m_dwPublishVideoRoundID;}
	//获取发布人的类型
	WORD GetVideoResourceType() { return m_wVideoStreamType;}
	//获取状态
	bool IsPublishVideo(){return m_bIsPublishVideo;}
	//设置停止直播
	void SetStopPublishVideo() {m_bIsPublishVideo = false;}
	//设置视频头数据
	bool SetVideoHeaderData(char *pData,WORD wDataSize);
	//获取视频头数据
	bool GetVideoHeaderData(char *pData,WORD &wDataSize);
	//获取视频时间戳与当前时间戳的间隔
	DWORD GetCurrentVideoTimeInterval();
	//添加用户
	int AddPlayVideoUser(DWORD dwIndexID,DWORD dwRoundID);
	//删除用户
	int RemoveVideoUser(DWORD dwIndexID);
	//获取用户列表
	void GetPlayVideoUserList(int index,CPlayUserMap *pUserMap);
	//获取用户个数
	int GetStreamVideoUserCount() { return m_nTotalVideoUserNum;}
	//设置音频时间
	void SetVideoTime();

public: 
	//增加发布流客户端
	void AddPublisAudioClient(DWORD dwIndexID,DWORD dwRoundID);
	//增加发布流客户端
	void AddPublisVideoClient(DWORD dwIndexID,DWORD dwRoundID);
	//删除发布流客户端
	void RemovePublishAudioClient(DWORD dwIndexID);
	//删除发布流客户端
	void RemovePublishVideoClient(DWORD dwIndexID);
	//获取发布流列表
	bool GetPublishAudioClientList(CPublishClientMap *pMap);
	//获取发布流列表
	bool GetPublishVideoClientList(CPublishClientMap *pMap);
	//获取推送流客户端个数
	int GetPublishAudioClientCount() { return (int)m_publishAudioClientMap.size();}
	//获取推送流客户端个数
	int GetPublishVideoClientCount() { return (int)m_publishVideoClientMap.size();}
	//清理音频推送连接
	void ClearAudioPublishClient();
	//清理视频推送连接
	void ClearVideoPublishClient();
};
typedef map<UNDWORD,CStreamItem*>				CStreamMap;
typedef CStreamMap::iterator					CStreamMapIt;
#define STREAM_QUEUE_NUM				16			//0xF + 1
#define STREAM_INDEX(n) n & 0xF
class CStreamManage
{
private:
	CThreadLock							m_lock[STREAM_QUEUE_NUM];		//流资源管理
	CStreamMap							m_streamMap[STREAM_QUEUE_NUM];

private:
	CThreadLock							m_freeStreamMapLock;
	CStreamMap							m_freeItemMap;	//要清理的数据

public:
	CStreamManage();
	~CStreamManage();
private:
	void AddStreamItemToFreeQueue(CStreamItem *pStream);
	 
public:
	void ClearStreamResource();

public:
	//获取Stream
	CStreamItem* GetStreamItem(UNDWORD udwStreamID);
	//创建stream
	CStreamItem* CreateSteamItem(UNDWORD udwStreamID);
	//删除stream
	void RemoveStream(UNDWORD udwStreamId);
};
#endif //__LIUJUN_20150728_STREAM_INFO_H