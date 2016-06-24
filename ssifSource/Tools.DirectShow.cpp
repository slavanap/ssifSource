// BD3D video decoding tool
// Copyright 2016 Vyacheslav Napadovsky.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include <mtype.h>
#include "DumpFilter.hpp"

#define DECLARE_VARIABLES
#include "Tools.DirectShow.hpp"

namespace Tools {

	namespace DirectShow {

		HRESULT DSHelpCreateInstance(LPOLESTR bstrLibName, REFCLSID rclsid, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppv) {
			// Load the library (bstrlibname should have the fullpath) 
			HINSTANCE hDLL = CoLoadLibrary(bstrLibName, TRUE);
			if (hDLL == NULL)
				return E_FAIL;

			// Get the function pointer 
			typedef HRESULT(WINAPI* PFNDllGetClassObject)(
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

		HRESULT GetPin(IBaseFilter* pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin, bool bVideo) {
			CComPtr<IEnumPins> pEnum;
			*ppPin = NULL;

			if (!pFilter)
				return E_POINTER;

			HRESULT hr = pFilter->EnumPins(&pEnum);
			if (FAILED(hr))
				return hr;

			ULONG ulFound;
			IPin *pPin;
			hr = E_FAIL;

			while (S_OK == pEnum->Next(1, &pPin, &ulFound)) {
				PIN_DIRECTION pindir = (PIN_DIRECTION)3;
				pPin->QueryDirection(&pindir);
				if (bVideo) {
					AM_MEDIA_TYPE *pMT;
					CComPtr<IEnumMediaTypes> pMTEnum;
					if (FAILED(pPin->EnumMediaTypes((IEnumMediaTypes**)&pMTEnum)) ||
						FAILED(pMTEnum->Next(1, (AM_MEDIA_TYPE**)&pMT, NULL)))
					{
						continue;
					}
					pMTEnum = NULL;
					bool bCurVideo = ((pMT->majortype == MEDIATYPE_Video) != 0);
					DeleteMediaType(pMT);
					if (!bCurVideo)
						continue;
				}
				if (pindir == dirrequired) {
					if (iNum == 0) {
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

		HRESULT GetPinByName(IBaseFilter* pFilter, PIN_DIRECTION dirreq, LPCWSTR wszName, IPin **ppPin) {
			CComPtr<IEnumPins> pEnum;
			*ppPin = NULL;

			if (!pFilter)
				return E_POINTER;

			HRESULT hr = pFilter->EnumPins(&pEnum);
			if (FAILED(hr))
				return hr;

			ULONG ulFound;
			IPin *pPin;
			hr = E_FAIL;

			while (S_OK == pEnum->Next(1, &pPin, &ulFound)) {
				PIN_INFO pInfo;
				pInfo.dir = (PIN_DIRECTION)3;
				if (SUCCEEDED(pPin->QueryPinInfo(&pInfo))) {
					if ((dirreq == (PIN_DIRECTION)(-1) || dirreq == pInfo.dir) && !StrCmpW(pInfo.achName, wszName)) {
						*ppPin = pPin;
						hr = S_OK;
						break;
					}
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
		IPin* GetInPin(IBaseFilter * pFilter, int nPin, bool bVideo) {
			CComPtr<IPin> pComPin;
			GetPin(pFilter, PINDIR_INPUT, nPin, &pComPin, bVideo);
			return pComPin;
		}


		IPin* GetOutPin(IBaseFilter * pFilter, int nPin, bool bVideo) {
			CComPtr<IPin> pComPin;
			GetPin(pFilter, PINDIR_OUTPUT, nPin, &pComPin, bVideo);
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
				while ((hr = pEnumPin->Next(1L, &pPin, &ulFetched)) == S_OK) {
					// Is this pin connected?  We're not interested in connected pins.
					hr = pPin->ConnectedTo(&pConnectedPin);
					if (pConnectedPin) {
						pConnectedPin->Release();
						pConnectedPin = NULL;
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

		HRESULT CreateDumpFilter(const IID& riid, LPVOID *pFilter) {
			//  for debugging with installed filter
			//	return CoCreateInstance(CLSID_DumpFilter, NULL, CLSCTX_INPROC_SERVER, riid, pFilter);

			HRESULT hr = S_OK;
			CUnknown *filter = CDump::CreateInstance(NULL, &hr);
			if (SUCCEEDED(hr))
				return filter->NonDelegatingQueryInterface(riid, pFilter);
			return hr;
		}

	}

}