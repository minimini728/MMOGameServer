#include <iostream>
#include <windows.h>
#include "Profiler.h"
#pragma comment(lib, "winmm.lib")
#include <time.h>

//Profiler profiler;
ProfileInfo profiler[10];

void ProfileBegin(const char* tag)
{
	// 배열에 태그가 있는지 확인
	for (int i = 0; i < 10; i++)
	{
		// 있으면 갱신
		if (profiler[i].flag && profiler[i].tag != nullptr && strcmp(profiler[i].tag, tag) == 0)
		{
			if (profiler[i].inProgress)
			{
				printf("(%s) Begin End Not Matching\n", tag);
				return;
			}

			profiler[i].call++;
			LARGE_INTEGER start;
			QueryPerformanceCounter(&start);
			profiler[i].startTime = start;
			profiler[i].inProgress = true;

			return;
		}
	}

	// 비어있는 배열 인덱스 찾기
	for (int i = 0; i < 10; i++)
	{
		// 비어있는 배열 인덱스에 추가
		if (profiler[i].flag == false)
		{
			profiler[i].flag = true;
			profiler[i].tag = tag;
			profiler[i].call = 1;
			profiler[i].min = LLONG_MAX;
			LARGE_INTEGER start;
			QueryPerformanceCounter(&start);
			profiler[i].startTime = start;
			profiler[i].inProgress = true;
			return;
		}
	}

}

void ProfileEnd(const char* tag)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	// 배열에서 태그 찾기
	for (int i = 0; i < 10; i++)
	{
		if (profiler[i].flag && profiler[i].tag != nullptr && strcmp(profiler[i].tag, tag) == 0)
		{
			if (!profiler[i].inProgress)
			{
				printf("(%s)Begin End Not Matching\n", tag);
				return;
			}

			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);

			__int64 elapse = (end.QuadPart - profiler[i].startTime.QuadPart);
			profiler[i].totalTime += elapse;

			profiler[i].max = elapse > profiler[i].max ? elapse : profiler[i].max;
			profiler[i].min = elapse < profiler[i].min ? elapse : profiler[i].min;

			profiler[i].inProgress = false;
			return;
		}
	}
}

void ProfileDataOutText(const char* fileName)
{
	FILE* file;
	errno_t err = fopen_s(&file, fileName, "w");

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	if (file == NULL || err != 0)
	{
		printf("file open error!\n");
		return;
	}

	fprintf(file, "%-15s| %-15s| %-15s| %-15s| %-15s|\n", "Name", "Average", "Min", "Max", "Call");
	for (int i = 0; i < 10; i++)
	{
		if (!profiler[i].flag)
			continue;

		// 평균 계산 (마이크로초 단위)
		double average = profiler[i].call > 2
			? static_cast<double>(profiler[i].totalTime - profiler[i].min - profiler[i].max)
			/ (profiler[i].call - 2) * 1'000'000 / freq.QuadPart
			: 0.0;

		// 최대, 최소 계산 (마이크로초 단위)
		double minTime = static_cast<double>(profiler[i].min) * 1'000'000 / freq.QuadPart;
		double maxTime = static_cast<double>(profiler[i].max) * 1'000'000 / freq.QuadPart;

		// 출력
		fprintf(file, "%-15s| %-15.4f| %-15.4f| %-15.4f| %-10lld\n",
			profiler[i].tag, average, minTime, maxTime, profiler[i].call);

	}
	fclose(file);
}

void ProfileReset()
{
	for (int i = 0; i < 10; i++)
	{
		if (!profiler[i].flag)
			continue;

		profiler[i].flag = true;
		profiler[i].inProgress = false;
		profiler[i].startTime.QuadPart = 0;
		profiler[i].call = 0;
		profiler[i].totalTime = 0;
		profiler[i].max = 0;
		profiler[i].min = LLONG_MAX;
	}
}

void ProfileDataOut()
{
	time_t currentTime = time(NULL);
	struct tm localTime;
	char formatTime[100] = { 0 };

	if (localtime_s(&localTime, &currentTime) == 0)
	{
		// 문자열 조합
		snprintf(formatTime, sizeof(formatTime), "PROFILE_%04d%02d%02d_%02d%02d%02d.txt",
			localTime.tm_year + 1900,
			localTime.tm_mon + 1,
			localTime.tm_mday,
			localTime.tm_hour,
			localTime.tm_min,
			localTime.tm_sec);

		printf("포맷된 시간: %s\n", formatTime);
	}
	else
	{
		fprintf(stderr, "시간 변환에 실패했습니다.\n");
	}

	ProfileDataOutText(formatTime);
}