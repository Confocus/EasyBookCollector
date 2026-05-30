#pragma once
#include "framework.h"
#include <commctrl.h>  // ListView_SetColumnWidth等宏定义在这里
#include <oleidl.h>   
#include <shlobj.h>
#include <shobjidl.h>     // 常用拖放辅助接口
#include <unknwn.h>   
#include <shellapi.h>
#pragma comment(lib, "comctl32.lib")  // 链接公共控件库（避免链接错误）
#pragma comment(lib, "ole32.lib")

class CDropTarget : public IDropTarget
{
	LONG m_ref;
public:
	CDropTarget() : m_ref(1) {}

	// IUnknown
	HRESULT QueryInterface(REFIID iid, void** ppv) override
	{
		if (iid == IID_IUnknown || iid == IID_IDropTarget)
		{
			*ppv = this;
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	ULONG AddRef() override { return ++m_ref; }
	ULONG Release() override { if (--m_ref == 0) delete this; return m_ref; }

	// IDropTarget
	HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
	{
		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}
	HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
	{
		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}
	HRESULT DragLeave() override { return S_OK; }

	// 拖放落下 → 解析 URL
	HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
	{
		//todo：一种方法是，这里可以触发一次Web查询:browser.tabs.query({})
		//todo：方法二：Firefox Remote Debugging Protocol

		IEnumFORMATETC* pEnum = nullptr;
		if (SUCCEEDED(pDataObj->EnumFormatEtc(DATADIR_GET, &pEnum)))
		{
			FORMATETC fe;
			ULONG fetched;

			while (pEnum->Next(1, &fe, &fetched) == S_OK)
			{
				wchar_t name[256];
				GetClipboardFormatNameW(fe.cfFormat, name, 256);

				OutputDebugStringW(L"FORMAT: ");
				OutputDebugStringW(name);
				OutputDebugStringW(L"\n");
			}

			pEnum->Release();
		}
		//Firefox枚举出来的类型：
		/*	application / x - moz - tabbrowser - tab
			text / x - moz - text - internal
			DragImageBits
			DragContext*/

		*pdwEffect = DROPEFFECT_COPY;

		//{
		//	FORMATETC fe = {};
		//	fe.cfFormat = RegisterClipboardFormatW(L"application/x-moz-tabbrowser-tab");//
		//	fe.dwAspect = DVASPECT_CONTENT;
		//	fe.lindex = -1;
		//	fe.tymed = TYMED_HGLOBAL | TYMED_ISTREAM;//TYMED_HGLOBAL;
		//	HRESULT hr = pDataObj->QueryGetData(&fe);
		//	STGMEDIUM stg = {};
		//	if (SUCCEEDED(pDataObj->GetData(&fe, &stg)))
		//	{
		//		LPCWSTR text = (LPCWSTR)GlobalLock(stg.hGlobal);
		//		SIZE_T size = GlobalSize(stg.hGlobal);

		//		if (text)
		//		{
		//			OutputDebugStringW(L"FIREFOX DATA:\n");
		//			OutputDebugStringW(text);
		//			OutputDebugStringW(L"\n");

		//			GlobalUnlock(stg.hGlobal);
		//		}

		//		ReleaseStgMedium(&stg);
		//	}
		//}

		{
			FORMATETC fe = {};
			fe.cfFormat = RegisterClipboardFormatW(L"text/x-moz-text-internal");
			fe.dwAspect = DVASPECT_CONTENT;
			fe.lindex = -1;
			fe.tymed = TYMED_HGLOBAL;

			STGMEDIUM stg = {};

			if (SUCCEEDED(pDataObj->GetData(&fe, &stg)))
			{
				LPCWSTR text = (LPCWSTR)GlobalLock(stg.hGlobal);
				SIZE_T size = GlobalSize(stg.hGlobal);

				if (text)
				{
					OutputDebugStringW(L"FIREFOX DATA:\n");
					OutputDebugStringW(text);
					OutputDebugStringW(L"\n");

					GlobalUnlock(stg.hGlobal);
				}

				ReleaseStgMedium(&stg);
			}
		}

		/*{
			FORMATETC fe = {};
			fe.cfFormat = RegisterClipboardFormatW(L"DragContext");
			fe.dwAspect = DVASPECT_CONTENT;
			fe.lindex = -1;
			fe.tymed = TYMED_HGLOBAL;

			STGMEDIUM stg = {};

			if (SUCCEEDED(pDataObj->GetData(&fe, &stg)))
			{
				LPCWSTR text = (LPCWSTR)GlobalLock(stg.hGlobal);
				SIZE_T size = GlobalSize(stg.hGlobal);

				if (text)
				{
					OutputDebugStringW(L"FIREFOX DATA:\n");
					OutputDebugStringW(text);
					OutputDebugStringW(L"\n");

					GlobalUnlock(stg.hGlobal);
				}

				ReleaseStgMedium(&stg);
			}
		}*/

		return S_OK;
	}
};