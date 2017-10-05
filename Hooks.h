#pragma once

class CLogger;

#define WM_DTWATCHER WM_APP + 0x3000

class CHooks
{
public:
	CHooks(CLogger &logger);
	virtual ~CHooks(void);

	void StartWatching(HWND hwnd);
	void Stop();

	void Lock();
	void Unlock();
	bool IsLocked() const;
	DWORD Watch();

private:
	CHooks();
	HWND m_hwndNotify;
	CLogger &m_logger;
	HANDLE m_worker;
	HANDLE m_hlistener;
	bool  m_working;
	HANDLE m_signal;
};

