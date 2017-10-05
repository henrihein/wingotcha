#pragma once

class CSaveCapture
{
public:
	CSaveCapture(void);
	virtual ~CSaveCapture(void);

	operator LPCWSTR () const { return m_path; }

protected:
	virtual void InitPath(bool withFile = false);

private:
	WCHAR m_path[MAX_PATH];
	PWCHAR m_location;
};

class CSaveCaptureFile : public CSaveCapture
{
public:
	CSaveCaptureFile(void);
	virtual ~CSaveCaptureFile(void);
};

class CSaveCaptureFolder : public CSaveCapture
{
public:
	CSaveCaptureFolder(void);
	virtual ~CSaveCaptureFolder(void);
};
