#pragma once

#include <string>

void initNumericEdit(HWND hEdit);

class NumericEdit
{
private:
	HWND hEdit;
	std::wstring curUndo = _T("");
	std::wstring lastUndo = _T("");
public:
	void attach(HWND hEdit);
	LRESULT wndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	HWND getHwnd();
	void setText(PCTSTR str);
private:
	void updateStr();
};

LRESULT CALLBACK readOnlyEditSubclassProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);