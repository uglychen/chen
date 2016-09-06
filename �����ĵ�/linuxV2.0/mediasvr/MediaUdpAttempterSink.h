////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��MediaUdpAttempterSink.h
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
#ifndef __LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
#define __LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
#include "netkernel.h"
#include "ServiceThread.h"
#include "TransDataBuffer.h"
#include "cluster.h"
#include "StreamInfo.h"
#include <map>
using namespace std;
//����������Ϣ
typedef struct tagConnectInfo
{
	bool			bIsPublish;			//���Ż����ϴ�
	WORD			wStatus;			//0δ��¼��1��¼
	WORD			wClientType;		//ȡֵglobaldef.h:CONNECT_TYPE
	WORD			wMediaType;			//ȡֵENUM_MDT
	DWORD			dwRoundID;
	UNDWORD			udwID;				//�ͻ���ID
	UNDWORD			udwPublishStreamID; //�ϴ�����ID���߲��ŵ���ID
}CONNECT_INFO;


//����ͻ������ӽṹ
typedef enum enumMediaDataType
{
	EM_MEDIA_TYPE_NO				=0,			//û��
	EM_MEDIA_TYPE_AUDIO				=1,			//��Ƶ
	EM_MEDIA_TYPE_VIDEO				=2,			//��Ƶ
}ENUM_MDT;

class CMediaAttempterSink;

////////////////////////////////////////////////////////////////////////////
class CMediaUdpAttempterSink:public IAttemperEngineSink
{
private:
	CUdpNetworkHelper				m_udpNetworkHelper;
	IUdpAttemperEngine				*m_pIUdpAttemperEngine;

private:
	CONNECT_INFO					*m_pConnectInfo;			//�ͻ�����������

private:
	CStreamManage					*m_pStreamManager;
	IClusterData					*m_pIClusterData;

private:
	IPPORTID						m_ipPortID;					//����
	DWORD							m_dwMaxConnectNum;			//���������

private:
	CMediaAttempterSink				*m_pMediaSvr;               //��ҵ��
	DWORD                           m_dwStudentNum;             //ѧ������Ƶ/��Ƶ·��,1·��һ������

public:
	CMediaUdpAttempterSink(CMediaAttempterSink* pSvr);
	virtual ~CMediaUdpAttempterSink();

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
	bool InitServer(CStreamManage *pStreamManage,IClusterData *pIClusterData,UNDWORD udwIpPort,DWORD dwMaxConnectNum = 500000);
	//����
	bool StartServer(const char *pszIp);
	//ֹͣ
	bool StopServer();
	//��ȡ���ͽӿ�
	IUdpAttemperEngine* GetIUdpAttemperEngine() { return m_pIUdpAttemperEngine;}

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
	//�ͻ��˵�½
	bool OnReqUserLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	bool OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//�û�������Ƶ����
	bool OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//�û�������Ƶ����
	bool OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);

private:
	//��֤��¼
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//��֤����
	inline bool CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n);

public:
	//ת������
	bool TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize);
};
#endif //__LIUJUN_20150922_MEDIA_UDP_ATTEMPTER_SINK_H
