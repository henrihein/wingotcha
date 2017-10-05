#ifndef __MAINWND__
#define __MAINWND__

#include "CamCapture.h"
#include "TaskbarIcon.h"
#include "Logger.h"
#include "Hooks.h"

#define DT_SLOT_SIZE 3
#define DT_KEY_SIZE  4
typedef BYTE DT_KEYSLOT[DT_SLOT_SIZE];
typedef DT_KEYSLOT DT_KEYMATCH[DT_KEY_SIZE];

class CMainWindow
{
public:
    CMainWindow(HINSTANCE hinst);

    ATOM Register();
	void OnStartup(HWND hwnd);

    int DoModal();

	//Events
	void OnClose();
	void OnCreate(HWND hwnd);
	void OnTaskbarCreated();
	LRESULT OnErase(HWND hwnd, WPARAM wParam);
	bool DoPaint(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnCallback(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnWatcherTick(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnMouseClick(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnWindowActivation(WPARAM wParam, LPARAM lParam);
	void OnTimer(WPARAM idt);
	LRESULT OnCommand(const WORD cid, const LPARAM lParam);

	static const WCHAR g_wszMainClass[];
	static const DT_KEYMATCH g_key;
	static UINT	g_wmTaskbarRestart;

protected:
    static LRESULT CALLBACK WindowProc(HWND   hWnd, UINT   uMsg, WPARAM wParam, LPARAM lParam);
	void TestTimers();
	bool GetWorkArea();
	void FitKeySlot();
	void Show();
	void Hide();
	void Lock();
	void Unlock();
	void StopCapture();
	void BumpDefCon();
	void MinimizeIDEs();
	void WhileLocked();

private:
	CMainWindow();
    LONG  m_cRef;
    LONG  m_nNumImages;
	TaskbarIcon m_icon;
	const HINSTANCE m_hinst;
	HWND m_hwndMain;
	CLogger m_logger;
	CCamCapture m_capture;
	bool	m_captureShowing, m_workstationLocked;
	RECT m_rcWorkarea;
	HBITMAP m_hbmMain;
	HICON   m_icoShield;
	HCURSOR m_curHand, m_curSav;
	CHooks  m_hooks;
	WORD	m_racket;
	WORD	m_defCon;
	HMONITOR m_hMonitor;
	WPARAM	m_timeout;
};

#endif //__MAINWND__

