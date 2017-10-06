#include "stdafx.h"
#include "Logger.h"


CLogger::CLogger(void)
{
}

CLogger::~CLogger(void)
{
}

void CLogger::Log(LPCWSTR text)
{
	OutputDebugString(text);
	OutputDebugString(L"\r\n");
}

void CLogger::Log(LPCWSTR text, LPCWSTR more)
{
	WCHAR outText[480];

	StringCbPrintfW(outText, sizeof(outText), text, more);
	Log(outText);
}
void CLogger::LogHR(LPCWSTR text, HRESULT hr)
{
	WCHAR outText[480];

	StringCbPrintfW(outText, sizeof(outText), L"%s HRESULT: 0x%X\r\n", text, hr);
	Log(outText);
}

void CLogger::Log(LPCWSTR text, DWORD par)
{
	WCHAR outText[480];

	StringCbPrintfW(outText, sizeof(outText), text, par);
	Log(outText);
}

void CLogger::LogTime(LPCWSTR text, const LARGE_INTEGER &timeCount)
{
	WCHAR outText[400];
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);
	const LONGLONG cfreq		= freq.QuadPart;
	const LONGLONG cft			= timeCount.QuadPart;
	const LONGLONG cMilliSecs	= (cft / (cfreq / 1000));
	const LONGLONG cSecsTotal	= cft / cfreq;
	const LONGLONG cHours		= cSecsTotal / (60 * 60);
	const LONGLONG cMins		= (cSecsTotal - cHours * 60 * 60) / 60;
	const LONGLONG cSecsDisp	= (cSecsTotal - cHours * 60 * 60 - cMins * 60);
	StringCbPrintfW(outText, sizeof(outText), L"%s TIMER %02u:%02u:%02u:%04u", text, cHours, cMins, cSecsDisp, cMilliSecs - cSecsTotal * 1000);
	Log(outText);
}

void CLogger::LogTimeDiff(LPCWSTR text, const LARGE_INTEGER &timeStart, const LARGE_INTEGER &timeEnd)
{
	LARGE_INTEGER timeDiff;

	timeDiff.QuadPart = timeEnd.QuadPart - timeStart.QuadPart;
	LogTime(text, timeDiff);
}

