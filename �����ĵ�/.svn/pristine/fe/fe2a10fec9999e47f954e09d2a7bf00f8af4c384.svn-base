////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：MediaUdpAttempterSink.h
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
#ifndef __LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
#define __LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "TransDataBuffer.h"
#include "cluster.h"
#include "StreamInfo.h"
#include <map>
using namespace std;
//定义连接信息
typedef struct tagConnectInfo
{
	bool			bIsPublish;			//播放还是上传
	WORD			wStatus;			//0未登录，1登录
	WORD			wClientType;		//取值globaldef.h:CONNECT_TYPE
	WORD			wMediaType;			//取值ENUM_MDT
	DWORD			dwRoundID;
	UNDWORD			udwID;				//客户端ID
	UNDWORD			udwPublishStreamID; //上传的流ID或者播放的流ID
}CONNECT_INFO;


//定义客户端连接结构
typedef enum enumMediaDataType
{
	EM_MEDIA_TYPE_NO				=0,			//没有
	EM_MEDIA_TYPE_AUDIO				=1,			//音频
	EM_MEDIA_TYPE_VIDEO				=2,			//视频
}ENUM_MDT;

class CMediaAttempterSink;

////////////////////////////////////////////////////////////////////////////
class CMediaUdpAttempterSink:public IAttemperEngineSink
{
private:
	CUdpNetworkHelper				m_udpNetworkHelper;
	IUdpAttemperEngine				*m_pIUdpAttemperEngine;

private:
	CONNECT_INFO					*m_pConnectInfo;			//客户端连接索引

private:
	CStreamManage					*m_pStreamManager;
	IClusterData					*m_pIClusterData;

private:
	IPPORTID						m_ipPortID;					//本机
	DWORD							m_dwMaxConnectNum;			//最大连接数

private:
	CMediaAttempterSink				*m_pMediaSvr;               //主业务
	DWORD                           m_dwStudentNum;             //学生的音频/视频路数,1路算一个负载

public:
	CMediaUdpAttempterSink(CMediaAttempterSink* pSvr);
	virtual ~CMediaUdpAttempterSink();

	//from IUnknownEx
public:
	//是否有效
	virtual bool  IsValid() {return this != NULL;}
	//释放对象
	virtual bool  Release(){if(this != NULL) delete this;return true;}
	//接口查询
	virtual void *  QueryInterface(DWORD dwQueryVer);

public:
	//初始化
	bool InitServer(CStreamManage *pStreamManage,IClusterData *pIClusterData,UNDWORD udwIpPort,DWORD dwMaxConnectNum = 500000);
	//启动
	bool StartServer(const char *pszIp);
	//停止
	bool StopServer();
	//获取发送接口
	IUdpAttemperEngine* GetIUdpAttemperEngine() { return m_pIUdpAttemperEngine;}

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
	bool OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//用户下载视频数据
	bool OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//用户下载音频数据
	bool OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);

private:
	//验证登录
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//验证数据
	inline bool CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n);

public:
	//转发数据
	bool TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize);
};
#endif //__LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
