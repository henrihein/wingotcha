#include "stdafx.h"
#include "resource.h"
#include "MainWnd.h"
#include "SaveCapture.h"

#define IDT_STOPCAPTURE			88
#define IDT_WHILELOCKED			89

const WCHAR CMainWindow::g_wszMainClass[] =  L"_GTDTPRMainWindow";
const DT_KEYMATCH CMainWindow::g_key = { { VK_SHIFT, VK_CONTROL, 0 }, { VK_SHIFT, 0, 0 }, { 0, 0, 0 }, { VK_MENU, 0, 0 } };
UINT	CMainWindow::g_wmTaskbarRestart = 0;

//This is for testing the performance characteristics of 
//GetTickCount vs GetSystemTime.  I prefer to use the latter,
//for accuracy, but as a keyboard logger, performance is 
//critical.  So far I have found GetSystemTime to be only
//slightly slower, but also a curiously big variability
//between systems.
void CMainWindow::TestTimers()
{
#if 0
	DWORD ticks = 0;
	FILETIME ft;
	LARGE_INTEGER pcStart, pcMid, pcEnd;
	const DWORD loopCount = 12000 * 80000;

	QueryPerformanceCounter(&pcStart);
	for (DWORD ix = 0; ix < loopCount; ix++)
		ticks = GetTickCount();
	QueryPerformanceCounter(&pcMid);
	m_logger.LogTimeDiff(L"GetTickCount", pcStart, pcMid);
	for (DWORD ix = 0; ix < loopCount; ix++)
		GetSystemTimeAsFileTime(&ft);
	QueryPerformanceCounter(&pcEnd);
	m_logger.LogTimeDiff(L"GetSystemTime", pcMid, pcEnd);
#endif
}


CMainWindow::CMainWindow(HINSTANCE hinst) : m_icon(m_logger), m_capture(g_hInstance, m_logger), m_hinst(hinst), m_hooks(m_logger)
{
    m_cRef = 0;
    m_nNumImages = 0;
	memset(&m_rcWorkarea, 0, sizeof(m_rcWorkarea));
	m_hbmMain = NULL;
	m_icoShield = NULL;
	m_racket = 0;
	m_curHand = m_curSav = NULL;
	m_defCon = 0;
	m_captureShowing = m_workstationLocked = false;
	m_timeout = 31;
}

bool CMainWindow::GetWorkArea()
{
	POINT pt = { 20, 20 };
	MONITORINFO mi;

	m_hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	memset(&m_rcWorkarea, 0, sizeof(m_rcWorkarea));
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	if (m_hMonitor)
	{
		if (GetMonitorInfo(m_hMonitor, &mi))
			m_rcWorkarea = mi.rcWork;
	}
	return (0 != m_rcWorkarea.top) || (0 != m_rcWorkarea.bottom);
}

void CMainWindow::Show()
{
	if (m_hwndMain && IsWindow(m_hwndMain))
	{
		ShowWindow(m_hwndMain, SW_SHOWNORMAL);
		SetForegroundWindow(m_hwndMain);
	}
}
void CMainWindow::Hide()
{
	if (m_hwndMain && IsWindow(m_hwndMain))
		ShowWindow(m_hwndMain, SW_HIDE);
}

ATOM CMainWindow::Register()
{
    WNDCLASSEX wcex = { 0 };

	g_wmTaskbarRestart = RegisterWindowMessage(L"TaskbarCreated");
	m_icoShield = LoadIcon(m_hinst, MAKEINTRESOURCE(IDI_DTSHIELD));
	m_curHand = LoadCursor(m_hinst, MAKEINTRESOURCE(IDC_HANDCUR));
    wcex.cbSize         = sizeof(wcex);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WindowProc;
    wcex.hInstance      = g_hInstance;
    wcex.hIcon          = m_icoShield;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = g_wszMainClass;

    return RegisterClassEx(&wcex);
}
void CMainWindow::OnStartup(HWND hwnd)
{
	m_hooks.StartWatching(hwnd);
}

int CMainWindow::DoModal()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	m_icon.Remove();
    return (int) msg.wParam;
}

