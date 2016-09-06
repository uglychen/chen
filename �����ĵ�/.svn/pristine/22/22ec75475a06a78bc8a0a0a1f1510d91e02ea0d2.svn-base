////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：TransDataBuffer.h
// Project Code ：集群数据转发时，二次封装的缓冲区
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 04 14, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#ifndef __LIUJUN_20150414_TRANS_DATA_BUFFER_H
#define __LIUJUN_20150414_TRANS_DATA_BUFFER_H
#include "globaldef.h"
#include <assert.h>
#include "ServiceThread.h"
#include <memory.h>
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
////////////////////////////////////////////////////////////////////////////
//定义转发数据时的最大缓存个数,大于网络底层的工作线程个数,最大值32个，网络底层线程默认为CPU*2
#define CLUSTER_TRANS_DATA_MEM_NUM				64
typedef struct tagTransDataBuffer
{
	WORD		wBufferSize;
	WORD		wDataSize;
	char		*pBuffer;

	tagTransDataBuffer()
	{
		wBufferSize = 0;
		wDataSize = 0;
		pBuffer = NULL;
	}
	~tagTransDataBuffer()
	{
		if(pBuffer!= NULL)
			free(pBuffer);
		pBuffer = NULL;
		wBufferSize = 0;
		wDataSize = 0;
	}
	//如果需要转字节序，请在调用前转
	void SetData(char *pData,WORD wSize)
	{
		if(wBufferSize < wDataSize + wSize)
		{ 
			wBufferSize += (wSize < 1024)?1024:wSize;
			char *pTmp = (char*)malloc(wBufferSize);
			if(pBuffer != NULL)
			{
				memcpy(pTmp,pBuffer,wDataSize);
				free(pBuffer);
			}
			pBuffer = pTmp; 
		}
		ASSERT(pBuffer != NULL);
		memcpy(pBuffer + wDataSize,pData,wSize);
		wDataSize += wSize;
	} 
	void Release()
	{
		wDataSize = 0;
	}
}TRANSDATABUFFER;
////////////////////////////////////////////////////////////////////////////
class CTransDataBuffer
{
private:
	TRANSDATABUFFER			m_transDataBuffer[CLUSTER_TRANS_DATA_MEM_NUM];
	volatile DWORD			m_dwTransDataBufferIndex;

public:
	CTransDataBuffer();
	~CTransDataBuffer();

public:
	DWORD GetBufferTotalSize();
	TRANSDATABUFFER* GetTransDataBuffer();
private:
	CThreadLock m_Lock;
};
#endif //__LIUJUN_20150414_TRANS_DATA_BUFFER_H
