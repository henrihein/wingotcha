#include "stdafx.h"
#include "VideoProfile.h"


CVideoProfile::CVideoProfile(void) : m_profileManager(NULL), m_profile(NULL)
{
	WMCreateProfileManager(&m_profileManager);
	CreateProfile();
}


CVideoProfile::~CVideoProfile(void)
{
	if (m_profile)
		m_profile->Release();
	if (m_profileManager)
		m_profileManager->Release();
}

void CVideoProfile::CreateProfile()
{
	if (m_profileManager)
		m_profileManager->LoadProfileByID(WMProfile_V80_56VideoOnly, &m_profile);
}

