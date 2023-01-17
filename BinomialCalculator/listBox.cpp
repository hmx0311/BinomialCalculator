#include "framework.h"
#include "listbox.h"

#include "BinomialCalculator.h"

#include <uxtheme.h>
#include <windowsx.h>

static LRESULT listBoxSubclassProc(HWND hLB, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	return ((ResultList*)GetWindowLongPtr(hLB, GWLP_USERDATA))->wndProc(msg, wParam, lParam);
}

void ResultList::attach(HWND hLB)
{
	this->hLB = hLB;
	SetWindowLongPtr(hLB, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hLB, listBoxSubclassProc, 0, 0);
}

LRESULT ResultList::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CONTEXTMENU:
		{
			POINT pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			MapWindowPoints(HWND_DESKTOP, hLB, &pos, 1);
			int index = SendMessage(hLB, LB_ITEMFROMPOINT, 0, POINTTOPOINTS(pos));
			if (HIWORD(index) == 0)
			{
				isPopingMenu = true;
				HMENU menu = CreatePopupMenu();
				AppendMenu(menu, 0, 1, _T("É¾³ý(&D)"));
				if (TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hLB, nullptr) != 0)
				{
					ListBox_DeleteString(hLB, index);
				}
				DestroyMenu(menu);
				isPopingMenu = false;
				SendMessage(hLB, WM_MOUSELEAVE, 0, 0);
			}
			return 0;
		}
	case WM_ERASEBKGND:
		{
			RECT rect;
			GetClientRect(hLB, &rect);
			rect.top += (ListBox_GetCount(hLB) - GetScrollPos(hLB, SB_VERT)) * ListBox_GetItemHeight(hLB, 0);
			if (rect.top < rect.bottom)
			{
				FillRect((HDC)wParam, &rect, GetSysColorBrush(COLOR_WINDOW));
			}
			return (LRESULT)TRUE;
		}
	case WM_LBUTTONDBLCLK:
		msg = WM_LBUTTONDOWN;
		break;
	case WM_MOUSEMOVE:
		if (ListBox_GetCount(hLB) > 0)
		{
			if (!isTracking)
			{
				isTracking = true;
				TRACKMOUSEEVENT eventTrack = { sizeof(TRACKMOUSEEVENT),TME_LEAVE,hLB ,0 };
				TrackMouseEvent(&eventTrack);
			}
			POINT pos = { GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
			if (wParam & MK_LBUTTON)
			{
				if (lastTrackItemID == -1)
				{
					return 0;
				}
				RECT rcItem;
				ListBox_GetItemRect(hLB, lastTrackItemID, &rcItem);
				if (PtInRect(&rcItem, pos))
				{
					if (!isInClkRect)
					{
						isInClkRect = true;
						HDC hDC = GetDC(hLB);
						drawItem(hDC, lastTrackItemID, ODS_SELECTED, rcItem);
						ReleaseDC(hLB, hDC);
					}
				}
				else
				{
					if (isInClkRect)
					{
						isInClkRect = false;
						HDC hDC = GetDC(hLB);
						drawItem(hDC, lastTrackItemID, ODS_HOTLIGHT, rcItem);
						ReleaseDC(hLB, hDC);
					}
				}
			}
			else
			{
				int itemID = (short)LOWORD(SendMessage(hLB, LB_ITEMFROMPOINT, 0, lParam));
				RECT rcItem;
				ListBox_GetItemRect(hLB, itemID, &rcItem);
				if (!PtInRect(&rcItem, pos))
				{
					itemID = -1;
				}
				if (itemID == lastTrackItemID)
				{
					return 0;
				}
				HDC hDC = GetDC(hLB);
				if (lastTrackItemID != -1)
				{
					RECT rcLast;
					ListBox_GetItemRect(hLB, lastTrackItemID, &rcLast);
					drawItem(hDC, lastTrackItemID, 0, rcLast);
				}
				if (itemID != -1)
				{
					drawItem(hDC, itemID, ODS_HOTLIGHT, rcItem);
				}
				ReleaseDC(hLB, hDC);
				lastTrackItemID = itemID;
			}
		}
		return 0;
	case WM_LBUTTONUP:
		if (ListBox_GetCount(hLB) > 0)
		{
			ListBox_SetCurSel(hLB, -1);
			if (isInClkRect && lastTrackItemID != -1)
			{
				RECT rcItem;
				ListBox_GetItemRect(hLB, lastTrackItemID, &rcItem);
				TCHAR str[41];
				ListBox_GetText(hLB, lastTrackItemID, str);
				SendMessage(GetParent(hLB), WM_COMMAND, ID_RETRIEVE_RESULT, (LPARAM)str);
				HDC hDC = GetDC(hLB);
				drawItem(hDC, lastTrackItemID, ODS_HOTLIGHT, rcItem);
				ReleaseDC(hLB, hDC);
			}
			isInClkRect = true;
		}
		break;
	case WM_MOUSELEAVE:
		{
			isTracking = false;
			if (!isPopingMenu && lastTrackItemID != -1)
			{
				RECT rcLast;
				ListBox_GetItemRect(hLB, lastTrackItemID, &rcLast);
				HDC hDC = GetDC(hLB);
				drawItem(hDC, lastTrackItemID, 0, rcLast);
				ReleaseDC(hLB, hDC);
				lastTrackItemID = -1;
			}
			ListBox_SetCurSel(hLB, -1);
			isInClkRect = true;
			break;
		}
	case WM_MOUSEWHEEL:
		ListBox_SetCurSel(hLB, -1);
		lastTrackItemID = -1;
		isInClkRect = true;
		break;
	case WM_SETFOCUS:
		SetFocus((HWND)wParam);
		break;
	}
	return DefSubclassProc(hLB, msg, wParam, lParam);
}

void ResultList::drawItem(HDC hDC, int itemID, UINT itemState, RECT& rcItem)
{
	if (itemID == -1 || isPopingMenu)
	{
		return;
	}
	HDC hMemDC;
	HPAINTBUFFER hPaintBuffer = BeginBufferedPaint(hDC, &rcItem, BPBF_COMPATIBLEBITMAP, nullptr, &hMemDC);
	HFONT hOldFont=SelectFont(hMemDC, hNormalFont);

	if (itemState & ODS_HOTLIGHT || !isInClkRect)
	{
		FillRect(hMemDC, &rcItem, GetStockBrush(LTGRAY_BRUSH));
	}
	else if (itemState & ODS_SELECTED)
	{
		FillRect(hMemDC, &rcItem, GetStockBrush(GRAY_BRUSH));
	}
	else
	{
		FillRect(hMemDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
	}

	SetBkMode(hMemDC, TRANSPARENT);
	TCHAR sText[41];
	ListBox_GetText(hLB, itemID, sText);
	SetTextColor(hMemDC, GetSysColor(COLOR_WINDOWTEXT));
	DrawText(hMemDC, sText, -1, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	SelectFont(hMemDC, hOldFont);
	EndBufferedPaint(hPaintBuffer, TRUE);
}

HWND ResultList::getHwnd()
{
	return hLB;
}

void ResultList::addResult(PCTSTR str)
{
	int i = ListBox_FindString(hLB, -1, str);
	if (i == 0)
	{
		return;
	}
	if (i > 0)
	{
		ListBox_DeleteString(hLB, i);
	}
	ListBox_InsertString(hLB, 0, str);
	ListBox_SetTopIndex(hLB, 0);
	lastTrackItemID = -1;
	isInClkRect = true;
}

void ResultList::reset()
{
	ListBox_ResetContent(hLB);
}
