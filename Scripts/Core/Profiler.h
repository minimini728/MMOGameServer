#pragma once
struct ProfileInfo
{
	bool flag = false;
	bool inProgress = false;
	const char* tag = nullptr;
	LARGE_INTEGER startTime = {};
	__int64 call = 0;
	__int64 totalTime = 0;
	__int64 max = 0;
	__int64 min = 0;
};


void ProfileBegin(const char* tag);
void ProfileEnd(const char* tag);
void ProfileDataOutText(const char* fileName);
void ProfileReset();
void ProfileDataOut();