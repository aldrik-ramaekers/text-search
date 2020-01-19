/*
    LooplessSizeMove.c
    Implements functions for modal-less window resizing and movement in Windows
	
    Author: Nathaniel J Fries
	
    The author asserts no copyright, this work is released into the public domain.
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef TIME_LOOP
#include <stdio.h>
#endif /* TIME_LOOP */

/*	fills the MINMAXINFO structure pointed to by the second argument with
 default MIMAX values, then sends WM_GETMINMAXINFO to allow the
 user's Window Procedure to modify it.
*/
void GetMinMaxInfo(HWND hwnd, PMINMAXINFO info);
/* 	begins the loopless resize/move process */
LRESULT PrepareSizeMove(HWND hwnd, WPARAM action, DWORD dwPos);
/* 	stops the loopless resize/move process.
 if cancel is TRUE, restores window size and position to
 what they were before resizing/moving.
*/
void StopSizing(BOOL cancel);
/* 	see LooplessSizeMove.h */
LRESULT CALLBACK LSMProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOLEAN SizingCheck(const MSG *lpmsg);

#define GetWindowLongAW(hwnd, lp)\
(IsWindowUnicode(hwnd)) ? \
(GetWindowLongW(hwnd, lp)) : \
(GetWindowLongA(hwnd, lp))
#define SendMessageAW(hwnd, msg, wParam, lParam)\
(IsWindowUnicode(hwnd)) ? \
(SendMessageW(hwnd, msg, wParam, lParam)) : \
(SendMessageA(hwnd, msg, wParam, lParam))
#define PostMessageAW(hwnd, msg, wParam, lParam)\
(IsWindowUnicode(hwnd)) ? \
(PostMessageW(hwnd, msg, wParam, lParam)) : \
(PostMessageA(hwnd, msg, wParam, lParam))
#define DefWindowProcAW(hwnd, msg, wParam, lParam)\
(IsWindowUnicode(hwnd)) ? \
(DefWindowProcW(hwnd, msg, wParam, lParam)) : \
(DefWindowProcA(hwnd, msg, wParam, lParam))

#define RECTWIDTH(r)	((r).right - (r).left)
#define RECTHEIGHT(r)	((r).bottom - (r).top)

#define LSM_LEFT 	0x01
#define LSM_TOP 	0x02
#define LSM_RIGHT   0x04
#define LSM_BOTTOM  0x08
#define LSM_CAPTION 0x00
#define LSM_NOGRAB  0xF0

#define LSM_SHAKE_MINTIME 20 /* minimum time between bothering */
#define LSM_SHAKE_MAXTIME 1000 /* maximum time between movements for "shake" effect */
#define LSM_SHAKE_STATE_MAXIMIZED 1
#define LSM_SNAP_HELPER_CLASS "LSM_SNAP_HELPER"
/*
    information required to reverse a "shake" action.
*/
typedef struct _SHAKERESTORENODE
{
    HWND hwnd;
    WINDOWPLACEMENT place;
    struct _SHAKERESTORENODE *next;
} SHAKERESTORENODE, *PSHAKERESTORENODE;
typedef struct
{
    DWORD dwPrevTime;
    WORD wDir;
    WORD wCount;
    SHAKERESTORENODE *restoreList;
} SHAKEDATA;
typedef BOOL (*SHAKEFOREACHFN)(PSHAKERESTORENODE, LPARAM);

typedef struct
{
    HWND helperWindow;
    WORD isSnapped;
    WORD snapType; /* LSM_TOP, LSM_LEFT, LSM_RIGHT */
    RECT rcWork;
    RECT rcRestore;
} SNAPDATA;

/* holds all data that needs to be held on to for resizing */
typedef struct
{
    HWND hwnd;
    MINMAXINFO minmax;
    RECT rcWin;
    RECT rcOrig;
    POINT ptCapture;
    LONG grab;
    SHAKEDATA shake;
    SNAPDATA snap;
} SIZEMOVEDATA;

