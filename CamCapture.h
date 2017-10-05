class CLogger;

class CCamCapture
{
public:
	CCamCapture(HINSTANCE hinst, CLogger &logger);
	virtual ~CCamCapture();

	HRESULT GetInterfaces(void);
	HRESULT CaptureVideo();
	HRESULT FindCaptureDevice(IBaseFilter ** ppSrcFilter);
	HRESULT SetupVideoWindow(void);
	HRESULT ChangePreviewState(int nShow);
	HRESULT HandleGraphEvent(void);

	bool Show();
	void Close();

	bool CaptureWndProc(UINT message, WPARAM wParam, LPARAM lParam);

protected:
	void InitClass();
	void Msg(WCHAR *szFormat, ...);
	void CloseInterfaces(void);
	void ResizeVideoWindow(void);
	void DrawQ();

private:
	CCamCapture();
	enum PLAYSTATE {Stopped, Paused, Running, Init};
	const HINSTANCE	m_hinst;
	ATOM			m_clas;
	HWND			m_hwnd;
	IVideoWindow  * m_pVW;
	IMediaControl * m_pMC;
	IMediaEventEx * m_pME;
	IGraphBuilder * m_pGraph;
	IBaseFilter		*m_fileFilter;
	ICaptureGraphBuilder2 * m_pCapture;
	DWORD m_dwGraphRegister;
	PLAYSTATE m_psCurrent;
	CLogger &m_logger;
	bool m_hasDevice;
};

//
// Macros
//
#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

#define JIF(x) if (FAILED(hr=(x))) \
    {Msg(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n\0"), hr); return hr;}

//
// Constants
//
#define DEFAULT_VIDEO_WIDTH     320
#define DEFAULT_VIDEO_HEIGHT    320


// Application-defined message to notify app of filtergraph events
#define WM_GRAPHNOTIFY  WM_APP+0x1040

