////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：MediaAttempterSink.h
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
#ifndef __LIUJUN_20150728_MEDIA_ATTEMPTER_SINK_H
#define __LIUJUN_20150728_MEDIA_ATTEMPTER_SINK_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "TransDataBuffer.h"
#include "cluster.h"
#include "StreamInfo.h"
#include "MediaUdpAttempterSink.h"
#include "TransAVDataEngine.h"
#include <map>
using namespace std;
////定义连接信息
//typedef struct tagConnectInfo
//{
//	bool			bIsPublish;			//播放还是上传
//	WORD			wStatus;			//0未登录，1登录
//	WORD			wClientType;		//取值globaldef.h:CONNECT_TYPE
//	WORD			wMediaType;			//取值ENUM_MDT
//	DWORD			dwRoundID;
//	UNDWORD			udwID;				//客户端ID
//	UNDWORD			udwPublishStreamID; //上传的流ID或者播放的流ID
//}CONNECT_INFO;

#ifndef MEDIA_HEADER_BUFFER_SIZE
#define MEDIA_HEADER_BUFFER_SIZE			128
#endif //MEDIA_HEADER_BUFFER_SIZE
typedef struct tagPublishInfo
{
	WORD			wRepeatConnectionCounter;
	int				nSleepTime;	//秒
	ENUM_MDT		mediaType;
	UNDWORD			udwStreamID;
	WORD            wSrcServerType;
	WORD            wDstServerType;
	UNDWORD			udwSvrID;
	tagPublishInfo()
	{
		wRepeatConnectionCounter = 0;
		mediaType = EM_MEDIA_TYPE_NO;
		udwStreamID = 0;
		udwSvrID = 0;
	}
	void Release()
	{
		wRepeatConnectionCounter = 0;
		mediaType = EM_MEDIA_TYPE_NO;
		udwStreamID = 0;
		udwSvrID = 0;
	}

}PublishInfo;
typedef map<UNDWORD,int>							CDstSvrPublishStateMap;
typedef CDstSvrPublishStateMap::iterator			CDstSvrPublishStateMapIt;
typedef list<PublishInfo*>							CPublishInfoList;
typedef CPublishInfoList::iterator					CPublishInfoListIt;
////////////////////////////////////////////////////////////////////////////
class CMediaAttempterSink:public IAttemperEngineSink,IClientAttemperEngineSink,IClusterEvent
{
private:
	CTcpNetKernelHelper				m_tcpNetworkHelper;
	ITcpIOAttemperEngine			*m_pITcpAttemperEngine;

private:
	CONNECT_INFO					*m_pConnectInfo;			//客户端连接索引

private:
	CDstSvrPublishStateMap			m_audioDstSvrPublishStateMap;		//音频推送到目标服务器状态
	CThreadLock						m_audioDstSvrPublishStateMapLock;
	CDstSvrPublishStateMap			m_videoDstSvrPublishStateMap;		//视频推送到目标服务器状态
	CThreadLock						m_videoDstSvrPublishStateMapLock;
	CPublishInfoList				m_freePublishInfoList;
	CThreadLock						m_freePublishInfoListLock;

private:
	CStreamManage					m_streamManager;

private:
	CClusterHelper					m_clusterHelper;
	IClusterEngine					*m_pIClusterEngine;
	IClusterData					*m_pIClusterData;

private:
	IPPORTID						m_ipPortID;					//本机
	DWORD							m_dwMaxConnectNum;			//最大连接数

private:
	CTransAVDataEngine				m_transAVDataEngine;

	//UDP客户端
private:
	CMediaUdpAttempterSink			m_udpAttempterSink;			//UDP通讯

private:
	DWORD                           m_dwGatewayIndex;
	DWORD                           m_dwGatewayRound;
	DWORD                           m_dwTeacherNum;
	DWORD                           m_dwStudentNum;

public:
	CMediaAttempterSink();
	virtual ~CMediaAttempterSink();

