#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlstr.h>

// Prototypes
typedef HRESULT (WINAPI *FPCanUnloadNow)(void);
typedef HRESULT (WINAPI *FPGetClassObject)(_In_ REFCLSID rclsid, _In_ REFIID riid, _Out_ LPVOID *ppv);
typedef HRESULT (WINAPI *FPRegisterServer)(void);
typedef void (CALLBACK *FPEntryPoint)(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

// Function pointers in wrapped library
HMODULE hWrappedLibrary = nullptr;
FPCanUnloadNow fpCanUnloadNow = nullptr;
FPGetClassObject fpGetClassObject = nullptr;
FPRegisterServer fpRegisterServer = nullptr, fpUnregisterServer = nullptr;
FPEntryPoint fpConfigure = nullptr;

#define EXPORT_API __declspec(dllexport)

STDAPI DllCanUnloadNow(void) {
	return (fpCanUnloadNow) ? fpCanUnloadNow() : S_OK;
}

typedef HRESULT (WINAPI *FuncCreateInstance)(IClassFactory* vtbl, IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
FuncCreateInstance fpOldCreateInstance = nullptr;

HRESULT WINAPI IClassFactory_CreateInstance(IClassFactory* vtbl, IUnknown* pUnkOuter, REFIID riid, void** ppvObject) {
	if (!fpOldCreateInstance)
		return E_UNEXPECTED;
	HRESULT hr = fpOldCreateInstance(vtbl, pUnkOuter, riid, ppvObject);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION;
	CComQIPtr<IPropertyBag> prog_bag = (IUnknown*)*ppvObject;
	LPOLESTR settings[] = {
		L"app_mode=1",
		L"ilevels=2",
		L"olevels=2",
		L"ics=5",
		L"di=9",
		L"deblock=10",
		L"ai=1",
		L"crop1088=1",
		L"vmr_ar=0",
		L"low_latency=0",
		L"brightness=0",
		L"contrast=0",
		L"saturation=0",
		L"use_tray=0",
		L"use_cuda=0"
	};
	for(int i=0; i < sizeof(settings)/sizeof(settings[0]); ++i) {
		hr = prog_bag->Write(T2OLE(_T("Settings")), &CComVariant(settings[i]));
		if (FAILED(hr))
			goto lerror;
	}
	return hr;

lerror:
	((IUnknown*)*ppvObject)->Release();
	return E_UNEXPECTED;
}

STDAPI DllGetClassObject(__in REFCLSID rclsid, __in REFIID riid, LPVOID FAR* ppv) {
	HRESULT hr = fpGetClassObject(rclsid, riid, ppv);
	if (hr != S_OK)
		return hr;
	
	if (fpOldCreateInstance == nullptr) {
		CComQIPtr<IClassFactory> pFactory = (IUnknown*)*ppv;
		IUnknown *unk = (IUnknown*)pFactory;
		void **vtbl = *(void***)unk;
		void **createinstance_ptr = vtbl + 3; // 3 is for CreateInstance offset
		fpOldCreateInstance = (FuncCreateInstance)createinstance_ptr[0];

		void *newptr = &IClassFactory_CreateInstance;
		// remove protection
		HANDLE cp = GetCurrentProcess();
		DWORD oldProtect;
		SIZE_T temp;
		VirtualProtectEx(cp, createinstance_ptr, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
		// write new method offset
		WriteProcessMemory(cp, createinstance_ptr, &newptr, sizeof(void*), &temp);
		// return protection back
		VirtualProtectEx(cp, createinstance_ptr, sizeof(void*), oldProtect, &oldProtect);
	}
	return hr;
}

STDAPI DllRegisterServer(void) {
	return (fpRegisterServer) ? fpRegisterServer() : E_FAIL;
}

STDAPI DllUnregisterServer(void) {
	return (fpUnregisterServer) ? fpUnregisterServer() : E_FAIL;
}

EXTERN_C void CALLBACK Configure(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	if (fpConfigure)
		return fpConfigure(hwnd, hinst, lpszCmdLine, nCmdShow);
}

LPTSTR lpDependency = _T("CoreAVCDecoder.dll");

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call)	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);

			TCHAR fullname[MAX_PATH+32];
			size_t len;
			if (GetModuleFileName(hModule, fullname, MAX_PATH) == 0)
				return FALSE;
			for (len = lstrlen(fullname); len > 0 && fullname[len - 1] != '\\'; --len);
			fullname[len] = '\0';
			lstrcat(fullname, lpDependency);
			hWrappedLibrary = LoadLibrary(fullname);

			if (hWrappedLibrary == nullptr)
				return FALSE;
			fpCanUnloadNow = (FPCanUnloadNow)GetProcAddress(hWrappedLibrary, "DllCanUnloadNow");
			fpGetClassObject = (FPGetClassObject)GetProcAddress(hWrappedLibrary, "DllGetClassObject");
			fpRegisterServer = (FPRegisterServer)GetProcAddress(hWrappedLibrary, "DllRegisterServer"); 
			fpUnregisterServer = (FPRegisterServer)GetProcAddress(hWrappedLibrary, "DllUnregisterServer");
			fpConfigure = (FPEntryPoint)GetProcAddress(hWrappedLibrary, "Configure");
			if (!fpGetClassObject || !fpCanUnloadNow)
				return FALSE;
			break;
		case DLL_PROCESS_DETACH:
			FreeLibrary(hWrappedLibrary);
			hWrappedLibrary = nullptr;
			fpCanUnloadNow = nullptr;
			fpGetClassObject = nullptr;
			fpRegisterServer = nullptr;
			fpUnregisterServer = nullptr;
			fpConfigure = nullptr;
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
