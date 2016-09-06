////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��GatewayAttempterSink.h
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
#ifndef __LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
#define __LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "cluster.h"
#include <map>
using namespace std;

////////////////////////////////////////////////////////////////////////////
//����������Ϣ
typedef struct tagConnectInfo
{
	WORD			wStatus;			//0δ��¼��1��¼
	WORD			wClientType;		//ȡֵglobaldef.h:CONNECT_TYPE
	DWORD			dwRoundID;
	UNDWORD			udwID;				//�ͻ���ID
	UNDWORD			udwPublishStreamID; //�ϴ�����ID
}CONNECT_INFO;


////�����ѡ��ý���������Ϣ
typedef struct tagMediaInfo
{
	tagMediaInfo()
		:port(0),load(0)
	{
		memset(ip,0,sizeof(ip));
	}
	char ip[IP_LEN+1];
	WORD port;
	DWORD load;
}MEDIA_SERVER_INFO;

////////////////////////////////////////////////////////////////////////////
class CGatewayAttempterSink:public IAttemperEngineSink
{
private:
	CTcpNetKernelHelper					m_tcpNetworkHelper;
	ITcpIOAttemperEngine				*m_pITcpIOAttemperEngine;

private:
	CONNECT_INFO					*m_pConnectInfo;			//�ͻ�����������

private:
	IPPORTID						m_ipPortID;					//����
	DWORD							m_dwMaxConnectNum;			//���������

	//Media������Ϣ
private:
	std::map<DWORD,MEDIA_SERVER_INFO> m_mediaLoadMap;

public:
	CGatewayAttempterSink();
	virtual ~CGatewayAttempterSink();

	//from IUnknownEx
public:
	//�Ƿ���Ч
	virtual bool  IsValid() {return this != NULL;}
	//�ͷŶ���
	virtual bool  Release(){if(this != NULL) delete this;return true;}
	//�ӿڲ�ѯ
	virtual void *  QueryInterface(DWORD dwQueryVer);

public:
	//��ʼ��
	bool InitServer(DWORD dwMaxConnectNum, char* ip, WORD wdPort);
	//����
	bool StartServer(char* IP);
	//ֹͣ
	bool StopServer();

	//from IAttemperEngineSink
public:
	//����������Ϣ
	virtual bool OnConnectEvent(NTY_IOConnectEvent *pAcceptEvent);
	//�����ȡ��Ϣ
	virtual bool OnRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize);
	//����ر���Ϣ
	virtual bool OnCloseEvent(NTY_IOCloseEvent *pCloseEvent);
	 
private:
	//����
	bool OnActive(DWORD dwIndexID,DWORD dwRoundID,COMMAND command);
	//��֤��¼
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//�ͻ��˵�½
	bool OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//��������¼
	bool OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	//������������Ϣ
	bool OnReqUpMediaLoad(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	bool OnReqSvrAllocate(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
};
#endif //__LIUJUN_20150728_GATEWAY_ATTEMPTER_SINK_H