DWORD dwSizeMoveTlsIndex = 0;
#define LSMTlsCheck() if(dwSizeMoveTlsIndex == 0){ dwSizeMoveTlsIndex = TlsAlloc(); }
#define LSMSet(data) TlsSetValue(dwSizeMoveTlsIndex, data)
#define LSMGet() TlsGetValue(dwSizeMoveTlsIndex)

void GetMinMaxInfo(HWND hwnd, PMINMAXINFO info)
{
    RECT rc;
    LONG style = GetWindowLongAW(hwnd, GWL_STYLE);
    LONG altStyle = ((style & WS_CAPTION) == WS_CAPTION)?
        (style & ~WS_BORDER):(style);
	
    /* calculate the default values in case WindowProc does not respond */
    GetClientRect(GetParent(hwnd), &rc);
    AdjustWindowRectEx(&rc, altStyle, ((style & WS_POPUP) && GetMenu(hwnd)),
					   GetWindowLongAW(hwnd, GWL_EXSTYLE));
    info->ptMaxPosition.x = rc.left;
    info->ptMaxPosition.y = rc.top;
    info->ptMaxSize.x = rc.right - rc.left;
    info->ptMaxSize.y = rc.bottom - rc.top;
    if(style & WS_CAPTION)
    {
        info->ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
        info->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
    }
    else
    {
        /* why not zero? this is what ReactOS and presumably Wine do,
        and they're the experts at replicating Windows UI behavior */
        info->ptMinTrackSize.x = info->ptMaxPosition.x * -2;
        info->ptMaxTrackSize.y = info->ptMaxPosition.y * -2;
    }
    info->ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
    info->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);
	
    /* ask Window proc to make any changes */
    SendMessageAW(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)info);
}

BOOL ForEachShakeNodeFree(PSHAKERESTORENODE node, LPARAM ignore)
{
    LocalFree(node);
    return TRUE;
}
BOOL ForEachShakeNode(SHAKEDATA *pShake, SHAKEFOREACHFN fn, LPARAM lParam)
{
    SHAKERESTORENODE *next, *curr;
    BOOL res = TRUE;
    if(pShake->restoreList)
    {
        curr = pShake->restoreList;
        while(res && curr)
        {
            next = curr->next;
            fn(curr, lParam);
            curr = next;
        }
    }
    return res;
}

void SnapCleanup(SIZEMOVEDATA *sizemove)
{
    ShowWindow(sizemove->snap.helperWindow, SW_HIDE);
    ZeroMemory(&sizemove->snap.rcWork, sizeof(RECT));
}

