#include "stdafx.h"
#include "Hooks.h"
#include "Logger.h"

HHOOK g_mouseHook = NULL;
HHOOK g_kbHook = NULL;
DWORD g_lastMouseInput = 0, g_lastInput = 0;
bool  g_locked = false;
HWND  g_hwndLast = NULL;

LRESULT CALLBACK MouseListener(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    if (nCode < 0)
        return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
	g_lastMouseInput = GetTickCount();
	if (g_locked && g_hwndLast && (HC_ACTION == nCode) && lParam)
	{
		MSLLHOOKSTRUCT *pmouseData = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);

		switch (wParam)
		{
		case WM_MOUSEMOVE:
			break;
		default:
			if (g_hwndLast != WindowFromPoint(pmouseData->pt))
				return 1;
			break;
		}
	}
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam); 
} 

LRESULT CALLBACK KeyboardListener(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode < 0)
        return CallNextHookEx(g_kbHook, nCode, wParam, lParam); 
	g_lastInput = GetTickCount();
	if (g_locked)
	{
		if (HC_ACTION == nCode)
		{
			KBDLLHOOKSTRUCT *pkbData = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

			if (pkbData)
			{
				switch (pkbData->vkCode)
				{
				case VK_SHIFT:
				case VK_TAB:
				case VK_CONTROL:
				case VK_MENU:
				case VK_CAPITAL:
				case VK_NUMLOCK:
				case VK_SCROLL:
				case VK_LSHIFT:
				case VK_RSHIFT:
				case VK_LCONTROL:
				case VK_RCONTROL:
				case VK_LMENU:
				case VK_RMENU:
					break;
				default:
					return 1;
				}
			}
		}
	}
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam); 
} 

DWORD CALLBACK WatcherWorker(LPVOID lpv)
{
	CHooks *thisPtr = reinterpret_cast<CHooks*>(lpv);

	ATLASSERT(thisPtr);
	if (thisPtr)
		return thisPtr->Watch();
	return 1311;
}


CHooks::CHooks(CLogger &logger) : m_logger(logger)
{
	m_worker = NULL;
	m_signal = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hwndNotify = NULL;
}


CHooks::~CHooks(void)
{
}

void CHooks::StartWatching(HWND hwnd)
{
	DWORD dwid = 0;

	g_hwndLast = m_hwndNotify = hwnd;
	m_working = true;
	g_lastInput = GetTickCount();
	g_lastMouseInput = GetTickCount();
	ATLASSERT(NULL == m_worker);
	m_worker = CreateThread(NULL, 0, WatcherWorker, reinterpret_cast<LPVOID>(this), 0, &dwid);
	g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseListener, NULL, 0);
	g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardListener, NULL, 0);
}

void CHooks::Stop()
{
	if (g_mouseHook)
	{
		UnhookWindowsHookEx(g_mouseHook);
		g_mouseHook = NULL;
	}
	if (g_kbHook)
	{
		UnhookWindowsHookEx(g_kbHook);
		g_kbHook = NULL;
	}
	if (m_worker)
	{
		m_working = false;
		if (m_signal)
			SetEvent(m_signal);
		WaitForSingleObject(m_worker, INFINITE);
	}
}

void CHooks::Lock()
{
	g_locked = true;
}
void CHooks::Unlock()
{
	g_locked = false;
	g_lastInput = GetTickCount();
	g_lastMouseInput = GetTickCount();
}
bool CHooks::IsLocked() const
{
	return g_locked;
}

DWORD CHooks::Watch()
{
	DWORD dwWait = 6661;

	while (m_working)
	{
		if (g_locked)
			dwWait = 643;
		else
			dwWait = 6661;
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_signal, dwWait))
			return 0;
		DWORD lastInput = max(g_lastInput, g_lastMouseInput);
 		if (lastInput)
		{	
			DWORD curSec = (GetTickCount() - lastInput) / 1000;

			m_logger.Log(L"Interval passed.  Last input was %u secs.", curSec);
			PostMessage(m_hwndNotify, WM_DTWATCHER, curSec, 0);
		}
	}
	return 1;
}
