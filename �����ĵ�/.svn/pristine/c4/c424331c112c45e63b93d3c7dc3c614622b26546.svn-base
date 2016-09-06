////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     £∫Logger.h
// Project Code £∫»’÷æ
// Abstract     £∫
// Reference    £∫
//
// Version      £∫1.0
// Author       £∫LiuJun
// Accomplished date £∫ 07 28, 2015
// Description  : 
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#ifndef __LIUJUN_20150728_LOGGER_H
#define __LIUJUN_20150728_LOGGER_H
#include "log.h"
#include <string>
using namespace std;
////////////////////////////////////////////////////////////////////////////
class CLogger
{
private:
	ILogOutput			*m_pILogOutput;

public:
	CLogger(ILogOutput *pILogOutput);
	~CLogger();

public:
	void OutInfo(string fileName,int lineNum,string functionName,string format,...);
	void OutDebug(string fileName,int lineNum,string functionName,string format,...);
	void OutWarn(string fileName,int lineNum,string functionName,string format,...);
	void OutError(string fileName,int lineNum,string functionName,string format,...);
};
extern CLogger *g_pLogger;
#endif //__LIUJUN_20150728_LOGGER_H