LRESULT CALLBACK CMainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pcs = (LPCREATESTRUCT) lParam;
			CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(pcs->lpCreateParams);

			thisPtr->TestTimers();
			thisPtr->OnCreate(hwnd);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) thisPtr);
            break;
        }

        case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
            PostQuitMessage(0);
            break;
        }

        case WM_SETCURSOR:
        {
            break;
        }
		case WM_NCHITTEST:
		{
			LRESULT ht = DefWindowProc(hwnd, message, wParam, lParam);
			if (HTCLIENT == ht)
				return HTCAPTION;
			return ht;
		}
		case WM_ERASEBKGND:
		{
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				return thisPtr->OnErase(hwnd, wParam);
			break;
		}
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_FILE_EXIT:
				case IDM_EXIT:
                {
					CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

					if (thisPtr)
					{
						thisPtr->OnClose();
						SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
					}
                    DestroyWindow(hwnd);
                    break;
                }
                case ID_HELP_ABOUT:
				case IDM_ABOUT:
                {
                    WCHAR szAppName[DEFAULT_STRING_SIZE] = L"";

	                LoadString(g_hInstance, IDS_APP_NAME, szAppName, COUNTOF(szAppName));

                    ShellAbout(hwnd, szAppName, NULL, NULL);

                    break;
                }
				case IDM_LOCK:
				{
					if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
						thisPtr->Lock();
					break;
				}
				case IDM_FAST:
				case IDM_SLOW:
					if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
						return thisPtr->OnCommand(LOWORD(wParam), lParam);
					break;
            }
            break;
        }
		case WM_PAINT:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
			{
				if (thisPtr->DoPaint(hwnd, wParam, lParam))
					return 1;
			}
			break;
		case WM_NCLBUTTONDBLCLK:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				thisPtr->OnMouseClick(hwnd, wParam, lParam);
			return 1;
		case WM_NCACTIVATE:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				thisPtr->OnWindowActivation(wParam, lParam);
			return 1;
		case WM_TIMER:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				thisPtr->OnTimer(wParam);
			return 1;
		case WM_WTSSESSION_CHANGE:
			switch (wParam)
			{
			case WTS_SESSION_UNLOCK:
			case WTS_SESSION_LOGON:
				if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
					thisPtr->Unlock();
			}
			break;
		case WM_DTCALLBACK:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				thisPtr->OnCallback(hwnd, wParam, lParam);
			break;
		case WM_DTWATCHER:
			if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
				thisPtr->OnWatcherTick(hwnd, wParam, lParam);
			break;
    }
	if (CMainWindow::g_wmTaskbarRestart == message)
	{
		if (CMainWindow *thisPtr = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
			thisPtr->OnTaskbarCreated();
	}
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void CMainWindow::OnClose()
{
	m_hooks.Stop();
	m_capture.Close();
}
void CMainWindow::OnCreate(HWND hwnd)
{
	m_icon.Show(g_hInstance, m_hwndMain = hwnd);
	if (GetWorkArea())
	{
		RECT rcWnd = m_rcWorkarea;

		rcWnd.top += 4;
		if (rcWnd.right) rcWnd.right -= 4;
		rcWnd.bottom = rcWnd.top + 78;
		rcWnd.left = rcWnd.right - 72;
		MoveWindow(hwnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, FALSE);
	}
	m_hbmMain = LoadBitmap(m_hinst, MAKEINTRESOURCE(IDB_SHIELD));
	WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
}
void CMainWindow::OnTaskbarCreated()
{
	m_icon.Show(g_hInstance, m_hwndMain);
}

LRESULT CMainWindow::OnErase(HWND hwnd, WPARAM wParam)
{
	HDC hdc = reinterpret_cast<HDC>(wParam);

	ATLASSERT(hdc);
	if (hdc)
	{
		HBRUSH			hbr = (HBRUSH)GetStockObject(WHITE_BRUSH);
		HPEN			hpen = (HPEN)GetStockObject(WHITE_PEN);
		HGDIOBJ			hbrSav = SelectObject(hdc, hbr);
		HGDIOBJ			hpenSav = (HPEN)SelectObject(hdc, hpen);
		RECT			rc;

		if (hbrSav && hpenSav)
		{
			GetClientRect(hwnd, &rc);
			Rectangle(hdc, 0, 0, rc.right, rc.bottom);
			SelectObject(hdc, hbrSav);
			SelectObject(hdc, hpenSav);
		}
		return 1;
	}
	return 0;
}

bool CMainWindow::DoPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT psData;

	if (m_icoShield && BeginPaint(hwnd, &psData))
	{
		RECT rc;

		GetClientRect(hwnd, &rc);
		DrawIcon(psData.hdc, (rc.right - rc.left) / 2 - 12, (rc.bottom - rc.top) / 2 - 8, m_icoShield);
		EndPaint(hwnd, &psData);
		return true;
	}
	return false;
}

void CMainWindow::OnCallback(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (WM_RBUTTONUP == LOWORD(lParam))
	{
		POINT ptCur = { 0, 0 };

		SetForegroundWindow(hwnd);
		GetCursorPos(&ptCur);
		if (HMENU hmenu = LoadMenu(m_hinst, MAKEINTRESOURCE(IDR_POPUP)))
		{	
			if (HMENU hmenuSub = GetSubMenu(hmenu, 0))
			{
				TrackPopupMenu(hmenuSub, TPM_RIGHTBUTTON, ptCur.x, ptCur.y, 0, hwnd, NULL);
				PostMessage(hwnd, WM_NULL, 0, 0);
			}
		}
	}
}

void CMainWindow::OnWatcherTick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (!m_hooks.IsLocked())
	{
		//Lock if we see the requisite delay
		if (m_timeout < wParam)
			Lock();
	}
}

