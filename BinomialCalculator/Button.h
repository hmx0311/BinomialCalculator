#pragma once
#include <uxtheme.h>
#include <Vsstyle.h>

extern HTHEME hButtonTheme;

class Button
{
private:
	PUSHBUTTONSTATES lastState = PBS_NORMAL;
protected:
	HWND hButton;
	HICON hIcon = nullptr;
	HBRUSH hBkgBrush = GetSysColorBrush(CTLCOLOR_DLG);
	bool isTracking = false;
public:
	virtual LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void drawItem(HDC hDC, UINT itemState, RECT& rcItem);
	void attach(HWND hButton);
	HWND getHwnd();
	void setText(LPCWSTR str);
	void setIcon(HICON hIcon);
	void setBkgBrush(HBRUSH hBkgBrush);
private:
	void drawButton(HDC hDC, PUSHBUTTONSTATES states, RECT& rcItem);
};
