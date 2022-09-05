#pragma once

extern WNDPROC defListBoxProc;

LRESULT CALLBACK listBoxProc(HWND hListBox, UINT message, WPARAM wParam, LPARAM lParam);

class ListBox
{
private:
	HWND hListBox;
	int resultCnt = 0;
	bool isTracking = false;
	int lastTrackItemID = -1;
	bool isInClkRect = true;
public:
	LRESULT wndProc(UINT message, WPARAM wParam, LPARAM lParam);
	void drawItem(HDC hDC, int itemID, UINT itemState, RECT& rcItem);
	void attach(HWND hListBox);
	HWND getHwnd();
	void addResult(LPCWSTR str);
	void reset();
};
