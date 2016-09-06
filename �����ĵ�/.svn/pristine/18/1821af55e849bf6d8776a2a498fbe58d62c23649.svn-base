////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,LiuJun
// All rights reserved.
//
// Filename     £∫Logger.cpp
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
#include "Logger.h"
#include <stdarg.h>
using namespace std;
////////////////////////////////////////////////////////////////////////////
#   define COL(x)  "\033[;" #x "m"  
#   define RED     COL(31)  
#   define GREEN   COL(32)  
#   define YELLOW  COL(33)  
#   define BLUE    COL(34)  
#   define MAGENTA COL(35)  
#   define CYAN    COL(36)  
#   define WHITE   COL(0)  
#   define GRAY    "\033[0m"  
////////////////////////////////////////////////////////////////////////////
CLogger *g_pLogger = NULL;
CLogger::CLogger(ILogOutput *pILogOutput)
{
	m_pILogOutput = pILogOutput;
	g_pLogger = this;
}
CLogger::~CLogger()
{

}
void CLogger::OutInfo(string fileName,int lineNum,string functionName,string format,...)
{
	char msg[2048];
	va_list argList;
	va_start(argList,format);
	vsnprintf(msg,2048,format.c_str(),argList);
	va_end(argList);

	//printf(GREEN"%s : %u : %s  \n"GRAY,fileName,lineNum,functionName,msg);
	printf(WHITE"%s : %u : %s  \n"GRAY,fileName.c_str(),lineNum,msg);

	SYSTEMTIME st;
	GetLocalTime(&st);
	TCHAR szTime[24];
	sprintf(szTime,"%d-%02d-%02d %02d:%02d:%02d ",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	string strLog(szTime);
	strLog.append(msg);
	if(m_pILogOutput != NULL)
		m_pILogOutput->OutputMsg(LOG_INFO,strLog.c_str());
}
void CLogger::OutDebug(string fileName,int lineNum,string functionName,string format,...)
{
	char msg[2048];
	va_list argList;
	va_start(argList,format);
	vsnprintf(msg,2048,format.c_str(),argList);
	va_end(argList);

	//printf(BLUE"%s : %u : %s  \n"GRAY,fileName,lineNum,functionName,msg);
	printf(CYAN"%s : %u : %s  \n"GRAY,fileName.c_str(),lineNum,msg);

	SYSTEMTIME st;
	GetLocalTime(&st);
	TCHAR szTime[24];
	sprintf(szTime,"%d-%02d-%02d %02d:%02d:%02d ",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	string strLog(szTime);
	strLog.append(msg);
	if(m_pILogOutput != NULL)
		m_pILogOutput->OutputMsg(LOG_DEBUG,strLog.c_str());
}
void CLogger::OutWarn(string fileName,int lineNum,string functionName,string format,...)
{
	char msg[2048];
	va_list argList;
	va_start(argList,format);
	vsnprintf(msg,2048,format.c_str(),argList);
	va_end(argList);

	//printf(YELLOW"%s : %u : %s  \n"GRAY,fileName,lineNum,functionName,msg);
	printf(YELLOW"%s : %u : %s  \n"GRAY,fileName.c_str(),lineNum,msg);

	SYSTEMTIME st;
	GetLocalTime(&st);
	TCHAR szTime[24];
	sprintf(szTime,"%d-%02d-%02d %02d:%02d:%02d ",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	string strLog(szTime);
	strLog.append(msg);
	if(m_pILogOutput != NULL)
		m_pILogOutput->OutputMsg(LOG_WARN,strLog.c_str());

}
void CLogger::OutError(string fileName,int lineNum,string functionName,string format,...)
{
	char msg[2048];
	va_list argList;
	va_start(argList,format);
	vsnprintf(msg,2048,format.c_str(),argList);
	va_end(argList);

	//printf(RED"%s : %u : %s  \n"GRAY,fileName,lineNum,functionName,msg);
	printf(RED"%s : %u : %s  \n"GRAY,fileName.c_str(),lineNum,msg);

	
	SYSTEMTIME st;
	GetLocalTime(&st);
	TCHAR szTime[24];
	sprintf(szTime,"%d-%02d-%02d %02d:%02d:%02d ",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	string strLog(szTime);
	strLog.append(msg);
	if(m_pILogOutput != NULL)
		m_pILogOutput->OutputMsg(LOG_ERROR,strLog.c_str());

}