#include "StdAfx.h"
#include "CamCapture.h"
#include "SaveCapture.h"
#include "Logger.h"
#include "VideoProfile.h"

#define CLASSNAME       L"_CamCaptWnd"

CCamCapture::CCamCapture(HINSTANCE hinst, CLogger &logger) : m_logger(logger), m_hinst(hinst)
{
	m_pVW = NULL;
	m_pMC = NULL;
	m_pME = NULL;
	m_pGraph = NULL;
	m_pCapture = NULL;
	m_fileFilter = NULL;
	m_psCurrent = Stopped;
	m_dwGraphRegister=0;
	m_hwnd = NULL;
	m_clas = NULL;
	m_hasDevice = false;
	InitClass();
}

CCamCapture::~CCamCapture()
{
	CloseInterfaces();
}

HRESULT CCamCapture::CaptureVideo()
{
    HRESULT hr;
    IBaseFilter *pSrcFilter=NULL;
	CSaveCaptureFile wstrFilename;
	CVideoProfile profile;

    // Get DirectShow interfaces
    hr = GetInterfaces();
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Failed to get video interfaces", hr);
        return hr;
    }

    // Attach the filter graph to the capture graph
    hr = m_pCapture->SetFiltergraph(m_pGraph);
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Failed to set capture filter graph", hr);
        return hr;
    }

    // Use the system device enumerator and class enumerator to find
    // a video capture/preview device, such as a desktop USB video camera.
    hr = FindCaptureDevice(&pSrcFilter);
    if (FAILED(hr))
    {
        // Don't display a message because FindCaptureDevice will handle it
        return hr;
    }
   
    // Add Capture filter to our graph.
    hr = m_pGraph->AddFilter(pSrcFilter, L"Video Capture");
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't add the capture filter to the graph", hr);
        pSrcFilter->Release();
        return hr;
    }
    // Render the preview pin on the video capture filter
    // Use this instead of m_pGraph->RenderFile
    hr = m_pCapture->RenderStream (&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
                                   pSrcFilter, NULL, NULL);
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't render the video capture stream.", hr);
        pSrcFilter->Release();
        return hr;
    }
	m_logger.Log(L"Trying to set filename: %s", wstrFilename);
	hr = m_pCapture->SetOutputFileName(&MEDIASUBTYPE_Asf, wstrFilename, &m_fileFilter, NULL);
	if (FAILED(hr))
		m_logger.LogHR(L"Failed to set output file", hr);
	if (m_fileFilter)
	{
		IConfigAsfWriter *pConfigFilter = 0;

		if (SUCCEEDED(hr = m_fileFilter->QueryInterface(IID_IConfigAsfWriter, (void**)&pConfigFilter)))
		{
			pConfigFilter->ConfigureFilterUsingProfile(profile);
			pConfigFilter->Release();
		}
	}
	//Render to the file
    hr = m_pCapture->RenderStream (&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                   pSrcFilter, NULL, m_fileFilter);
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't capture the video capture stream.", hr);
        pSrcFilter->Release();
        return hr;
    }
    // Now that the filter has been added to the graph and we have
    // rendered its stream, we can release this reference to the filter.
    pSrcFilter->Release();

    // Set video window style and position
    hr = SetupVideoWindow();
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't initialize video window", hr);
        return hr;
    }

    // Start previewing video data
    hr = m_pMC->Run();
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't run the graph!", hr);
        return hr;
    }

    // Remember current state
    m_psCurrent = Running;
	m_hasDevice = true;
    return S_OK;
}


