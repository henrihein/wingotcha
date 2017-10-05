#pragma once
class CVideoProfile
{
public:
	CVideoProfile(void);
	~CVideoProfile(void);

	operator IWMProfile* () { return m_profile; }

protected:
	void CreateProfile();

private:
	IWMProfileManager *m_profileManager;
	IWMProfile *m_profile;
};

