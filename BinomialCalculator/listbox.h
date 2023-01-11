#pragma once

class ResultList
{
private:
	HWND hLB;
	bool isPopingMenu = false;
	bool isTracking = false;
	int lastTrackItemID = -1;
	bool isInClkRect = true;

public:
	void attach(HWND hLB);
	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void drawItem(HDC hDC, int itemID, UINT itemState, RECT& rcItem);
	HWND getHwnd();
	void addResult(PCTSTR str);
	void reset();
};
