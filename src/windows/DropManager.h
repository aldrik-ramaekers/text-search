#pragma once

#include "platform.h"

#include <oleidl.h>

// create a class inheriting from IDropTarget
class DropManager : public IDropTarget
{
public:
	//--- implement the IUnknown parts
	// you could do this the proper way with InterlockedIncrement etc,
	// but I've left out stuff that's not exactly necessary for brevity
	ULONG AddRef()  { return 1; }
	ULONG Release() { return 0; }

	// we handle drop targets, let others know
	HRESULT QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == IID_IDropTarget)
		{
			*ppvObject = this;	// or static_cast<IUnknown*> if preferred
			// AddRef() if doing things properly
                        // but then you should probably handle IID_IUnknown as well;
			return S_OK;
		}

		*ppvObject = NULL;
		return E_NOINTERFACE;
	};


	//--- implement the IDropTarget parts

	// occurs when we drag files into our applications view
	HRESULT DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		// TODO: check whether we can handle this type of object at all and set *pdwEffect &= DROPEFFECT_NONE if not;

		// do something useful to flag to our application that files have been dragged from the OS into our application
		//...
		dragdrop_data.is_dragging_file = true;

		// trigger MouseDown for button 1 within ImGui
		//...

		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}

	// occurs when we drag files out from our applications view
	HRESULT DragLeave() { 
		dragdrop_data.is_dragging_file = false;
		return S_OK; 
		}

	// occurs when we drag the mouse over our applications view whilst carrying files (post Enter, pre Leave)
	HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		// trigger MouseMove within ImGui, position is within pt.x and pt.y
		// grfKeyState contains flags for control, alt, shift etc
		//...

		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}

	// occurs when we release the mouse button to finish the drag-drop operation
	HRESULT Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		dragdrop_data.is_dragging_file = false;
		// grfKeyState contains flags for control, alt, shift etc

		// render the data into stgm using the data description in fmte
		FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgm;

		if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
		{
			HDROP hdrop = (HDROP)stgm.hGlobal; // or reinterpret_cast<HDROP> if preferred
			UINT file_count = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

			// We only accept 1 file for now..
			for (UINT i = 0; i < 1; i++)
			{
				TCHAR szFile[MAX_PATH];
				UINT cch = DragQueryFile(hdrop, i, szFile, MAX_PATH);
				if (cch > 0 && cch < MAX_PATH)
				{
					WideCharToMultiByte(CP_UTF8,0,szFile,-1,(LPSTR)dragdrop_data.path, MAX_INPUT_LENGTH, NULL, NULL);
				}
			}

			// we have to release the data when we're done with it
			ReleaseStgMedium(&stgm);
			
			dragdrop_data.did_drop = true;
		}

		// trigger MouseUp for button 1 within ImGui
		//...

		*pdwEffect &= DROPEFFECT_COPY;
		return S_OK;
	}
};