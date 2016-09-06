////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ��MediaAttempterSink.h
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
////����������Ϣ
//typedef struct tagConnectInfo
//{
//	bool			bIsPublish;			//���Ż����ϴ�
//	WORD			wStatus;			//0δ��¼��1��¼
//	WORD			wClientType;		//ȡֵglobaldef.h:CONNECT_TYPE
//	WORD			wMediaType;			//ȡֵENUM_MDT
//	DWORD			dwRoundID;
//	UNDWORD			udwID;				//�ͻ���ID
//	UNDWORD			udwPublishStreamID; //�ϴ�����ID���߲��ŵ���ID
//}CONNECT_INFO;

#ifndef MEDIA_HEADER_BUFFER_SIZE
#define MEDIA_HEADER_BUFFER_SIZE			128
#endif //MEDIA_HEADER_BUFFER_SIZE
typedef struct tagPublishInfo
{
	WORD			wRepeatConnectionCounter;
	int				nSleepTime;	//��
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
	CONNECT_INFO					*m_pConnectInfo;			//�ͻ�����������

private:
	CDstSvrPublishStateMap			m_audioDstSvrPublishStateMap;		//��Ƶ���͵�Ŀ�������״̬
	CThreadLock						m_audioDstSvrPublishStateMapLock;
	CDstSvrPublishStateMap			m_videoDstSvrPublishStateMap;		//��Ƶ���͵�Ŀ�������״̬
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
	IPPORTID						m_ipPortID;					//����
	DWORD							m_dwMaxConnectNum;			//���������

private:
	CTransAVDataEngine				m_transAVDataEngine;

	//UDP�ͻ���
private:
	CMediaUdpAttempterSink			m_udpAttempterSink;			//UDPͨѶ

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
	//�Ƿ���Ч
	virtual bool  IsValid() {return this != NULL;}
	//�ͷŶ���
	virtual bool  Release(){if(this != NULL) delete this;return true;}
	//�ӿڲ�ѯ
	virtual void *  QueryInterface(DWORD dwQueryVer);

private:
	//��ӷ�����Ƶ״̬������Ѿ����ڣ������Ѿ������ӷ����÷���������Ƶ�ˣ�
	//����Ҫ�����·���,Ĭ��Ϊtrue�������ɹ�����Ҫ��ֹ�ظ�����
	bool AddPublishAudio(UNDWORD udwSvrID);
	//ɾ����Ƶ����״̬
	bool RemovePublishAudio(UNDWORD udwSvrID);
	//��ӷ�����Ƶ״̬������Ѿ����ڣ������Ѿ������ӷ����÷���������Ƶ�ˣ�
	//����Ҫ�����·���,Ĭ��Ϊtrue�������ɹ�����Ҫ��ֹ�ظ�����
	bool AddPublishVideo(UNDWORD udwSvrID);
	//ɾ����Ƶ����״̬
	bool RemovePublishVideo(UNDWORD udwSvrID);
	//��ȡ���е���Ϣ�ṹ,û�еĻ�new�µ�
	PublishInfo* GetFreePublishInfo();
	//�ͷ����͵���Ϣ�ṹ
	void FreePublishInfo(PublishInfo* pPublishInfo);
	//ɾ�����п��е���Ϣ�ṹ
	void ClearFreePublishInfo();

public:
	//��ʼ��
	bool InitServer(DWORD dwMaxConnectNum = 500000);
	//����
	bool StartServer(const char *pszIp);
	//ֹͣ
	bool StopServer();
	//������Դ
	void RecycleResource();

public:
	//��Ⱥ�¼�֪ͨ
	virtual void ClusterEventNotify(tagClusterEvent *pClusterEvent);
	//����ת��,��Ⱥ�׵��ϲ������ֱ�����û�ָ�ֱ�Ӵ�������
	virtual void OnClusterTransDataEvent(COMMAND command,char *pData,WORD wDataSize);

	//��Ⱥ���ݴ���
private:
	//��Ⱥת����������������Դ���ڵķ�����λ��,�����ǰ״̬��û�����ͣ�����������
	bool OnClusterPlayAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//��Ⱥת����������������Դ���ڵķ�����λ��,�����ǰ״̬��û�����ͣ�����������
	bool OnClusterPlayVideo(COMMAND command, char * pBuffer, WORD wDataSize);
	//��Ⱥ�������ķ�����Ƶ,��������������Ƶ���û�������0Ҫ����������
	bool OnClusterPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//��Ⱥ�������ķ�����Ƶ,�������������Ƶ���û�������0Ҫ��������Ƶ����
	bool OnClusterPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize);
	//��Ƶ��Դֹͣ��������ʦֹͣ�ϴ�����ͬ�������Դ���µ��û�������ʱ�򶼻��и���Ϣ
	bool OnClusterStopPublishAudio(COMMAND command, char * pBuffer, WORD wDataSize);
	//��Ƶ��Դֹͣ��������ʦֹͣ�ϴ�����ͬ�������Դ���µ��û�������ʱ�򶼻��и���Ϣ
	bool OnClusterStopPublishVideo(COMMAND command, char * pBuffer, WORD wDataSize);

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
	//��������¼
	bool OnReqSvrLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize); 
	//�û�������Ƶ����
	bool OnReqPublishVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//�û�������Ƶ����
	bool OnReqPublishAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//�û�������Ƶ����
	bool OnReqPlayVideo(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//�û�������Ƶ����
	bool OnReqPlayAudio(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//��Ƶͷ����
	bool OnReqVideoHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//��Ƶͷ����
	bool OnReqAudioHeaderData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//��Ƶ����
	bool OnReqVideoData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);
	//��Ƶ����
	bool OnReqAudioData(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char* pData, WORD wDataSize);

private:
	//��֤��¼
	inline bool CheckClientIsLogin(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd);
	//��֤����
	inline bool CheckClientData(DWORD dwIndexID,DWORD dwRoundID,DWORD dwCmd,char* pData, WORD wDataSize,WORD n);
	//��֤�ϴ�Ȩ��
	inline bool CheckUploadData(DWORD dwIndexID,DWORD dwCmd,CStreamItem *pStreamItem);

private:
	//ת������
	inline bool TransData(CStreamItem *pStreamItem,COMMAND command, char* pData, WORD wDataSize);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:	
	virtual bool OnClientConnectEvent(NTY_IOConnectEvent *pAcceptEvent);
	virtual bool OnClientRecvEvent(NTY_IORecvEvent *pRecvEvent, COMMAND command, char* pData, WORD wDataSize);
	virtual bool OnClientCloseEvent(NTY_IOCloseEvent *pCloseEvent);


private:
	//��¼��������,ID UNDWORD ,TYPE WORD(����Ϊҵ������)
	bool ReqPublishLogin(DWORD dwIndexID,DWORD dwRoundID);
	//��¼��Ӧ
	bool OnRespPublishLogin(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam);
	//����publish
	bool ReqPublishPublish(DWORD dwIndexID,DWORD dwRoundID,WPARAM wParam);
	//publish��Ӧ
	bool OnRespPublishPublish(DWORD dwIndexID,DWORD dwRoundID,COMMAND command, char * pBuffer, WORD wDataSize,WPARAM wParam);
public:
	//������ͬ������������
	bool SendGatewayServerLoad();
private:
	//��ȡ��ǰ����������(������ʦ��ѧ��)
	inline DWORD GetServerLoadNum() const {return m_dwTeacherNum + m_dwStudentNum;}
};
#endif //__LIUJUN_20150728_MEDIA_ATTEMPTER_SINK_H
