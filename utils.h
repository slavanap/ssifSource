#pragma once

#ifdef DECLARE_VARIABLES
#define MYEXTERN
#include <InitGuid.h>
#else
#define MYEXTERN extern
#endif

#include <strsafe.h>
#include <dshow.h>

DEFINE_GUID(CLSID_CoreAVC,           0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xde, 0x6d, 0x31, 0x0c, 0x7c, 0x31);
DEFINE_GUID(CLSID_MpegSplitter,      0x1365BE7A, 0xC86A, 0x473C, 0x9A, 0x41, 0xC0, 0xA6, 0xE8, 0x2C, 0x9F, 0xA3);
DEFINE_GUID(CLSID_MatroskaSplitter,  0x0A68C3B5, 0x9164, 0x4a54, 0xAF, 0xAF, 0x99, 0x5B, 0x2F, 0xF0, 0xE0, 0xD4);
DEFINE_GUID(CLSID_Mpeg4Splitter,     0x3CCC052E, 0xBDEE, 0x408a, 0xBE, 0xA7, 0x90, 0x91, 0x4E, 0xF2, 0x96, 0x4B);
DEFINE_GUID(CLSID_SampleGrabber,     0xb62f694e, 0x0593, 0x4e60, 0xaa, 0x1c, 0x16, 0xaf, 0x64, 0x96, 0xac, 0x39);

DEFINE_GUID(MEDIASUBTYPE_I420,       0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
DEFINE_GUID(MFVideoFormat_YUY2,      0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

MYEXTERN HINSTANCE hInstance;

HRESULT DSHelpCreateInstance(LPOLESTR bstrLibName, REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv);
HRESULT GetPin(IBaseFilter* pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin, bool bVideo = false);
HRESULT GetPinByName(IBaseFilter* pFilter, PIN_DIRECTION dirreq, LPCWSTR wszName, IPin **ppPin);

//
// NOTE: The GetInPin and GetOutPin methods DO NOT increment the reference count
// of the returned pin.  Use CComPtr interface pointers in your code to prevent
// memory leaks caused by reference counting problems.  The SDK samples that use
// these methods all use CComPtr<IPin> interface pointers.
// 
//     For example:  CComPtr<IPin> pPin = GetInPin(pFilter,0);
//
IPin* GetInPin(IBaseFilter * pFilter, int nPin, bool bVideo = false);
IPin* GetOutPin(IBaseFilter * pFilter, int nPin, bool bVideo = false);

HRESULT RenderOutputPins(IGraphBuilder* pGB, IBaseFilter* pFilter);

void ErrorMessageA(DWORD dwError, LPSTR pBuffer, size_t nSize);

#define SAFE_CloseHandle(x) do { \
    if ((x) != NULL) { \
        CloseHandle(x); \
        x = NULL; \
    } \
} while(0)


// logger class

class mylogger {
private:
    HANDLE hFile;
    RTL_CRITICAL_SECTION cs;
public:
    mylogger() {
        InitializeCriticalSection(&cs);
        hFile = CreateFile(L"ssifSource2.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    ~mylogger() {
        CloseHandle(hFile);
        DeleteCriticalSection(&cs);
    }
    void log(const char *fmt, ...) {
        char text[1024];
        va_list ap;
        if (fmt == NULL)
            return;
        va_start(ap, fmt);
        vsprintf_s(text, 1023, fmt, ap);
        va_end(ap);
        size_t len = strlen(text);
        text[len] = '\n';
        text[len+1] = 0;

        DWORD temp;
        EnterCriticalSection(&cs);
        WriteFile(hFile, text, len+1, &temp, NULL);
        LeaveCriticalSection(&cs);
    }
};

//MYEXTERN mylogger logger;