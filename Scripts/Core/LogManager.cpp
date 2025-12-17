#include <iostream>
#include <Windows.h>
#include "LogManager.h"

int gLogLevel = LOG_LEVEL_DEBUG;
WCHAR gLogBuff[1024];

CLogManager* CLogManager::_LogManager = nullptr;

CLogManager* CLogManager::GetInstance(void)
{
	if (_LogManager == nullptr)
	{
		_LogManager = new CLogManager;
		atexit(Destroy);
	}
	return _LogManager;
}

void CLogManager::Destroy(void)
{
	delete _LogManager;
	_LogManager = nullptr;
}

CLogManager::CLogManager()
{
	time_t currentTime = time(NULL);
	struct tm localTime;
	wchar_t formatTime[200] = { 0 };

	if (localtime_s(&localTime, &currentTime) == 0)
	{
		// 문자열 조합
		swprintf(formatTime, sizeof(formatTime), L"LOG_%04d%02d%02d_%02d%02d%02d.txt",
			localTime.tm_year + 1900,
			localTime.tm_mon + 1,
			localTime.tm_mday,
			localTime.tm_hour,
			localTime.tm_min,
			localTime.tm_sec);

		_logFileName = formatTime;

	}
	else
	{
		_logFileName = L"default_log.txt";
	}

	InitLog(_logFileName);
}


void CLogManager::InitLog(const std::wstring& fileName)
{
	_logFileName = fileName;

	// 최초 로그 시작 메시지 작성
	FILE* fp = nullptr;
	_wfopen_s(&fp, _logFileName.c_str(), L"at, ccs=UNICODE");
	if (fp)
	{
		fwprintf(fp, L"=== start logging ===\n");
		fclose(fp);
	}
}

void CLogManager::WriteLog(const std::wstring& str, int logLevel)
{
	FILE* fp = nullptr;
	_wfopen_s(&fp, _logFileName.c_str(), L"at, ccs=UNICODE");
	if (fp)
	{
		fwprintf(fp, L"[level %d] %s\n", logLevel, str.c_str());
		fclose(fp);  // 매번 닫음
	}
}