LRESULT WINAPI SnapHelperWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_PAINT:
        {
            SIZEMOVEDATA *sizemove = LSMGet();
            HDC hDC = GetDC(hwnd);
            HPEN hPen = CreatePen(PS_INSIDEFRAME, 1, GetSysColor(COLOR_HIGHLIGHT));
            LOGBRUSH blog = { BS_HOLLOW, 0, 0 };
            HBRUSH hBrush = CreateBrushIndirect(&blog);
            if(!sizemove || !hDC || !hPen || !hBrush)
                break;
			
            SelectObject(hDC, hPen);
            SelectObject(hDC, hBrush);
			
            Rectangle(hDC, 0, 0, RECTWIDTH(sizemove->snap.rcWork), RECTHEIGHT(sizemove->snap.rcWork));
			
            DeleteObject(hBrush);
            DeleteObject(hPen);
            break;
        }
		default:
        {
            break;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT PrepareSizeMove(HWND hwnd, WPARAM action, DWORD dwPos)
{
    WINDOWINFO winfo;
    SIZEMOVEDATA *sm;
    RECT rcClipCursor;
	
    winfo.cbSize = sizeof(WINDOWINFO);
    /* most likely not a valid window */
    if(GetWindowInfo(hwnd, &winfo) == FALSE)
        return 0;
    /* can't move or resize an invisible window */
    if(!IsWindowVisible(hwnd))
        return 0;
    /* can't resize a window without the resizing border */
    if((action & 0xfff0) == SC_MOVE && !(winfo.dwStyle & WS_SIZEBOX))
        return 0;
	
    /*
      if another window on this thread has capture,
      it might be using this too...
      tell it to clean up before setting the tls value
    */
    ReleaseCapture();
	
    if(!(sm = LSMGet()))
    {
        WNDCLASSA wndcls;
        HINSTANCE hInstance = GetModuleHandleA(NULL);
        sm = LocalAlloc(0, sizeof(SIZEMOVEDATA));
        if(!sm)
        {
            /* error */
            return 1;
        }
        if(!LSMSet(sm))
        {
            LocalFree(sm);
            return 2;
        }
        /* prevent potential crashes and bad initialization bugs */
        ZeroMemory(sm, sizeof(SIZEMOVEDATA));
		
        if(!GetClassInfoA(hInstance, LSM_SNAP_HELPER_CLASS, &wndcls))
        {
            ZeroMemory(&wndcls, sizeof(WNDCLASSA));
            wndcls.style = CS_SAVEBITS;
            wndcls.hbrBackground = GetSysColorBrush(COLOR_HOTLIGHT);
            wndcls.lpszClassName = LSM_SNAP_HELPER_CLASS;
            wndcls.lpfnWndProc = SnapHelperWinProc;
            wndcls.hInstance = hInstance;
            RegisterClassA(&wndcls);
        }
        /*  WS_EX_LAYERED: window is not fully opaque
            WS_EX_TRANSPARENT: mouse events pass through layered window
            ES_EX_NOACTIVATE: window cannot be activated by user (no loss of mouse capture or keyboard focus)
        */
        sm->snap.helperWindow = CreateWindowExA(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
												LSM_SNAP_HELPER_CLASS, "", WS_POPUP /* no borders */, 0, 0, 100, 100, NULL, NULL, hInstance, NULL);
        if(sm->snap.helperWindow)
        {
            SetLayeredWindowAttributes(sm->snap.helperWindow, 0, 75, LWA_ALPHA);
        }
    }
    else if(sm->hwnd != hwnd)
    {
        /* forget shake data, consistent with Windows 7 behavior */
        ForEachShakeNode(&sm->shake, ForEachShakeNodeFree, 0);
        SnapCleanup(sm);
        sm->shake.restoreList = NULL;
    }
    sm->grab = action & 0x000f;
    sm->hwnd = hwnd;
    GetMinMaxInfo(hwnd, &sm->minmax);
    sm->rcWin = winfo.rcWindow;
    if(winfo.dwStyle & WS_CHILD)
    {
        /* map points into the parent's coordinate space */
        HWND parent = GetParent(hwnd);
        MapWindowPoints(0, parent, (LPPOINT)&sm->rcWin, 2);
        GetWindowRect(parent, &rcClipCursor);
        MapWindowPoints(parent, HWND_DESKTOP, (LPPOINT)&rcClipCursor, 2);
    }
    else if(!(winfo.dwExStyle & WS_EX_TOPMOST))
    {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcClipCursor, 0);
    }
    else
    {
        rcClipCursor.left = rcClipCursor.top = 0;
        rcClipCursor.right = GetSystemMetrics(SM_CXSCREEN);
        rcClipCursor.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    sm->rcOrig = sm->rcWin;
	
    sm->ptCapture.x = (short)LOWORD(dwPos);
    sm->ptCapture.y = (short)HIWORD(dwPos);
    ClipCursor(&rcClipCursor);
	
	
    /* notify WinProc we're beginning, but return instead of looping */
    SendMessageAW(hwnd, WM_ENTERSIZEMOVE, 0, 0);
    if(GetCapture() != hwnd)
    {
        SetCapture(hwnd);
    }
    return 0;
}

LRESULT CALLBACK LSMProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LSMTlsCheck();
    switch(msg)
    {
        case WM_NCLBUTTONDOWN:
        {
            switch(wParam)
            {
                case HTLEFT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_LEFT, lParam);
                case HTTOPLEFT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_LEFT | LSM_TOP, lParam);
                case HTBOTTOMLEFT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_LEFT | LSM_BOTTOM, lParam);
                case HTRIGHT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_RIGHT, lParam);
                case HTTOPRIGHT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_RIGHT | LSM_TOP, lParam);
                case HTBOTTOMRIGHT:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_RIGHT | LSM_BOTTOM, lParam);
                case HTTOP:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_TOP, lParam);
                case HTBOTTOM:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_SIZE | LSM_BOTTOM, lParam);
                case HTCAPTION:
				return SendMessageAW(hwnd, WM_SYSCOMMAND, SC_MOVE | LSM_CAPTION, lParam);
                default:
				break;
            }
        }
        case WM_SYSCOMMAND:
        {
            switch(wParam & 0xfff0)
            {
                case SC_MOVE:
                case SC_SIZE:
				/* begin resize 'loop' */
				return PrepareSizeMove(hwnd, wParam, lParam);
                default:
				break;
            }
            break;
        }
        case WM_CAPTURECHANGED:
        {
            /* nothing we can do; stop resizing & do clean-up */
            SIZEMOVEDATA *sizemove = LSMGet();
            if(sizemove && (HWND)lParam != sizemove->hwnd)
            {
                SendMessageAW(sizemove->hwnd, WM_EXITSIZEMOVE, 0, 0);
                ClipCursor(NULL);
                /* no longer unset LSM data:
                    1) never really needed to
                    2) necessary to hold onto shake data
                */
                if(GetForegroundWindow() != sizemove->hwnd)
                {
                    /* forget shake data, consistent with Windows 7 behavior */
                    ForEachShakeNode(&sizemove->shake, ForEachShakeNodeFree, 0);
                    sizemove->shake.restoreList = NULL;
                }
                sizemove->shake.wCount = 0;
                sizemove->shake.wDir = 0;
                sizemove->shake.dwPrevTime = 0;
                SnapCleanup(sizemove);
				
                sizemove->grab = LSM_NOGRAB;
            }
        }
        default:
		break;
    }
    return DefWindowProcAW(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsShakeMinimize(HWND hwnd, LPARAM lParam)
{
    SIZEMOVEDATA *sizemove = (SIZEMOVEDATA *)(lParam);
    if(hwnd != sizemove->hwnd && IsWindowVisible(hwnd) && !IsIconic(hwnd))
    {
        SHAKERESTORENODE *pNode = 0;
        WINDOWPLACEMENT tPlace;
        /* note: there are a few shell-created windows that glitch up graphically if you try to minimize them */
        DWORD dwProcID, dwShellPID;
        GetWindowThreadProcessId(hwnd, &dwProcID);
        GetWindowThreadProcessId(GetShellWindow(), &dwShellPID);
        if(dwProcID == dwShellPID)
        {
            return TRUE;
        }
        /* note: not supposed to minimize "parent application" windows. I assume this means windows of
            the window's application?
            See: http://social.technet.microsoft.com/Forums/windows/en-US/b047378c-2a5a-4661-a871-cec459cb9bdc/aeroshake-not-working?forum=w7itproui
        */
        GetWindowThreadProcessId(sizemove->hwnd, &dwShellPID);
        if(dwProcID == dwShellPID)
        {
            return TRUE;
        }
		
		
        pNode = LocalAlloc(0, sizeof(SHAKERESTORENODE));
        if(!pNode)
            return FALSE;
		
        pNode->hwnd = hwnd;
		
        pNode->place.length = sizeof(WINDOWPLACEMENT);
        pNode->place.flags = WPF_ASYNCWINDOWPLACEMENT;
        GetWindowPlacement(hwnd, &pNode->place);
        tPlace = pNode->place;
        tPlace.showCmd = SW_SHOWMINNOACTIVE;
        SetWindowPlacement(hwnd, &tPlace);
		
        pNode->next = sizemove->shake.restoreList;
        sizemove->shake.restoreList = pNode;
    }
    return TRUE;
}

BOOL ForEachShakeNodeRestore(PSHAKERESTORENODE node, LPARAM ignore)
{
    if(node)
    {
        SetWindowPlacement(node->hwnd, &node->place);
        LocalFree(node);
        return TRUE;
    }
    return FALSE;
}

void ShakeCheck(const MSG *lpmsg, SIZEMOVEDATA *sizemove)
{
    static DWORD dwShakeDisabled = 2;
    WORD dir = 0;
    int dx, dy;
    if(lpmsg->message != WM_MOUSEMOVE && lpmsg->message != WM_NCMOUSEMOVE)
        return;
    if(dwShakeDisabled == 2)
    {
        HKEY hKey;
        DWORD dwSizeDW = 4;
        DWORD dwType = REG_DWORD;
        if(/* major version number */LOBYTE(LOWORD(GetVersion())) < 6)
        {
            dwShakeDisabled = 1;
        }
        else
        {
            dwShakeDisabled = 0;
            if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Policies\\Microsoft\\Windows\\Explorer", 0, KEY_READ, &hKey)
			   == ERROR_SUCCESS)
            {
                RegQueryValueExA(hKey, "NoWindowMinimizingShortcuts", 0, &dwType, (LPBYTE)&dwShakeDisabled, &dwSizeDW);
                RegCloseKey(hKey);
            }
        }
    }
    if(dwShakeDisabled)
        return;
    if(lpmsg->time - sizemove->shake.dwPrevTime < LSM_SHAKE_MINTIME)
        return;
	
    dx = lpmsg->pt.x - sizemove->ptCapture.x;
    dy = lpmsg->pt.y - sizemove->ptCapture.y;
    if(dx > 8)
        dir |= LSM_LEFT;
    else if(dx < -8)
        dir |= LSM_RIGHT;
    if(dy > 8)
        dir |= LSM_TOP;
    else if(dy < -8)
        dir |= LSM_BOTTOM;
	
    if(dir && ((lpmsg->time - sizemove->shake.dwPrevTime) < LSM_SHAKE_MAXTIME) && (dir != sizemove->shake.wDir))
    {
        /* Do Shake */
        sizemove->shake.wCount++;
        if(sizemove->shake.wCount >= 3)
        {
            if(sizemove->shake.restoreList)
            {
                ForEachShakeNode(&sizemove->shake, ForEachShakeNodeRestore, (LPARAM)sizemove);
                sizemove->shake.restoreList = 0;
            }
            else
            {
                EnumWindows(EnumWindowsShakeMinimize, (LPARAM)(sizemove));
            }
            RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INTERNALPAINT | RDW_INVALIDATE
						 | RDW_ALLCHILDREN | RDW_UPDATENOW);
            sizemove->shake.wCount = 0;
        }
    }
    else
    {
        sizemove->shake.wCount = 0;
    }
    sizemove->shake.dwPrevTime = lpmsg->time;
    sizemove->shake.wDir = dir;
}

