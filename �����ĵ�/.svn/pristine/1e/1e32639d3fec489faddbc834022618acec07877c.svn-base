#include "Logger.h"
#include "ReadIniFile.h"
#include "GatewayAttempterSink.h"


int main()
{
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask,SIGPIPE);
	if(pthread_sigmask(SIG_BLOCK,&signal_mask,NULL) != 0)
		printf(" ============================== sigaction return -1 =======================\r\n");

	CLogHelper				m_logHelper;
	ILogService				*m_pILogService; 
	CGatewayAttempterSink		gatewaysvr;

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

	//读取配置文件
	TCHAR szConfigPath[] = TEXT("./gate.ini");
	TCHAR szApp[] = TEXT("SYSCONFIG");

	CReadIniFile readFile;
	if(readFile.SetFileName(szConfigPath) == false)
	{
		printf("读取配置文件:%s失败",szConfigPath);
		return false;
	}

	char IP[IP_LEN+1] = {0};
	strncpy(IP,"0.0.0.0",strlen("0.0.0.0"));
	DWORD dwLength = readFile.GetStringValue(szApp,TEXT("BIND_IP"),TEXT("0.0.0.0"),IP,IP_LEN);
	WORD wdPort = readFile.GetIntValue(szApp,TEXT("BIND_PORT"),-1);
	if(!dwLength || !wdPort)
	{
		printf("BIND的IP和端口错误");
		return false;
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
	
	if(gatewaysvr.InitServer(1000000,IP,wdPort) == false)
	{
		goto __Exit; 
	}
	if(gatewaysvr.StartServer(IP) == false)
	{
		goto __Exit; 
	}
	//daemon(0,0);
	while(true)
	{
		sleep(5);
	}
	gatewaysvr.StopServer();

__Exit:
	printf("gateway server exit...\n");
	return 0;
}
