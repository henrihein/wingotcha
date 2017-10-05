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

	StringCbPrintfW(outText, sizeof(outText), L"%s TIMER %u.%u", text, timeCount.HighPart, timeCount.LowPart);
	Log(outText);
}

void CLogger::LogTimeDiff(LPCWSTR text, const LARGE_INTEGER &timeStart, const LARGE_INTEGER &timeEnd)
{
	LARGE_INTEGER timeDiff;

	timeDiff.QuadPart = timeEnd.QuadPart - timeStart.QuadPart;
	LogTime(text, timeDiff);
}

