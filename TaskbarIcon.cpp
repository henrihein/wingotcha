#include "StdAfx.h"
#include "TaskbarIcon.h"
#include "Logger.h"
#include "resource.h"

TaskbarIcon::TaskbarIcon(CLogger &logger) : m_logger(logger)
{
	memset(&m_data, 0, sizeof(m_data));
	m_data.cbSize = sizeof(m_data);
	m_data.uID = 0x41;
	m_icoReg = m_icoAttn = NULL;
}


TaskbarIcon::~TaskbarIcon(void)
{
}

void TaskbarIcon::Show(HINSTANCE hinst, HWND hwnd)
{
	m_hinst = hinst;
	m_icoReg = LoadIcon(m_hinst, MAKEINTRESOURCE(IDI_MAIN));
	m_icoAttn = LoadIcon(m_hinst, MAKEINTRESOURCE(IDI_DTSHIELD));
	m_data.hWnd = hwnd;
	m_data.hIcon = m_icoReg;
	m_data.uFlags = NIF_ICON | NIF_MESSAGE;
	m_data.uCallbackMessage = WM_DTCALLBACK;
	Shell_NotifyIcon(NIM_ADD, &m_data);
}

void TaskbarIcon::Remove()
{
	m_data.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE, &m_data);
}

void TaskbarIcon::SetRegular()
{
	m_data.uFlags = NIF_ICON;
	m_data.hIcon = m_icoReg;
	Shell_NotifyIcon(NIM_MODIFY, &m_data);
}
void TaskbarIcon::SetAttention()
{
	m_data.uFlags = NIF_ICON;
	m_data.hIcon = m_icoAttn;
	Shell_NotifyIcon(NIM_MODIFY, &m_data);
}

