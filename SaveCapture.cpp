#include "stdafx.h"
#include "SaveCapture.h"


CSaveCapture::CSaveCapture(void)
{
	m_location = NULL;
}

CSaveCapture::~CSaveCapture(void)
{
	if (m_location)
		CoTaskMemFree(m_location);
}

void CSaveCapture::InitPath(bool withFile)
{
	SYSTEMTIME st;

	if (S_OK == SHGetKnownFolderPath(FOLDERID_Videos, 0, NULL, &m_location))
	{
		if (withFile)
		{
			GetLocalTime(&st);
			StringCbPrintfW(m_path, sizeof(m_path), L"%s\\camcapt%02u%02u%02u.%04u.wmv", m_location, st.wYear, st.wMonth, st.wDay, st.wMinute * 80 | st.wSecond);
		}
		else
			StringCbCopyW(m_path, sizeof(m_path), m_location);
	}
}

CSaveCaptureFile::CSaveCaptureFile(void) : CSaveCapture()
{
	InitPath(true);
}

CSaveCaptureFile::~CSaveCaptureFile(void)
{
}

CSaveCaptureFolder::CSaveCaptureFolder(void) : CSaveCapture()
{
	InitPath(false);
}

CSaveCaptureFolder::~CSaveCaptureFolder(void)
{
}
