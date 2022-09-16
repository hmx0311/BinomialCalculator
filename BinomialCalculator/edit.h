#pragma once

#include "button.h"

#include <string>

class NumericEdit
{
private:
	HWND hEdit;
	Button clearButton;
	std::wstring curUndo = _T("");
	std::wstring lastUndo = _T("");
public:
	void attach(HWND hEdit);
	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	HWND getHwnd();
	void getVCenteredRect(PRECT pRect);
	void setText(PCTSTR str);
private:
	void initLayout();
	void updateStr();
};

LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);