////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：TransAVDataEngine.h
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
#ifndef __LIUJUN_20151028_TRANS_AV_DATA_ENGINE_H
#define __LIUJUN_20151028_TRANS_AV_DATA_ENGINE_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "StreamInfo.h"
#include <list>
#include <map>
////////////////////////////////////////////////////////////////////////////
typedef enum numClientType
{
	EM_CLIENT_TYPE_TCP		=0x1,		//TCP客户端连接
	EM_CLIENT_TYPE_UDP		=0x2,		//UDP连接客户端
}ECT;
typedef enum numMsgType
{
	EM_MSG_TRANS			=0x1,		//需要分解消息转发
	EM_MSG_SEND				=0x2,		//消息已经分解直接转发
}EMSGTYPE;
#define MAX_TASK_BUFFER_NUM			2800
typedef struct tagAVData
{
	EMSGTYPE			msgType;				//消息类型
	ECT					clientType;				//客户端是TCP还是UDP连接
	map<DWORD,DWORD>	clientMap;
	COMMAND				cmd;
	WORD				wDataSize;				//数据大小
	char				cbBuffer[MAX_TASK_BUFFER_NUM];			//数据
}AVDATA;

typedef list<AVDATA*>					CAVDataList;
typedef list<AVDATA*>::iterator			CAVDataListIt;
////////////////////////////////////////////////////////////////////////////
#define TRANS_AV_DATA_THREAD_NUM				(32)
class CTransAVDataThread;
class CTransAVDataEngine
{
private:
	CThreadLock							m_avDataListLock;
	CAVDataList							m_avDataList;

private:
	CThreadLock							m_freeAVDataListLock;
	CAVDataList							m_freeList;

private:
	sem_t								m_semT;
	CTransAVDataThread					*m_pTransAVDataThread;		//
	WORD								m_wTransAVDataThreadNum;
	CStreamManage						*m_pStreamManager;

private:
	IUdpAttemperEngine					*m_pIUdpAttemperEngine;
	ITcpIOAttemperEngine				*m_pITcpAttemperEngine;

public:
	CTransAVDataEngine();
	~CTransAVDataEngine();

public:
	//获取内存
	AVDATA* GetFreeMem();
	//释放内存
	void FreeAVData(AVDATA * pAVData);

public:
	//初始化
	bool InitTransAVDataEngine(IUdpAttemperEngine *pIUdpAttemperEngine,ITcpIOAttemperEngine *pITcpAttemperEngine,CStreamManage *pStreamManage,WORD wThreadNum);
	//启动
	bool StartTransAVDataEngine();
	//停止
	bool StopTransAVDataEngine();

public:
	//放入数据
	void PutData(COMMAND command,char *pBuffer,WORD wDataSize,ECT clientType);

private:
	//放入数据
	void PutData(COMMAND cmd,char *pBuffer,WORD wDataSize,ECT clientType,map<DWORD,DWORD> userIndexMap);
	//获取数据
	AVDATA* GetAVData();

public:
	//执行任务
	bool ExecTask();
};

//执行线程
class CTransAVDataThread:public CServiceThread
{
private:
	CTransAVDataEngine		*m_pTransAVDataEngine;

public:
	CTransAVDataThread();
	virtual ~CTransAVDataThread();

public:
	//设置
	void SetTransAVDataEngine(CTransAVDataEngine *pTransAVDataEngine);
	//启动
	bool StartTransAVDataThread();
	//设置停止
	void SetThreadStop() { m_bRun = false;}
	//停止
	bool StopTransAVDataThread();

protected:
	virtual bool RepetitionRun();
};
#endif //__LIUJUN_20151028_TRANS_AV_DATA_ENGINE_H