void CMainWindow::FitKeySlot()
{
	BYTE kbStateMap[256];
	const DT_KEYSLOT &slot = g_key[m_racket];

	if (GetKeyboardState(kbStateMap))
	{
		BYTE vk = 0;
		bool slotMatch[DT_SLOT_SIZE] = { false, false, false }, noKeyPressed = true, noKeyInSlot = true;

		for (BYTE key = VK_TAB; key < 0xA0; key++)
		{
			if (0x80 & kbStateMap[key])
			{
				noKeyPressed = false;
				vk = key;
			}
			else
				vk = 0;
			for (WORD ixVKmatch = 0; ixVKmatch < DT_SLOT_SIZE; ixVKmatch++)
			{
				if (slot[ixVKmatch])
				{
					if (slot[ixVKmatch] == vk)
					{
						slotMatch[ixVKmatch] = true;
						break;
					}
				}
			}
		}
		for (BYTE ixSlot = 0; ixSlot < DT_SLOT_SIZE; ixSlot++)
		{
			if (slot[ixSlot])
			{
				if (!slotMatch[ixSlot])
				{
					BumpDefCon();
					m_racket = 0;
					return;
				}
				noKeyInSlot = false;
			}
		}
		if (noKeyInSlot != noKeyPressed)
		{
			BumpDefCon();
			m_racket = 0;
			return;
		}
		m_racket++;
		if (DT_KEY_SIZE == m_racket)
		{
			Unlock();
			return;
		}
	}
}

void CMainWindow::OnMouseClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	FitKeySlot();
}
void CMainWindow::OnWindowActivation(WPARAM wParam, LPARAM lParam)
{
	if (m_hooks.IsLocked())
	{
		if (!wParam)
			m_defCon += 5;
	}
}

void CMainWindow::OnTimer(WPARAM idt)
{
	switch (idt)
	{
	case IDT_STOPCAPTURE:
		KillTimer(m_hwndMain, idt);
		StopCapture();
		break;
	case IDT_WHILELOCKED:
		WhileLocked();
		break;
	default:
		ATLASSERT(false);
		break;
	}
}
LRESULT CMainWindow::OnCommand(const WORD cid, const LPARAM lParam)
{
	switch (cid)
	{
	case IDM_FAST: 
		m_timeout = 19;
		return 1;
	case IDM_SLOW:
		m_timeout = 83;
		return 1;
	default:
		return 0;
	}
	return 1;
}

void CMainWindow::Lock()
{
	Show();
	m_icon.SetAttention();
	m_curSav = SetCursor(m_curHand);
	m_hooks.Lock();
	MinimizeIDEs();
	SetTimer(m_hwndMain, IDT_WHILELOCKED, 1361, NULL);
}
void CMainWindow::Unlock()
{
	m_racket = 0;
	m_defCon = 0;
	m_captureShowing = m_workstationLocked = false;
	m_icon.SetRegular();
	m_hooks.Unlock();
	if (m_curSav)
		SetCursor(m_curSav);
	m_curSav = NULL;
	m_capture.Close();
	Hide();
	KillTimer(m_hwndMain, IDT_WHILELOCKED);
}
void CMainWindow::StopCapture()
{
	DWORD hinstx = 0;
	CSaveCaptureFolder captureFolder;

	m_capture.Close();
	if (32 > (hinstx = (DWORD)ShellExecute(m_hwndMain, L"open",  captureFolder, NULL, NULL, SW_SHOW)))
		m_logger.Log(L"Failed opening video folder (%u)", hinstx);
}

void CMainWindow::BumpDefCon()
{
	m_logger.Log(L"BUMPDEFCON: Foreground is 0x%X", (DWORD)GetForegroundWindow());
	m_defCon++;
	if ((8 < m_defCon) && !m_captureShowing)
	{
		m_captureShowing = m_capture.Show();
		SetTimer(m_hwndMain, IDT_STOPCAPTURE, 6200, NULL);
	}
	if ((41 < m_defCon) && !m_workstationLocked)
	{
		m_logger.Log(L"Locking workstation because defcon\r\n");
		m_workstationLocked = LockWorkStation() ? true : false;
	}
}

#pragma warning(disable: 4706)
DWORD CALLBACK LookForIDEWindow(HWND hwnd, LPARAM lParam)
{
	if (IsWindowVisible(hwnd))
	{
		WCHAR wszProcess[MAX_PATH];
		DWORD pid = 0, tid = 0;

		tid = GetWindowThreadProcessId(hwnd, &pid);
		if (pid)
		{
			if (HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid))
			{
				memset(wszProcess, 0, sizeof(wszProcess));
				if (GetModuleFileNameEx(hProc, NULL, wszProcess, MAX_PATH))
				{
					if (WCHAR *pwchFilename = wcsrchr(wszProcess, '\\'))
					{
						if (0 == _wcsicmp(pwchFilename, L"\\devenv.exe"))
							ShowWindow(hwnd, SW_MINIMIZE);
					}
				}
				CloseHandle(hProc);
			}
		}
	}
	return TRUE;
}

void CMainWindow::MinimizeIDEs()
{
	EnumWindows((WNDENUMPROC)LookForIDEWindow, 0);
}

void CMainWindow::WhileLocked()
{
	m_logger.Log(L"Locked...");
	if (GetForegroundWindow() != m_hwndMain)
	{
		m_logger.Log(L"Bumping defcon because not foreground.");
		BumpDefCon();
	}
}


