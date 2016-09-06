#include "Logger.h"
#include "mediasvr.h"
#include "MediaAttempterSink.h"
#include <stdio.h>
#   define COL(x)  "\033[;" #x "m"  
#   define RED     COL(31)  
#   define GREEN   COL(32)  
#   define YELLOW  COL(33)  
#   define BLUE    COL(34)  
#   define MAGENTA COL(35)  
#   define CYAN    COL(36)  
#   define WHITE   COL(0)  
#   define GRAY    "\033[0m"
int main(int argc, char* argv[])
{
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask,SIGPIPE);
	if(pthread_sigmask(SIG_BLOCK,&signal_mask,NULL) != 0)
		printf(" ============================== sigaction return -1 =======================\r\n");

	CLogHelper				m_logHelper;
	ILogService				*m_pILogService; 
	CMediaAttempterSink		mediasvr;

	if(argc < 2)
	{
		printf(RED"!!!!!!!!!!!!!!!!! 没有输入UDP服务的绑定IP!!!!!!!!!!\r\n ");
	}

	//初始化日志
	if(m_logHelper.CreateInstance() == false)
	{
		printf("创建日志组件失败:%s\r\n",m_logHelper.GetErrorMessage());
		return 0;
	}
	m_pILogService = m_logHelper.GetInterface();
	ILogOutput *pILogOutput = (ILogOutput*)m_pILogService->QueryInterface(VER_ILogOutput);
	if(pILogOutput == NULL)
	{
		printf("获取日志输出接口错误,程序退出\r\n");
		return 0;
	}
	CLogger log(pILogOutput);

	if(m_pILogService->IniLogSvr("./log/",LOG_INFO|LOG_WARN|LOG_DEBUG|LOG_ERROR) == false)
	{
		 printf("初始化日志组件失败\r\n");
		 goto __Exit; 
	}
	 printf("初始化日志完成\r\n");
	if(m_pILogService->StartLogSvr() == false)
	{
		 printf("启动日志组件失败\r\n");
		 goto __Exit; 
	}
	 printf("启动化日志完成\r\n");
	if(mediasvr.InitServer(1000000) == false)
	{
		goto __Exit; 
	}
	if(argc < 2)
	{
		if(mediasvr.StartServer("0.0.0.0") == false)
		{
			goto __Exit; 
		}
	}
	else
	{
		if(mediasvr.StartServer(argv[1]) == false)
		{
			goto __Exit; 
		}
	}
	
	while(true)
	{
		sleep(1);
		//清理资源
		mediasvr.RecycleResource();
	}
	mediasvr.StopServer();

__Exit:
	printf("media server exit...\n");
	return 0;
}