void SnapCheck(POINT pt, SIZEMOVEDATA *sizemove)
{
    static DWORD dwSnapActive = 2;
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO minfo;
	
    if(dwSnapActive == 2)
    {
        HKEY hKey;
        DWORD dwSizeDW = 4;
        DWORD dwType = REG_DWORD;
        dwSnapActive = 0;
        if(/* major version number */LOBYTE(LOWORD(GetVersion())) >= 6)
        {
            if(RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_READ, &hKey)
			   == ERROR_SUCCESS)
            {
                RegQueryValueExA(hKey, "WindowArrangementActive", 0, &dwType, (LPBYTE)&dwSnapActive, &dwSizeDW);
                RegCloseKey(hKey);
            }
        }
    }
    if(!dwSnapActive)
        return;
	
    minfo.cbSize = sizeof(MONITORINFO);
    if(monitor == NULL)
        return;
    if(!GetMonitorInfo(monitor, (LPMONITORINFO)&minfo))
        return;
    /* unsnap maximized windows */
    if(IsZoomed(sizemove->hwnd))
    {
        WINDOWPLACEMENT place;
        int dx, w;
        place.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(sizemove->hwnd, &place);
        place.showCmd = SW_RESTORE;
        OffsetRect(&place.rcNormalPosition, (sizemove->rcWin.left - place.rcNormalPosition.left),
                   (sizemove->rcWin.top - place.rcNormalPosition.top));
        w = RECTWIDTH(place.rcNormalPosition);
        if((dx = (place.rcNormalPosition.left - sizemove->ptCapture.x)) > 0)
        {
            place.rcNormalPosition.left += dx << 1;
            place.rcNormalPosition.right = place.rcNormalPosition.left + w;
        }
        if((dx = (sizemove->ptCapture.x - place.rcNormalPosition.right)) > 0)
        {
            place.rcNormalPosition.left += dx << 1;
            place.rcNormalPosition.right = place.rcNormalPosition.left + w;
        }
        sizemove->rcWin = place.rcNormalPosition;
        SetWindowPlacement(sizemove->hwnd, &place);
        /* don't return here, it's possible to unsnap without leaving the top of the screen */
    }
    else if(sizemove->snap.isSnapped)
    {
        /* TODO: Aero snap lets you drag half-screen snapped windows across the top of the screen */
		
        sizemove->rcWin.right = sizemove->snap.rcRestore.right;
        sizemove->rcWin.bottom = sizemove->snap.rcRestore.bottom;
        sizemove->snap.isSnapped = 0;
        return;
    }
    sizemove->snap.rcWork = minfo.rcWork;
    if(pt.x <= minfo.rcWork.left)
    {
        /* left side stretch */
        sizemove->snap.rcWork.right = (RECTWIDTH(minfo.rcWork) - GetSystemMetrics(SM_CXFRAME)) >> 1;
        sizemove->snap.snapType = LSM_LEFT;
    }
    else if(pt.x+1 >= minfo.rcWork.right)
    {
        /* right side stretch */
        sizemove->snap.rcWork.left = (minfo.rcWork.left + RECTWIDTH(minfo.rcWork) + GetSystemMetrics(SM_CXFRAME)) >> 1;
        sizemove->snap.snapType = LSM_RIGHT;
    }
    else if(pt.y <= minfo.rcWork.top)
    {
        /* full stretch */
        sizemove->snap.snapType = LSM_TOP;
    }
    else
    {
        /* not snapping, clean up snap state */
        SnapCleanup(sizemove);
        return;
    }
    SetWindowPos(sizemove->snap.helperWindow, HWND_TOPMOST,
                 sizemove->snap.rcWork.left, sizemove->snap.rcWork.top,
                 RECTWIDTH(sizemove->snap.rcWork), RECTHEIGHT(sizemove->snap.rcWork),
                 SWP_NOACTIVATE);
    ShowWindow(sizemove->snap.helperWindow, SW_SHOWNA);
}

