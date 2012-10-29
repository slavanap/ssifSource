#include "stdafx.h"

#define DECLARE_VARIABLES
#include "utils.h"

HRESULT DSHelpCreateInstance(LPOLESTR bstrLibName, REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv) { 
    // Load the library (bstrlibname should have the fullpath) 
    HINSTANCE hDLL = CoLoadLibrary(bstrLibName, TRUE); 
    if (hDLL == NULL) 
        return E_FAIL; 

    // Get the function pointer 
    typedef HRESULT (WINAPI* PFNDllGetClassObject)( 
        REFCLSID  rclsid, 
        REFIID    riid, 
        LPVOID*   ppv); 
    PFNDllGetClassObject pfnDllGetClassObject = 
        (PFNDllGetClassObject)GetProcAddress( 
        hDLL, "DllGetClassObject"); 
    if (!pfnDllGetClassObject) 
        return E_FAIL; 

    // Get the class factory 
    CComPtr<IClassFactory> pFactory; 
    HRESULT hr = pfnDllGetClassObject(rclsid, IID_IClassFactory, 
        (LPVOID*)&pFactory); 
    if (hr != S_OK) 
        return hr; 

    // Create object instance   
    return pFactory->CreateInstance(pUnkOuter, riid, ppv);
}

HRESULT GetPin(IBaseFilter* pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin) {
    CComPtr<IEnumPins> pEnum;
    *ppPin = NULL;

    if (!pFilter)
        return E_POINTER;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if(FAILED(hr)) 
        return hr;

    ULONG ulFound;
    IPin *pPin;
    hr = E_FAIL;

    while(S_OK == pEnum->Next(1, &pPin, &ulFound)) {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;
        pPin->QueryDirection(&pindir);
        if(pindir == dirrequired) {
            if(iNum == 0) {
                *ppPin = pPin;  // Return the pin's interface
                hr = S_OK;      // Found requested pin, so clear error
                break;
            }
            iNum--;
        } 
        pPin->Release();
    } 
    return hr;
}

//
// NOTE: The GetInPin and GetOutPin methods DO NOT increment the reference count
// of the returned pin.  Use CComPtr interface pointers in your code to prevent
// memory leaks caused by reference counting problems.  The SDK samples that use
// these methods all use CComPtr<IPin> interface pointers.
// 
//     For example:  CComPtr<IPin> pPin = GetInPin(pFilter,0);
//
IPin* GetInPin(IBaseFilter * pFilter, int nPin) {
    CComPtr<IPin> pComPin;
    GetPin(pFilter, PINDIR_INPUT, nPin, &pComPin);
    return pComPin;
}


IPin* GetOutPin(IBaseFilter * pFilter, int nPin) {
    CComPtr<IPin> pComPin;
    GetPin(pFilter, PINDIR_OUTPUT, nPin, &pComPin);
    return pComPin;
}

HRESULT RenderOutputPins(IGraphBuilder* pGB, IBaseFilter* pFilter) {
    HRESULT hr;
    IEnumPins *pEnumPin = NULL;
    IPin *pConnectedPin = NULL, *pPin = NULL;
    PIN_DIRECTION PinDirection;
    ULONG ulFetched;

    // Enumerate all pins on the filter
    hr = pFilter->EnumPins(&pEnumPin);

    if (SUCCEEDED(hr)) {
        // Step through every pin, looking for the output pins
        while((hr = pEnumPin->Next(1L, &pPin, &ulFetched)) == S_OK) {
            // Is this pin connected?  We're not interested in connected pins.
            hr = pPin->ConnectedTo(&pConnectedPin);
            if (pConnectedPin) {
                pConnectedPin->Release();
                pConnectedPin=NULL;
            }

            // If this pin is not connected, render it.
            if (hr == VFW_E_NOT_CONNECTED) {
                hr = pPin->QueryDirection(&PinDirection);
                if (hr == S_OK && PinDirection == PINDIR_OUTPUT) {
                    hr = pGB->Render(pPin);
                }
            }
            pPin->Release();

            // If there was an error, stop enumerating
            if (FAILED(hr))
                break;
        }
    }
    // Release pin enumerator
    pEnumPin->Release();
    return hr;
}


void ErrorMessageA(DWORD dwError, LPSTR pBuffer, size_t nSize) {
    LPVOID pText = 0;
    ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 
        NULL, dwError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPSTR)&pText, 0, NULL);
    StringCchCopyA(pBuffer, nSize, (LPSTR)pText);
    LocalFree(pText);
}
