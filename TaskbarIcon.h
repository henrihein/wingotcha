#pragma once

#define WM_DTCALLBACK WM_APP + 0x1000

class CLogger;

class TaskbarIcon
{
public:
	TaskbarIcon(CLogger &logger);
	virtual ~TaskbarIcon(void);

	void Show(HINSTANCE hinst, HWND hwnd);
	void Remove();

	void SetRegular();
	void SetAttention();

private:
	CLogger &m_logger;
	NOTIFYICONDATA m_data;
	HINSTANCE m_hinst;
	HICON m_icoReg;
	HICON m_icoAttn;
};