	//from IUnknownEx
public:
	//是否有效
	virtual bool  IsValid() {return this != NULL;}
	//释放对象
	virtual bool  Release(){if(this != NULL) delete this;return true;}
	//接口查询
	virtual void *  QueryInterface(DWORD dwQueryVer);

private:
	//添加发布音频状态，如果已经存在，表明已经有连接发布该服务器的音频了，
	//不需要在重新发布,默认为true，发布成功，主要防止重复发布
	bool AddPublishAudio(UNDWORD udwSvrID);
	//删除音频发布状态
	bool RemovePublishAudio(UNDWORD udwSvrID);
	//添加发布事频状态，如果已经存在，表明已经有连接发布该服务器的视频了，
	//不需要在重新发布,默认为true，发布成功，主要防止重复发布
	bool AddPublishVideo(UNDWORD udwSvrID);
	//删除视频发布状态
	bool RemovePublishVideo(UNDWORD udwSvrID);
	//获取空闲的信息结构,没有的话new新的
	PublishInfo* GetFreePublishInfo();
	//释放推送的信息结构
	void FreePublishInfo(PublishInfo* pPublishInfo);
	//删除所有空闲的信息结构
	void ClearFreePublishInfo();

public:
	//初始化
	bool InitServer(DWORD dwMaxConnectNum = 500000);
	//启动
	bool StartServer(const char *pszIp);
	//停止
	bool StopServer();
	//清理资源
	void RecycleResource();

public:
	//集群事件通知
	virtual void ClusterEventNotify(tagClusterEvent *pClusterEvent);
	//数据转发,集群抛到上层的数据直接是用户指令，直接处理数据
	virtual void OnClusterTransDataEvent(COMMAND command,char *pData,WORD wDataSize);

	//集群数据处理
private:
	//集群转发过来的流的数据源所在的服务器位置,如果当前状态是没有推送，则请求推送
	bool OnClusterPlayAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//集群转发过来的流的数据源所在的服务器位置,如果当前状态是没有推送，则请求推送
	bool OnClusterPlayVideo(COMMAND command, char * pBuffer, WORD wDataSize);
	//集群发过来的发布视频,如果本机请求该音频的用户数大于0要求推送数据
	bool OnClusterPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//集群发过来的发布音频,如果本机请求视频的用户数大于0要求推送视频数据
	bool OnClusterPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize);
	//音频资源停止发布当老师停止上传或者同意个流资源有新的用户发布的时候都会有该消息
	bool OnClusterStopPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//视频资源停止发布当老师停止上传或者同意个流资源有新的用户发布的时候都会有该消息
	bool OnClusterStopPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize);

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
	//客户端登陆
	bool OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//服务器登录
	bool OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	//用户发布视频数据
	bool OnReqPublishVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//用户发布音频数据
	bool OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//用户下载视频数据
	bool OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//用户下载音频数据
	bool OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//视频头数据
	bool OnReqVideoHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//音频头数据
	bool OnReqAudioHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//视频数据
	bool OnReqVideoData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//音频数据
	bool OnReqAudioData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);

private:
	//验证登录
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//验证数据
	inline bool CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n);
	//验证上传权限
	inline bool CheckUploadData(DWORD dwIndexID,DWORD dwCmd,CStreamItem *pStreamItem);

private:
	//转发数据
	inline bool TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:	
	virtual bool OnClientConnectEvent(NTY_IOConnectEvent *pAcceptEvent);
	virtual bool OnClientRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize);
	virtual bool OnClientCloseEvent(NTY_IOCloseEvent *pCloseEvent);


private:
	//登录到服务器,ID UNDWORD ,TYPE WORD(类型为业务连接)
	bool ReqPublishLogin(DWORD dwIndexID,DWORD dwRoundID);
	//登录响应
	bool OnRespPublishLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam);
	//发送publish
	bool ReqPublishPublish(DWORD dwIndexID,DWORD dwRoundID,WPARAM wParam);
	//publish响应
	bool OnRespPublishPublish(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam);
public:
	//向网关同步服务器负载
	bool SendGatewayServerLoad();
private:
	//获取当前服务器负载(包括老师和学生)
	inline DWORD GetServerLoadNum() const {return m_dwTeacherNum + m_dwStudentNum;}
};
#endif //__LIUJUN_20150728_MEDIA_ATTEMPTER_SINK_H
