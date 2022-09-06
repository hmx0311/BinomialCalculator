#include "framework.h"
#include "Button.h"

#include <CommCtrl.h>

#define BUTTON_ANIMATION_DURATION_SHORT 150
#define BUTTON_ANIMATION_DURATION_LONG  200

#define BUTTON_MARGIN_RATIO 0.1f

HTHEME hButtonTheme;

static LRESULT CALLBACK buttonSubclassProc(HWND hButton, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	return ((Button*)GetWindowLongPtr(hButton, GWLP_USERDATA))->wndProc(msg, wParam, lParam);
}

void Button::attach(HWND hButton)
{
	this->hButton = hButton;
	SetWindowLongPtr(hButton, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hButton, buttonSubclassProc, 0, 0);
}

LRESULT Button::wndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return LRESULT(TRUE);
	case WM_MOVE:
		BufferedPaintStopAllAnimations(hButton);
		break;
	case WM_MOUSEMOVE:
		{
			if (isTracking)
			{
				break;
			}
			isTracking = true;
			TRACKMOUSEEVENT eventTrack = { sizeof(TRACKMOUSEEVENT),TME_LEAVE,hButton ,0 };
			TrackMouseEvent(&eventTrack);
			InvalidateRect(hButton, nullptr, FALSE);
			break;
		}
	case WM_MOUSELEAVE:
		{
			isTracking = false;
			InvalidateRect(hButton, nullptr, FALSE);
			break;
		}
	case WM_LBUTTONDBLCLK:
		SendMessage(hButton, WM_LBUTTONDOWN, wParam, lParam);
		return LRESULT(TRUE);
	}
	return DefSubclassProc(hButton, msg, wParam, lParam);
}

void Button::drawItem(HDC hDC, UINT itemState, RECT& rcItem)
{
	PUSHBUTTONSTATES state = PBS_NORMAL;
	if (itemState & ODS_SELECTED)
	{
		state = PBS_PRESSED;
	}
	else if (isTracking)
	{
		state = PBS_HOT;
	}
	HFONT hFont = (HFONT)SendMessage(hButton, WM_GETFONT, 0, 0);
	BP_ANIMATIONPARAMS animParams = { sizeof(BP_ANIMATIONPARAMS),0, BPAS_LINEAR, state == PBS_PRESSED ? BUTTON_ANIMATION_DURATION_SHORT : BUTTON_ANIMATION_DURATION_LONG };
	HDC hdcFrom, hdcTo;
	HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(hButton, hDC, &rcItem, BPBF_COMPATIBLEBITMAP, NULL, &animParams, &hdcFrom, &hdcTo);
	if (lastState == state || hbpAnimation == nullptr)
	{
		int width = rcItem.right - rcItem.left;
		int height = rcItem.bottom - rcItem.top;
		HBITMAP hBmBuffer = CreateCompatibleBitmap(hDC, width, height);
		HDC hDCMem = CreateCompatibleDC(hDC);
		SelectObject(hDCMem, hBmBuffer);
		SelectObject(hDCMem, hFont);
		drawButton(hDCMem, state, rcItem);
		BitBlt(hDC, rcItem.left, rcItem.top, width, height, hDCMem, 0, 0, SRCCOPY);
		DeleteDC(hDCMem);
		DeleteObject(hBmBuffer);
	}
	else
	{
		SelectObject(hdcFrom, hFont);
		SelectObject(hdcTo, hFont);
		drawButton(hdcFrom, lastState, rcItem);
		drawButton(hdcTo, state, rcItem);
		BufferedPaintStopAllAnimations(hButton);
		EndBufferedAnimation(hbpAnimation, TRUE);
	}
	lastState = state;
}

HWND Button::getHwnd()
{
	return hButton;
}

void Button::setText(LPCWSTR str)
{
	SetWindowText(hButton, str);
}

void Button::setIcon(HICON hIcon)
{
	this->hIcon = hIcon;
	InvalidateRect(hButton, nullptr, FALSE);
}

void Button::setBkgBrush(HBRUSH hBkgBrush)
{
	this->hBkgBrush = hBkgBrush;
}

void Button::drawButton(HDC hDC, PUSHBUTTONSTATES state, RECT& rcItem)
{
	FillRect(hDC, &rcItem, hBkgBrush);
	RECT rcContent = rcItem;
	int margin;
	if (GetWindowLongPtr(hButton, GWL_STYLE) & BS_FLAT)
	{
		if (state == PBS_HOT)
		{
			FillRect(hDC, &rcItem, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
		}
		margin = BUTTON_MARGIN_RATIO * min(rcContent.right - rcContent.left, rcContent.bottom - rcContent.top);
	}
	else
	{
		DrawThemeBackground(hButtonTheme, hDC, BP_PUSHBUTTON, state, &rcItem, 0);
		GetThemeBackgroundContentRect(hButtonTheme, hDC, BP_PUSHBUTTON, state, &rcItem, &rcContent);
		margin = BUTTON_MARGIN_RATIO * min(rcContent.right - rcContent.left, rcContent.bottom - rcContent.top) - 1;
	}
	rcContent.left += margin;
	rcContent.top += margin;
	rcContent.right -= margin;
	rcContent.bottom -= margin;

	if (hIcon != nullptr)
	{
		ICONINFO iconInfo;
		GetIconInfo(hIcon, &iconInfo);
		BITMAP bmMask;
		GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmMask);
		HDC hDCImage = CreateCompatibleDC(hDC);
		HBITMAP hBmBuffer = CreateCompatibleBitmap(hDC, bmMask.bmWidth, bmMask.bmHeight);
		SelectObject(hDCImage, hBmBuffer);
		SetStretchBltMode(hDCImage, HALFTONE);
		StretchBlt(hDCImage, 0, 0, bmMask.bmWidth, bmMask.bmHeight, hDC, rcContent.left, rcContent.top, rcContent.right - rcContent.left, rcContent.bottom - rcContent.top, SRCCOPY);
		DrawIcon(hDCImage, 0, 0, hIcon);
		SetStretchBltMode(hDC, HALFTONE);
		StretchBlt(hDC, rcContent.left, rcContent.top, rcContent.right - rcContent.left, rcContent.bottom - rcContent.top, hDCImage, 0, 0, bmMask.bmWidth, bmMask.bmHeight, SRCCOPY);
		DeleteObject(hBmBuffer);
		DeleteObject(hDCImage);
	}
	SetBkMode(hDC, TRANSPARENT);
	TCHAR str[10];
	GetWindowText(hButton, str, 10);
	DrawText(hDC, str, wcslen(str), &rcContent, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}
