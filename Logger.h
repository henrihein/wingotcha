#pragma once
class CLogger
{
public:
	CLogger(void);
	virtual ~CLogger(void);

	void Log(LPCWSTR text);
	void Log(LPCWSTR text, LPCWSTR more);
	void Log(LPCWSTR text, DWORD par);
	void LogHR(LPCWSTR text, HRESULT hr);

	void LogTime(LPCWSTR text, const LARGE_INTEGER &timeCount);
	void LogTimeDiff(LPCWSTR text, const LARGE_INTEGER &timeStart, const LARGE_INTEGER &timeEnd);

};