HRESULT CCamCapture::FindCaptureDevice(IBaseFilter ** ppSrcFilter)
{
    HRESULT hr = S_OK;
    IBaseFilter * pSrc = NULL;
    IMoniker* pMoniker =NULL;
    ICreateDevEnum *pDevEnum =NULL;
    IEnumMoniker *pClassEnum = NULL;

    if (!ppSrcFilter)
	{
        return E_POINTER;
	}
   
    // Create the system device enumerator
    hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **) &pDevEnum);
    if (FAILED(hr))
    {
        m_logger.LogHR(L"Couldn't create system enumerator!", hr);
    }

    // Create an enumerator for the video capture devices

	if (SUCCEEDED(hr))
	{
	    hr = pDevEnum->CreateClassEnumerator (CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			m_logger.LogHR(L"Couldn't create class enumerator", hr);
	    }
	}

	if (SUCCEEDED(hr))
	{
		// If there are no enumerators for the requested type, then 
		// CreateClassEnumerator will succeed, but pClassEnum will be NULL.
		if (pClassEnum == NULL)
		{
			hr = E_FAIL;
		}
	}

    // Use the first video capture device on the device list.
    // Note that if the Next() call succeeds but there are no monikers,
    // it will return S_FALSE (which is not a failure).  Therefore, we
    // check that the return code is S_OK instead of using SUCCEEDED() macro.

	if (SUCCEEDED(hr))
	{
		hr = pClassEnum->Next (1, &pMoniker, NULL);
		if (hr == S_FALSE)
		{
	        m_logger.Log(L"Unable to access video capture device!");
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
    {
        // Bind Moniker to a filter object
        hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
        if (FAILED(hr))
        {
            m_logger.LogHR(L"Couldn't bind moniker to filter object", hr);
        }
    }

    // Copy the found filter pointer to the output parameter.
	if (SUCCEEDED(hr))
	{
	    *ppSrcFilter = pSrc;
		(*ppSrcFilter)->AddRef();
	}

	SAFE_RELEASE(pSrc);
    SAFE_RELEASE(pMoniker);
    SAFE_RELEASE(pDevEnum);
    SAFE_RELEASE(pClassEnum);

    return hr;
}


HRESULT CCamCapture::GetInterfaces(void)
{
    HRESULT hr;

    // Create the filter graph
    hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                           IID_IGraphBuilder, (void **) &m_pGraph);
    if (FAILED(hr))
        return hr;

    // Create the capture graph builder
    hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
                           IID_ICaptureGraphBuilder2, (void **) &m_pCapture);
    if (FAILED(hr))
        return hr;
    
    // Obtain interfaces for media control and Video Window
    hr = m_pGraph->QueryInterface(IID_IMediaControl,(LPVOID *) &m_pMC);
    if (FAILED(hr))
        return hr;

    hr = m_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID *) &m_pVW);
    if (FAILED(hr))
        return hr;

    hr = m_pGraph->QueryInterface(IID_IMediaEventEx, (LPVOID *) &m_pME);
    if (FAILED(hr))
        return hr;

    // Set the window handle used to process graph events
    hr = m_pME->SetNotifyWindow((OAHWND)m_hwnd, WM_GRAPHNOTIFY, 0);

    return hr;
}


void CCamCapture::CloseInterfaces(void)
{
    // Stop previewing data
    if (m_pMC)
        m_pMC->StopWhenReady();

    m_psCurrent = Stopped;

    // Stop receiving events
    if (m_pME)
        m_pME->SetNotifyWindow(NULL, WM_GRAPHNOTIFY, 0);

    // Relinquish ownership (IMPORTANT!) of the video window.
    // Failing to call put_Owner can lead to assert failures within
    // the video renderer, as it still assumes that it has a valid
    // parent window.
    if (m_pVW)
    {
        m_pVW->put_Visible(OAFALSE);
        m_pVW->put_Owner(NULL);
    }

    // Release DirectShow interfaces
	SAFE_RELEASE(m_fileFilter);
    SAFE_RELEASE(m_pMC);
    SAFE_RELEASE(m_pME);
    SAFE_RELEASE(m_pVW);
    SAFE_RELEASE(m_pGraph);
    SAFE_RELEASE(m_pCapture);
}


HRESULT CCamCapture::SetupVideoWindow(void)
{
    HRESULT hr;

    // Set the video window to be a child of the main window
    hr = m_pVW->put_Owner((OAHWND)m_hwnd);
    if (FAILED(hr))
        return hr;
    
    // Set video window style
    hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
    if (FAILED(hr))
        return hr;

    // Use helper function to position video window in client rect 
    // of main application window
    ResizeVideoWindow();

    // Make the video window visible, now that it is properly positioned
    hr = m_pVW->put_Visible(OATRUE);

	return hr;
}


