#pragma once

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_SYSTEM 2

extern int gLogLevel;
extern WCHAR gLogBuff[1024];

#define _LOG(LogLevel, fmt, ...)	\
do									\
{									\
	if (gLogLevel <= LogLevel)		\
	{								\
		swprintf(gLogBuff, 1024, fmt, ##__VA_ARGS__);		\
		CLogManager::GetInstance()->WriteLog(gLogBuff, LogLevel);	\
	}								\
									\
} while(0)							\

class CLogManager
{
private:
	static CLogManager* _LogManager;
	std::wstring _logFileName;
	CLogManager();

public:
	static CLogManager* GetInstance(void);
	static void Destroy(void);
	void InitLog(const std::wstring& fileName);
	void WriteLog(const std::wstring& string, int logLevel);
};