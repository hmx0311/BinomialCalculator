#include "framework.h"
#include "listbox.h"

#include "BinomialCalculator.h"

#include <CommCtrl.h>
#include <windowsx.h>

static LRESULT listBoxSubclassProc(HWND hListBox, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	return ((ResultList*)GetWindowLongPtr(hListBox, GWLP_USERDATA))->wndProc(msg, wParam, lParam);
}

void ResultList::attach(HWND hListBox)
{
	this->hListBox = hListBox;
	SetWindowLongPtr(hListBox, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hListBox, listBoxSubclassProc, 0, 0);
}

LRESULT ResultList::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		{
			RECT rect;
			GetClientRect(hListBox, &rect);
			rect.top += (SendMessage(hListBox, LB_GETCOUNT, 0, 0) - GetScrollPos(hListBox, SB_VERT)) * SendMessage(hListBox, LB_GETITEMHEIGHT, 0, 0);
			if (rect.top < rect.bottom)
			{
				FillRect((HDC)wParam, &rect, GetSysColorBrush(COLOR_WINDOW));
			}
			return (LRESULT)TRUE;
		}
	case WM_KEYDOWN:
		return (LRESULT)TRUE;
	case WM_LBUTTONDBLCLK:
		msg = WM_LBUTTONDOWN;
		break;
	case WM_MOUSEMOVE:
		{
			if (resultCnt == 0)
			{
				return (LRESULT)TRUE;
			}
			if (!isTracking)
			{
				isTracking = true;
				TRACKMOUSEEVENT eventTrack = { sizeof(TRACKMOUSEEVENT),TME_LEAVE,hListBox ,0 };
				TrackMouseEvent(&eventTrack);
			}
			POINT pos = { GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
			if (wParam & MK_LBUTTON)
			{
				if (lastTrackItemID == -1)
				{
					return (LRESULT)TRUE;
				}
				RECT rcItem;
				SendMessage(hListBox, LB_GETITEMRECT, lastTrackItemID, (WPARAM)&rcItem);
				if (PtInRect(&rcItem, pos))
				{
					if (!isInClkRect)
					{
						isInClkRect = true;
						HDC hDC = GetDC(hListBox);
						drawItem(hDC, lastTrackItemID, ODS_SELECTED, rcItem);
						ReleaseDC(hListBox, hDC);
					}
				}
				else
				{
					if (isInClkRect)
					{
						isInClkRect = false;
						HDC hDC = GetDC(hListBox);
						drawItem(hDC, lastTrackItemID, ODS_HOTLIGHT, rcItem);
						ReleaseDC(hListBox, hDC);
					}
				}
			}
			else
			{
				int itemID = (short)LOWORD(SendMessage(hListBox, LB_ITEMFROMPOINT, 0, lParam));
				RECT rcItem;
				SendMessage(hListBox, LB_GETITEMRECT, itemID, (WPARAM)&rcItem);
				if (!PtInRect(&rcItem, pos))
				{
					itemID = -1;
				}
				if (itemID == lastTrackItemID)
				{
					return (LRESULT)TRUE;
				}
				HDC hDC = GetDC(hListBox);
				if (lastTrackItemID != -1)
				{
					RECT rcLast;
					SendMessage(hListBox, LB_GETITEMRECT, lastTrackItemID, (WPARAM)&rcLast);
					drawItem(hDC, lastTrackItemID, 0, rcLast);
				}
				if (itemID != -1)
				{
					drawItem(hDC, itemID, ODS_HOTLIGHT, rcItem);
				}
				ReleaseDC(hListBox, hDC);
				lastTrackItemID = itemID;
			}
			return (LRESULT)TRUE;
		}
	case WM_LBUTTONUP:
		if (resultCnt > 0)
		{
			SendMessage(hListBox, LB_SETCURSEL, -1, 0);
			if (isInClkRect && lastTrackItemID != -1)
			{
				RECT rcItem;
				SendMessage(hListBox, LB_GETITEMRECT, lastTrackItemID, (WPARAM)&rcItem);
				TCHAR str[41];
				SendMessage(hListBox, LB_GETTEXT, lastTrackItemID, (LPARAM)str);
				SendMessage(GetParent(hListBox), WM_COMMAND, ID_RETRIEVE_RESULT, (LPARAM)str);
				HDC hDC = GetDC(hListBox);
				drawItem(hDC, lastTrackItemID, ODS_HOTLIGHT, rcItem);
				ReleaseDC(hListBox, hDC);
			}
			isInClkRect = true;
		}
		break;
	case WM_MOUSELEAVE:
		{
			isTracking = false;
			if (lastTrackItemID != -1)
			{
				RECT rcLast;
				SendMessage(hListBox, LB_GETITEMRECT, lastTrackItemID, (WPARAM)&rcLast);
				HDC hDC = GetDC(hListBox);
				drawItem(hDC, lastTrackItemID, 0, rcLast);
				ReleaseDC(hListBox, hDC);
			}
			SendMessage(hListBox, LB_SETCURSEL, -1, 0);
			lastTrackItemID = -1;
			isInClkRect = true;
			break;
		}
	case WM_MOUSEWHEEL:
		SendMessage(hListBox, LB_SETCURSEL, -1, 0);
		lastTrackItemID = -1;
		isInClkRect = true;
		break;
	}
	return DefSubclassProc(hListBox, msg, wParam, lParam);
}

void ResultList::drawItem(HDC hDC, int itemID, UINT itemState, RECT& rcItem)
{
	if (itemID == -1)
	{
		return;
	}
	int width = rcItem.right - rcItem.left;
	int height = rcItem.bottom - rcItem.top;
	HBITMAP bmp = CreateCompatibleBitmap(hDC, width, height);
	HDC hDCMem = CreateCompatibleDC(hDC);
	SelectObject(hDCMem, bmp);
	SelectObject(hDCMem, hNormalFont);

	RECT rcContent = { 0,0, rcItem.right, rcItem.bottom - rcItem.top };

	if (itemState & ODS_HOTLIGHT || !isInClkRect)
	{
		FillRect(hDCMem, &rcContent, GetStockBrush(LTGRAY_BRUSH));
	}
	else if (itemState & ODS_SELECTED)
	{
		FillRect(hDCMem, &rcContent, GetStockBrush(GRAY_BRUSH));
	}
	else
	{
		FillRect(hDCMem, &rcContent, GetSysColorBrush(COLOR_WINDOW));
	}

	SetBkMode(hDCMem, TRANSPARENT);
	TCHAR sText[41];
	SendMessage(hListBox, LB_GETTEXT, itemID, (LPARAM)sText);
	DrawText(hDCMem, sText, -1, &rcContent, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	BitBlt(hDC, rcItem.left, rcItem.top, width, height, hDCMem, 0, 0, SRCCOPY);
	DeleteDC(hDCMem);
	DeleteObject(bmp);
}

HWND ResultList::getHwnd()
{
	return hListBox;
}

void ResultList::addResult(LPCWSTR str)
{
	SendMessage(hListBox, LB_INSERTSTRING, 0, (LPARAM)str);
	SendMessage(hListBox, LB_SETTOPINDEX, 0, 0);
	resultCnt++;
}

void ResultList::reset()
{
	SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
	resultCnt = 0;
}
