////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     ：TransDataBuffer.cpp
// Project Code ：集群服务端回调
// Abstract     ：
// Reference    ：
//
// Version      ：1.0
// Author       ：LiuJun
// Accomplished date ： 04 14, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#include "TransDataBuffer.h"
////////////////////////////////////////////////////////////////////////////
CTransDataBuffer::CTransDataBuffer()
{
	m_dwTransDataBufferIndex = 0;
}
CTransDataBuffer::~CTransDataBuffer()
{

}
DWORD CTransDataBuffer::GetBufferTotalSize()
{
	int nIndex = 0;
	DWORD dwTotalSize = 0;
	for(nIndex = 0; nIndex < CLUSTER_TRANS_DATA_MEM_NUM;nIndex++)
	{
		dwTotalSize += m_transDataBuffer[nIndex].wBufferSize;
	}
	return dwTotalSize;
}
TRANSDATABUFFER* CTransDataBuffer::GetTransDataBuffer()
{
	m_Lock.Lock();
	DWORD dwIndex = m_dwTransDataBufferIndex++;
	m_Lock.UnLock();
	dwIndex = dwIndex & 0x001F;
	return &m_transDataBuffer[dwIndex];
}
