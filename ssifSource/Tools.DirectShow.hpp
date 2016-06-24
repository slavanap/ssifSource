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

#pragma once

#ifdef DECLARE_VARIABLES
#include <InitGuid.h>
#endif

#include <strsafe.h>
#include <dshow.h>

namespace Tools {

	namespace DirectShow {

		DEFINE_GUID(CLSID_CoreAVC, 0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xde, 0x6d, 0x31, 0x0c, 0x7c, 0x31);
		DEFINE_GUID(CLSID_MpegSplitter, 0x1365BE7A, 0xC86A, 0x473C, 0x9A, 0x41, 0xC0, 0xA6, 0xE8, 0x2C, 0x9F, 0xA3);
		DEFINE_GUID(CLSID_MatroskaSplitter, 0x0A68C3B5, 0x9164, 0x4a54, 0xAF, 0xAF, 0x99, 0x5B, 0x2F, 0xF0, 0xE0, 0xD4);
		DEFINE_GUID(CLSID_Mpeg4Splitter, 0x3CCC052E, 0xBDEE, 0x408a, 0xBE, 0xA7, 0x90, 0x91, 0x4E, 0xF2, 0x96, 0x4B);
		DEFINE_GUID(CLSID_SampleGrabber, 0xb62f694e, 0x0593, 0x4e60, 0xaa, 0x1c, 0x16, 0xaf, 0x64, 0x96, 0xac, 0x39);
		DEFINE_GUID(CLSID_DumpFilter, 0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

		DEFINE_GUID(MEDIASUBTYPE_I420, 0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
		DEFINE_GUID(MFVideoFormat_YUY2, 0x32595559, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

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

		HRESULT CreateDumpFilter(const IID& riid, LPVOID *pFilter);

	}

}