void CCamCapture::ResizeVideoWindow(void)
{
    // Resize the video preview window to match owner window size
    if (m_pVW)
    {
        RECT rc;
        
        // Make the preview video fill our window
        GetClientRect(m_hwnd, &rc);
        m_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
    }
}


HRESULT CCamCapture::ChangePreviewState(int nShow)
{
    HRESULT hr=S_OK;
    
    // If the media control interface isn't ready, don't call it
    if (!m_pMC)
        return S_OK;
    
    if (nShow)
    {
        if (m_psCurrent != Running)
        {
            // Start previewing video data
            hr = m_pMC->Run();
            m_psCurrent = Running;
        }
    }
    else
    {
        // Stop previewing video data
        hr = m_pMC->StopWhenReady();
        m_psCurrent = Stopped;
    }

    return hr;
}


HRESULT CCamCapture::HandleGraphEvent(void)
{
    LONG evCode;
	LONG_PTR evParam1, evParam2;
    HRESULT hr=S_OK;

    if (!m_pME)
        return E_POINTER;

    while(SUCCEEDED(m_pME->GetEvent(&evCode, &evParam1, &evParam2, 0)))
    {
        hr = m_pME->FreeEventParams(evCode, evParam1, evParam2);
    }

    return hr;
}


LRESULT CALLBACK _CamCaptWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (WM_CREATE == message)
	{
		CREATESTRUCT *createData = reinterpret_cast<CREATESTRUCT*>(lParam);

		ATLASSERT(createData);
		if (createData)
		{
			CCamCapture *thisPtr = reinterpret_cast<CCamCapture*>(createData->lpCreateParams);

			ATLASSERT(thisPtr);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(thisPtr));
		}
	}
	else if (WM_DESTROY == message)
		SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
	else
	{
		CCamCapture *thisPtr = reinterpret_cast<CCamCapture*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		if (thisPtr)
			if (thisPtr->CaptureWndProc(message, wParam, lParam)) return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
void CCamCapture::DrawQ()
{
	PAINTSTRUCT paintData;
	if (HDC hdc = BeginPaint(m_hwnd, &paintData))
	{
		SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
		SetBkColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, 4, 4, L"?", 1); 
		EndPaint(m_hwnd, &paintData);
	}
}
bool CCamCapture::CaptureWndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_GRAPHNOTIFY:
            HandleGraphEvent();
            return true;
        case WM_SIZE:
            ResizeVideoWindow();
            return true;

        case WM_WINDOWPOSCHANGED:
            ChangePreviewState(! (IsIconic(m_hwnd)));
            return true;

        case WM_CLOSE:            
            // Hide the main window while the graph is destroyed
            ShowWindow(m_hwnd, SW_HIDE);
            CloseInterfaces();  // Stop capturing and release interfaces
            return false;
		case WM_PAINT:
			if (!m_hasDevice)
			{
				DrawQ();
				return true;
			}
    }
    return false;
}

bool CCamCapture::Show()
{
	if (m_clas)
	    m_hwnd = CreateWindow(CLASSNAME, L"Who are you?",
                         WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_CLIPCHILDREN,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT,
                         0, 0, m_hinst, this);
	if (m_hwnd)
	{
		HRESULT hr = S_OK;

		ShowWindow(m_hwnd, SW_NORMAL);
        if (FAILED(hr = CaptureVideo()))
			m_logger.LogHR(L"CaptureVideo", hr);
	}
	return NULL != m_hwnd;
}
void CCamCapture::Close()
{
	if (m_hwnd && IsWindow(m_hwnd))
		SendMessage(m_hwnd, WM_CLOSE, 0, 0);
}

void CCamCapture::InitClass()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc   = _CamCaptWndProc;
    wc.hInstance     = m_hinst;
    wc.lpszClassName = CLASSNAME;
    wc.lpszMenuName  = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = NULL;
    m_clas = RegisterClass(&wc);
}

