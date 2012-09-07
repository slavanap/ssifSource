typedef TCHAR *PTCHAR;      // don't know why we loose it...
#include <streams.h>
#include "avisynth.h"

#pragma once

class CSampleGrabber: public CTransInPlaceFilter
{
private:
    // filter variables
    CMediaType m_mt;
    AM_MEDIA_TYPE *m_pAM_MEDIA_TYPE;

public:
    long m_SampleSize;
    long m_Width;
    long m_Height;
    long m_Stride;
    DWORD m_FrameSize;
    VideoInfo avisynth_vi;
    LONGLONG m_AvgTimePerFrame;
    HANDLE hDataReady, hDataParsed;
    BYTE * volatile pData;

    int nFrame;

public:
    CSampleGrabber(IUnknown* pOuter, HRESULT* phr, BOOL ModifiesData);
    ~CSampleGrabber();

    void CloseSyncHandles();

    // CTransInPlaceFilter
    HRESULT CheckInputType(const CMediaType *pmt);
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
    STDMETHODIMP Pause(void);
    STDMETHODIMP Stop(void);
    HRESULT Transform(IMediaSample *pMediaSample);

    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) {
        return NOERROR;
    }
    HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
};

CSampleGrabber* CreateGrabber();