void SnapFinalize(SIZEMOVEDATA *sizemove)
{
    if(sizemove->snap.rcWork.left != sizemove->snap.rcWork.right
       && sizemove->snap.rcWork.top != sizemove->snap.rcWork.bottom)
    {
        if(sizemove->snap.snapType == LSM_TOP)
        {
            /* Aero appears to animate this... so we will too */
            ShowWindow(sizemove->hwnd, SW_MAXIMIZE);
        }
        else
        {
            SetWindowPos(sizemove->hwnd, 0, sizemove->snap.rcWork.left, sizemove->snap.rcWork.top,
						 RECTWIDTH(sizemove->snap.rcWork), RECTHEIGHT(sizemove->snap.rcWork), 0);
            ZeroMemory(&sizemove->snap.rcRestore, sizeof(RECT));
            sizemove->snap.rcRestore.right = RECTWIDTH(sizemove->rcWin);
            sizemove->snap.rcRestore.bottom = RECTWIDTH(sizemove->rcWin);
            sizemove->snap.isSnapped = 1;
        }
    }
    SnapCleanup(sizemove);
}

BOOLEAN SizingCheck(const MSG *lpmsg)
{
    SIZEMOVEDATA *sizemove = LSMGet();
    POINT pt = lpmsg->pt;
    int dx = 0, dy = 0;
	/*
        Discussion of rev3 changes.
        There was a bug in previous revisions that would cause
            the resize state to continue even if the Window lost
            mouse capture. Windows provides notification of losing
            mouse capture, but it crashes the program to take capture
            back while processing that message.
            So, we choose to yield to this other program and stop resizing.
            This is probably user32 behavior anyway.
            Windows also sends this notification in response to calling ReleaseCapture,
                so all clean-up code has been moved to the handler.
            This also allowed us to eliminate the function StopSizing.
 */
    if(!sizemove) /* not sizing */
        return 0;
    if(sizemove->grab == LSM_NOGRAB) /* not sizing */
        return 0;
    if(lpmsg->hwnd != sizemove->hwnd) /* wrong window */
        return 0;
    if(lpmsg->message == WM_NCLBUTTONUP || lpmsg->message == WM_LBUTTONUP)
    {
        SnapFinalize(sizemove);
        ReleaseCapture();
        return 1;
    }
    if(lpmsg->message == WM_KEYDOWN)
    {
        switch(lpmsg->wParam)
        {
            case VK_RETURN:
			ReleaseCapture();
			return 1;
            case VK_ESCAPE:
            {
                SetWindowPos(sizemove->hwnd, 0, sizemove->rcOrig.left, sizemove->rcOrig.top,
							 RECTWIDTH(sizemove->rcOrig), RECTHEIGHT(sizemove->rcOrig), 0);
                ReleaseCapture();
                return 1;
            }
            case VK_UP:
			pt.y-=8;
			break;
            case VK_DOWN:
			pt.y+=8;
			break;
            case VK_LEFT:
			pt.x-=8;
			break;
            case VK_RIGHT:
			pt.x+=8;
			break;
            default:
			break;
        }
    }
	
    /* used to handle WM_MOUSEMOVE. This was unnecessary code  */
	
    dx = pt.x - sizemove->ptCapture.x;
    dy = pt.y - sizemove->ptCapture.y;
    if(dx || dy)
    {
        BOOL changeCursor = (lpmsg->message == WM_KEYDOWN);
        WPARAM wpHit = 0;
#ifdef TIME_LOOP
        LARGE_INTEGER pfBegin;
        LARGE_INTEGER pfDraw;
        LARGE_INTEGER pfFinal;
        LARGE_INTEGER pfFreq;
        QueryPerformanceFrequency(&pfFreq);
        QueryPerformanceCounter(&pfBegin);
#endif
		
        if(sizemove->grab == LSM_CAPTION)
        {
            ShakeCheck(lpmsg, sizemove);
            SnapCheck(pt, sizemove);
            OffsetRect(&sizemove->rcWin, dx, dy);
        }
		else
        {
            /* note on minmax correction
                if you do not correct the capture pos (set later from `pt`),
                window will expand massively if user pulls back mouse
                after failing to shrink when resizing from the
                bottom or the right borders.
            */
            /* when resizing using keys, Windows also moves the cursor */
            if(sizemove->grab & LSM_LEFT)
            {
                int lmax = sizemove->rcWin.right - sizemove->minmax.ptMaxTrackSize.x;
                int lmin = sizemove->rcWin.right - sizemove->minmax.ptMinTrackSize.x;
                if(sizemove->rcWin.left + dx < lmax)
                {
                    sizemove->rcWin.left = lmax;
                }
                else if(sizemove->rcWin.left + dx > lmin)
                {
                    sizemove->rcWin.left = lmin;
                }
                else
                {
                    sizemove->rcWin.left += dx;
                }
                pt.x = sizemove->rcWin.left;
                wpHit = WMSZ_LEFT;
            }
            else if(sizemove->grab & LSM_RIGHT)
            {
                int rmax = sizemove->rcWin.left + sizemove->minmax.ptMaxTrackSize.x;
                int rmin = sizemove->rcWin.left + sizemove->minmax.ptMinTrackSize.x;
                if(sizemove->rcWin.right + dx > rmax)
                {
                    sizemove->rcWin.right = rmax;
                }
                else if(sizemove->rcWin.right + dx < rmin)
                {
                    sizemove->rcWin.right = rmin;
                }
                else
                {
                    sizemove->rcWin.right += dx;
                }
                pt.x = sizemove->rcWin.right;
                wpHit = WMSZ_RIGHT;
            }
            if(sizemove->grab & LSM_TOP)
            {
                int tmax = sizemove->rcWin.bottom - sizemove->minmax.ptMaxTrackSize.y;
                int tmin = sizemove->rcWin.bottom - sizemove->minmax.ptMinTrackSize.y;
                if(sizemove->rcWin.top + dy < tmax)
                {
                    sizemove->rcWin.top = tmax;
                }
                else if(sizemove->rcWin.top + dy > tmin)
                {
                    sizemove->rcWin.top = tmin;
                }
                else
                {
                    sizemove->rcWin.top += dy;
                }
                pt.y = sizemove->rcWin.top;
                if(wpHit == WMSZ_LEFT)
                {
                    wpHit = WMSZ_TOPLEFT;
                }
                else if(wpHit == WMSZ_RIGHT)
                {
                    wpHit = WMSZ_TOPRIGHT;
                }
                else
                {
                    wpHit = WMSZ_TOP;
                }
            }
            else if(sizemove->grab & LSM_BOTTOM)
            {
                int bmax = sizemove->rcWin.top + sizemove->minmax.ptMaxTrackSize.y;
                int bmin = sizemove->rcWin.top + sizemove->minmax.ptMinTrackSize.y;
                if(sizemove->rcWin.bottom + dy > bmax)
                {
                    sizemove->rcWin.bottom = bmax;
                }
                else if(sizemove->rcWin.bottom + dy < bmin)
                {
                    sizemove->rcWin.bottom = bmin;
                }
                else
                {
                    sizemove->rcWin.bottom += dy;
                }
                pt.y = sizemove->rcWin.bottom;
                if(wpHit == WMSZ_LEFT)
                {
                    wpHit = WMSZ_BOTTOMLEFT;
                }
                else if(wpHit == WMSZ_RIGHT)
                {
                    wpHit = WMSZ_BOTTOMRIGHT;
                }
                else
                {
                    wpHit = WMSZ_BOTTOM;
                }
            }
        }
#ifdef TIME_LOOP
        QueryPerformanceCounter(&pfDraw);
#endif
        SendMessageAW(sizemove->hwnd, WM_SIZING, wpHit, (LPARAM)&sizemove->rcWin);
        SetWindowPos(sizemove->hwnd, 0, sizemove->rcWin.left, sizemove->rcWin.top,
					 RECTWIDTH(sizemove->rcWin), RECTHEIGHT(sizemove->rcWin), 0);
		
        sizemove->ptCapture = pt;
        /* when resizing using keys, Windows also moves the cursor */
        if(changeCursor)
            SetCursorPos(pt.x, pt.y);
#ifdef TIME_LOOP
        QueryPerformanceCounter(&pfFinal);
        if(1)
        {
            double msTotal, msDraw;
            msTotal = (double)(*(long long*)(&pfFinal) - *(long long *)(&pfBegin)) / (*(long long *)(&pfFreq) / 1000);
            msDraw = (double)(*(long long*)(&pfFinal) - *(long long *)(&pfDraw)) / (*(long long *)(&pfFreq) / 1000);
            printf("draw time: %.03fms\ntotal:     %.03fms\n", msDraw, msTotal);
        }
#endif
	}
	return 1;
}

void LSMCleanup()
{
    SIZEMOVEDATA *sm = LSMGet();
    if(sm)
    {
        if(sm->shake.restoreList)
        {
            ForEachShakeNode(&sm->shake, ForEachShakeNodeFree, 0);
        }
        if(sm->snap.helperWindow)
        {
            DestroyWindow(sm->snap.helperWindow);
        }
        LocalFree(sm);
        LSMSet(NULL);
    }
}
